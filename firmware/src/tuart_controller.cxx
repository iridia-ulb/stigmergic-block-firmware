
#include "tuart_controller.h"

/***********************************************************/
/***********************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>

/***********************************************************/
/***********************************************************/

#define PRESCALE_THRESHOLD 7085

CTUARTController::CInputCaptureInterrupt::CInputCaptureInterrupt(CTUARTController* pc_tuart_controller, uint8_t un_intr_vect_num) : 
   m_pcTUARTController(pc_tuart_controller) {
   Register(this, un_intr_vect_num);
}

CTUARTController::COutputCompareAInterrupt::COutputCompareAInterrupt(CTUARTController* pc_tuart_controller, uint8_t un_intr_vect_num) : 
   m_pcTUARTController(pc_tuart_controller) {
   Register(this, un_intr_vect_num);
}

CTUARTController::COutputCompareBInterrupt::COutputCompareBInterrupt(CTUARTController* pc_tuart_controller, uint8_t un_intr_vect_num) : 
   m_pcTUARTController(pc_tuart_controller) {
   Register(this, un_intr_vect_num);
}

/****************************************/
/****************************************/

void CTUARTController::CInputCaptureInterrupt::ServiceRoutine() {
   uint8_t state, bit, head;
   uint16_t capture, target;
   //int16_t offset;




   
   //m_pcTUARTController->InterruptICOccured = true;

   //capture = GET_INPUT_CAPTURE();
   capture = m_pcTUARTController->m_unInputCaptureRegister;

   m_pcTUARTController->DetectedEdges++;



   bit = m_pcTUARTController->m_unRxBit;
   if (bit) {
      //CONFIG_CAPTURE_FALLING_EDGE();
      m_pcTUARTController->m_unControlRegisterB &= ~_BV(ICES1);
      m_pcTUARTController->m_unRxBit = 0;
   } 
   else {
      //CONFIG_CAPTURE_RISING_EDGE();
      m_pcTUARTController->m_unControlRegisterB |= _BV(ICES1);
      m_pcTUARTController->m_unRxBit = 0x80;
   }
   /*
   m_pcTUARTController->Samples[m_pcTUARTController->DetectedEdges].EdgeIndex = m_pcTUARTController->DetectedEdges;
   m_pcTUARTController->Samples[m_pcTUARTController->DetectedEdges].CaptureTime = capture;
   m_pcTUARTController->Samples[m_pcTUARTController->DetectedEdges].RxState = m_pcTUARTController->m_unRxState;
   m_pcTUARTController->Samples[m_pcTUARTController->DetectedEdges].RxByte = m_pcTUARTController->m_unRxByte;
   m_pcTUARTController->Samples[m_pcTUARTController->DetectedEdges].RxBit = m_pcTUARTController->m_unRxBit;
   */
   state = m_pcTUARTController->m_unRxState;
   if (state == 0) {
      if (!bit) {
         //SET_COMPARE_B(capture + rx_stop_ticks);
         // (OCR1B = (val))
         /* Set compare B interrupt to fire just after the rising edge of the RX stop bit,
            assumes: 8 bit data, 1 stop bit, no parity */
         m_pcTUARTController->m_unOutputCompareRegisterB = capture + m_pcTUARTController->m_unRxStopTicks;
         //ENABLE_INT_COMPARE_B();
         // (TIFR1 = (1<<OCF1B), TIMSK1 |= (1<<OCIE1B))
         /* clear output compare B interrupt flag and enable */
         m_pcTUARTController->m_unInterruptFlagRegister = _BV(OCF1B);
         m_pcTUARTController->m_unInterruptMaskRegister |= _BV(OCIE1B);

         //m_pcTUARTController->EnabledOCBInterrupt = true;
         /* set m_unRxTarget to the next bit in the RX waveform that we want to sample */
         m_pcTUARTController->m_unRxTarget = capture + m_pcTUARTController->m_unTicksPerBit + m_pcTUARTController->m_unTicksPerBit/2;
         /* now our state is searching for bit 1 */
         m_pcTUARTController->m_unRxState = 1; 
      }
   }
   else {
      target = m_pcTUARTController->m_unRxTarget;
      while (1) {
         //offset = capture - target;
         //if (int32_t(capture) - target < 0) break;
         if (int16_t(capture - target) < 0) break;
         m_pcTUARTController->m_unRxByte = (m_pcTUARTController->m_unRxByte >> 1) | m_pcTUARTController->m_unRxBit;

         target += m_pcTUARTController->m_unTicksPerBit;
         state++;
         if (state >= 9) {
            /* byte is received, move it to the rx buffer */
            //DISABLE_INT_COMPARE_B();
            // (TIMSK1 &= ~(1<<OCIE1B))

            m_pcTUARTController->m_unInterruptMaskRegister &= ~_BV(OCIE1B);
            
            head = m_pcTUARTController->m_sRxBuffer.Head + 1;
            if (head >= IO_BUFFER_SIZE) { head = 0; }
            if (head != m_pcTUARTController->m_sRxBuffer.Tail) {
               m_pcTUARTController->m_sRxBuffer.Buffer[head] = m_pcTUARTController->m_unRxByte;
               m_pcTUARTController->m_sRxBuffer.Head = head;
            }
            /* configure input capture to search for the falling edge of the next start bit */
            //CONFIG_CAPTURE_FALLING_EDGE();
            // TCCR1B &= ~(1<<ICES1))
            m_pcTUARTController->m_unControlRegisterB &= ~_BV(ICES1);
            m_pcTUARTController->m_unRxBit = 0;
            m_pcTUARTController->m_unRxState = 0;
            return;
         }
      }
      m_pcTUARTController->m_unRxTarget = target;
      m_pcTUARTController->m_unRxState = state;
   }
}

void CTUARTController::COutputCompareAInterrupt::ServiceRoutine() {
   uint8_t state, byte, bit, head, tail;
   uint16_t target;

   //m_pcTUARTController->InterruptOCAOccured = true;

   state = m_pcTUARTController->m_unTxState;
   byte = m_pcTUARTController->m_unTxByte;
   //target = GET_COMPARE_A();
   // GET_COMPARE_A() (OCR1A)
   target = m_pcTUARTController->m_unOutputCompareRegisterA;

   while (state < 9) {
      target += m_pcTUARTController->m_unTicksPerBit;
      bit = byte & 1;
      byte >>= 1;
      state++;
      if (bit != m_pcTUARTController->m_unTxBit) {
         if (bit) {
            //CONFIG_MATCH_SET();
            //CONFIG_MATCH_SET() (m_unControlRegisterA = m_unControlRegisterA | ((1<<COM1A1) | (1<<COM1A0)))
            m_pcTUARTController->m_unControlRegisterA = (m_pcTUARTController->m_unControlRegisterA | (_BV(COM1A1)) | _BV(COM1A0));

         } else {
            //CONFIG_MATCH_CLEAR();
            //CONFIG_MATCH_CLEAR() (m_unControlRegisterA = (m_unControlRegisterA | (1<<COM1A1)) & ~(1<<COM1A0))
            m_pcTUARTController->m_unControlRegisterA = (m_pcTUARTController->m_unControlRegisterA | _BV(COM1A1)) & ~_BV(COM1A0);
         }
         //SET_COMPARE_A(target);
         // SET_COMPARE_A(val)		(OCR1A = (val))
         m_pcTUARTController->m_unOutputCompareRegisterA = target; 
         m_pcTUARTController->m_unTxBit = bit;
         m_pcTUARTController->m_unTxByte = byte;
         m_pcTUARTController->m_unTxState = state;
         // TODO: how to detect timing_error?
         return;
      }
   }
   if (state == 9) {
      m_pcTUARTController->m_unTxState = 10;
      //CONFIG_MATCH_SET();
      //CONFIG_MATCH_SET() (m_unControlRegisterA = m_unControlRegisterA | ((1<<COM1A1) | (1<<COM1A0)))
      m_pcTUARTController->m_unControlRegisterA = m_pcTUARTController->m_unControlRegisterA | (_BV(COM1A1) | _BV(COM1A0));
      //SET_COMPARE_A(target + ticks_per_bit);
      //SET_COMPARE_A(val)		(OCR1A = (val))
      m_pcTUARTController->m_unOutputCompareRegisterA = target + m_pcTUARTController->m_unTicksPerBit; 
      return;
   }
   head = m_pcTUARTController->m_sTxBuffer.Head;
   tail = m_pcTUARTController->m_sTxBuffer.Tail;
   if (head == tail) {
      m_pcTUARTController->m_unTxState = 0;

      //CONFIG_MATCH_NORMAL();
      //#define CONFIG_MATCH_NORMAL()		(m_unControlRegisterA = m_unControlRegisterA & ~((1<<COM1A1) | (1<<COM1A0)))
      //m_pcTUARTController->m_unControlRegisterA = m_pcTUARTController->m_unControlRegisterA & ~(_BV(COM1A1) | _BV(COM1A0));
      m_pcTUARTController->m_unControlRegisterA &= ~(_BV(COM1A1) | _BV(COM1A0));
      
      //DISABLE_INT_COMPARE_A();
      //#define DISABLE_INT_COMPARE_A()	(m_unInterruptMaskRegister &= ~(1<<OCIE1A))
      m_pcTUARTController->m_unInterruptMaskRegister &= ~_BV(OCIE1A);
   }
   else {
      m_pcTUARTController->m_unTxState = 1;
      if (++tail >= IO_BUFFER_SIZE) tail = 0;
      m_pcTUARTController->m_sTxBuffer.Tail = tail;
      m_pcTUARTController->m_unTxByte = m_pcTUARTController->m_sTxBuffer.Buffer[tail];
      m_pcTUARTController->m_unTxBit = 0;

      //CONFIG_MATCH_CLEAR();
      //CONFIG_MATCH_CLEAR() (m_unControlRegisterA = (m_unControlRegisterA | (1<<COM1A1)) & ~(1<<COM1A0))
      m_pcTUARTController->m_unControlRegisterA = (m_pcTUARTController->m_unControlRegisterA | _BV(COM1A1)) & ~_BV(COM1A0);

      //SET_COMPARE_A(target + ticks_per_bit);
      //SET_COMPARE_A(val)		(OCR1A = (val))
      m_pcTUARTController->m_unOutputCompareRegisterA = target + m_pcTUARTController->m_unTicksPerBit; 

      // TODO: how to detect timing_error?
   }
}

/* This interrupt only ever occurs if the stop bit is missing */
void CTUARTController::COutputCompareBInterrupt::ServiceRoutine() {
   uint8_t head, state, bit;

   //m_pcTUARTController->InterruptOCBOccured = true;

   //DISABLE_INT_COMPARE_B();
   //DISABLE_INT_COMPARE_B()	(m_unInterruptMaskRegister &= ~(1<<OCIE1B))
   m_pcTUARTController->m_unInterruptMaskRegister &= ~_BV(OCIE1B);

   //CONFIG_CAPTURE_FALLING_EDGE();
   //CONFIG_CAPTURE_FALLING_EDGE()	(m_unControlRegisterB &= ~(1<<ICES1))
   /* configure input capture to search for the falling edge of the next start bit */
   m_pcTUARTController->m_unControlRegisterB &= ~_BV(ICES1);

   state = m_pcTUARTController->m_unRxState;
   bit = m_pcTUARTController->m_unRxBit ^ 0x80; /* toggle MSB of m_unRxBit */
   while (state < 9) {
      m_pcTUARTController->m_unRxByte = (m_pcTUARTController->m_unRxByte >> 1) | bit;
      state++;
   }
   head = m_pcTUARTController->m_sRxBuffer.Head + 1;
   if (head >= IO_BUFFER_SIZE) head = 0;
   if (head != m_pcTUARTController->m_sRxBuffer.Tail) {
      m_pcTUARTController->m_sRxBuffer.Buffer[head] = m_pcTUARTController->m_unRxByte;
      m_pcTUARTController->m_sRxBuffer.Head = head;
   }
   m_pcTUARTController->m_unRxState = 0;
   //CONFIG_CAPTURE_FALLING_EDGE();
   //CONFIG_CAPTURE_FALLING_EDGE()	(m_unControlRegisterB &= ~(1<<ICES1))
   m_pcTUARTController->m_unControlRegisterB &= ~_BV(ICES1);
   m_pcTUARTController->m_unRxBit = 0;
}

/****************************************/
/****************************************/

CTUARTController::CTUARTController(uint32_t un_baud_rate,
                                   volatile uint8_t& un_ctrl_reg_a,
                                   volatile uint8_t& un_ctrl_reg_b,
                                   volatile uint8_t& un_intr_mask_reg,
                                   volatile uint8_t& un_intr_flag_reg,
                                   volatile uint16_t& un_input_capt_reg,
                                   volatile uint16_t& un_output_comp_reg_a,
                                   volatile uint16_t& un_output_comp_reg_b,
                                   volatile uint16_t& un_cnt_reg,
                                   uint8_t un_input_capt_intr_num,
                                   uint8_t un_output_comp_a_intr_num,
                                   uint8_t un_output_comp_b_intr_num) :
m_unControlRegisterA(un_ctrl_reg_a),
   m_unControlRegisterB(un_ctrl_reg_b),
   m_unInterruptMaskRegister(un_intr_mask_reg),
   m_unInterruptFlagRegister(un_intr_flag_reg),
   m_unInputCaptureRegister(un_input_capt_reg),
   m_unOutputCompareRegisterA(un_output_comp_reg_a),
   m_unOutputCompareRegisterB(un_output_comp_reg_b),
   m_unCountRegister(un_cnt_reg),
   m_unRxState(0),  
   m_unTxState(0),
   m_unTicksPerBit(0),
   m_bTimingError(false),
   m_unRxTarget(0),
   m_unRxStopTicks(0),
   m_unTxByte(0),
   m_unRxByte(0),
   m_unTxBit(0),
   m_unRxBit(0),
   m_cInputCaptureInterrupt(this, un_input_capt_intr_num),
   m_cOutputCompareAInterrupt(this, un_output_comp_a_intr_num),
   m_cOutputCompareBInterrupt(this, un_output_comp_b_intr_num) {

   /*
   InterruptOCAOccured = false;
   InterruptOCBOccured = false;
   InterruptICOccured = false;
   EnabledOCBInterrupt = false;
   FlagA = false;
   FlagB = false;
   FlagC = false;
   */
   DetectedEdges = 0;
   /*
   for(uint8_t i = 0; i < 40; i++) {
      Samples[i].EdgeIndex = 0;
      Samples[i].CaptureTime = 0;
      Samples[i].RxState = 0;
      Samples[i].RxByte = 0;
      Samples[i].RxBit = 0;
   }
   */
   uint32_t cycles_per_bit = (F_CPU + un_baud_rate / 2) / un_baud_rate;

   if (cycles_per_bit < PRESCALE_THRESHOLD) {
      //FlagA = true;
      // CONFIG_TIMER_NOPRESCALE()
      //CONFIG_TIMER_NOPRESCALE()	(m_unInterruptMaskRegister = 0, m_unControlRegisterA = 0, m_unControlRegisterB = (1<<ICNC1) | (1<<CS10))
      m_unInterruptMaskRegister = 0;
      m_unControlRegisterA = 0;
      m_unControlRegisterB = _BV(ICNC1) | _BV(CS10);
   } 
   else {
      //FlagB = true;
      cycles_per_bit /= 8;
      if (cycles_per_bit < PRESCALE_THRESHOLD) {
         //FlagC = true;
         //CONFIG_TIMER_PRESCALE_8();
         // CONFIG_TIMER_PRESCALE_8()	(m_unInterruptMaskRegister = 0, m_unControlRegisterA = 0, m_unControlRegisterB = (1<<ICNC1) | (1<<CS11))
         m_unInterruptMaskRegister = 0;
         m_unControlRegisterA = 0;
         m_unControlRegisterB = _BV(ICNC1) | _BV(CS11);
      } else {
         return; // minimum 283 baud at 16 MHz clock
      }
   }
   m_unTicksPerBit = cycles_per_bit;
   m_unRxStopTicks = cycles_per_bit * 37 / 4;


   //#define INPUT_CAPTURE_PIN		 8 // receive
   //#define OUTPUT_COMPARE_A_PIN	 9 // transmit
   //#define OUTPUT_COMPARE_B_PIN	 10 // unusable PWM

   //pinMode(INPUT_CAPTURE_PIN, INPUT_PULLUP);
   //digitalWrite(OUTPUT_COMPARE_A_PIN, HIGH);
   //pinMode(OUTPUT_COMPARE_A_PIN, OUTPUT);

   /* Pull up the RX line (input) */
   DDRB &= ~_BV(DDB0);
   PORTB |= _BV(PORTB0);
   
   /* Set the TX line to drive high (output) */
   DDRB |= _BV(DDB1);
   PORTB |= _BV(PORTB1);

   /* Clear input capture interrupt flag and enable */
   //ENABLE_INT_INPUT_CAPTURE();
   //ENABLE_INT_INPUT_CAPTURE()	(m_unInterruptFlagRegister = (1<<ICF1), m_unInterruptMaskRegister = (1<<ICIE1))  
   m_unInterruptFlagRegister = _BV(ICF1);
   m_unInterruptMaskRegister = _BV(ICIE1);
}

CTUARTController::~CTUARTController() {
   //DISABLE_INT_COMPARE_B();
   //DISABLE_INT_COMPARE_B()	(m_unInterruptMaskRegister &= ~(1<<OCIE1B))
   m_unInterruptMaskRegister &= ~_BV(OCIE1B);
   
   //DISABLE_INT_INPUT_CAPTURE();
   //DISABLE_INT_INPUT_CAPTURE()	(m_unInterruptMaskRegister &= ~(1<<ICIE1))
   m_unInterruptMaskRegister &= ~_BV(ICIE1);
 
   FlushInput();
   
   FlushOutput();
   

   //DISABLE_INT_COMPARE_A();
   //DISABLE_INT_COMPARE_A()	(m_unInterruptMaskRegister &= ~(1<<OCIE1A))
   m_unInterruptMaskRegister &= ~_BV(OCIE1A);
   
   // TODO: restore timer to original settings?
}


/****************************************/
/**           Transmission             **/
/****************************************/

void CTUARTController::Write(uint8_t un_byte)
{
   uint8_t intr_state, head;

   head = m_sTxBuffer.Head + 1;
   if (head >= IO_BUFFER_SIZE) head = 0;
   while (m_sTxBuffer.Tail == head); // wait until space in buffer // os sleep here
   intr_state = SREG;
   cli();
   if (m_unTxState) {
      m_sTxBuffer.Buffer[head] = un_byte;
      m_sTxBuffer.Head = head;
   } 
   else {
      m_unTxState = 1;
      m_unTxByte = un_byte;
      m_unTxBit = 0;
      
      //ENABLE_INT_COMPARE_A();
      //ENABLE_INT_COMPARE_A()	(m_unInterruptFlagRegister = (1<<OCF1A), m_unInterruptMaskRegister |= (1<<OCIE1A))
      m_unInterruptFlagRegister = _BV(OCF1A);
      m_unInterruptMaskRegister |= _BV(OCIE1A);
      
      //CONFIG_MATCH_CLEAR();
      //CONFIG_MATCH_CLEAR()		(m_unControlRegisterA = (m_unControlRegisterA | (1<<COM1A1)) & ~(1<<COM1A0))
      m_unControlRegisterA = (m_unControlRegisterA | _BV(COM1A1)) & ~_BV(COM1A0);
      
      //SET_COMPARE_A(GET_TIMER_COUNT() + 16);
      //SET_COMPARE_A(m_unCountRegister + 16);
      //SET_COMPARE_A(val)		(OCR1A = (val))
      /* start transmission in 16 ticks from now */
      m_unOutputCompareRegisterA = m_unCountRegister + 16;  
   }
   SREG = intr_state;
}

void CTUARTController::FlushOutput() {
   while (m_unTxState); /* wait */ // os sleep
}

uint8_t CTUARTController::Read(void) {
   uint8_t unData = -1;
   uint8_t unIntrState = SREG;
   cli();
   if(m_sRxBuffer.Tail != m_sRxBuffer.Head) {
      ++m_sRxBuffer.Tail;
      m_sRxBuffer.Tail %= IO_BUFFER_SIZE;
      unData = m_sRxBuffer.Buffer[m_sRxBuffer.Tail];
   }
   SREG = unIntrState;
   return unData;
}

uint8_t CTUARTController::Peek(void) {
   uint8_t head, tail;

   head = m_sRxBuffer.Head;
   tail = m_sRxBuffer.Tail;
   if (head == tail) return -1;
   return m_sRxBuffer.Buffer[tail];
}

bool CTUARTController::Available(void) {
   uint8_t unIntrState = SREG;
   cli();
   bool bHasRxData = (m_sRxBuffer.Head != m_sRxBuffer.Tail);
   SREG = unIntrState;
   return bHasRxData;
}

void CTUARTController::FlushInput(void) {
   m_sRxBuffer.Head = m_sRxBuffer.Tail;
}



