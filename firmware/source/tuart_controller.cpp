

#include "tuart_controller.h"
/*
#define ALTSS_USE_TIMER1
#define INPUT_CAPTURE_PIN		 8 // receive
#define OUTPUT_COMPARE_A_PIN	 9 // transmit
#define OUTPUT_COMPARE_B_PIN	 10 // unusable PWM

#define CONFIG_TIMER_NOPRESCALE()	(m_unInterruptMaskRegister = 0, m_unControlRegisterA = 0, m_unControlRegisterB = (1<<ICNC1) | (1<<CS10))
#define CONFIG_TIMER_PRESCALE_8()	(m_unInterruptMaskRegister = 0, m_unControlRegisterA = 0, m_unControlRegisterB = (1<<ICNC1) | (1<<CS11))
#define CONFIG_MATCH_NORMAL()		(m_unControlRegisterA = m_unControlRegisterA & ~((1<<COM1A1) | (1<<COM1A0)))
#define CONFIG_MATCH_TOGGLE()		(m_unControlRegisterA = (m_unControlRegisterA & ~(1<<COM1A1)) | (1<<COM1A0))
#define CONFIG_MATCH_CLEAR()		(m_unControlRegisterA = (m_unControlRegisterA | (1<<COM1A1)) & ~(1<<COM1A0))
#define CONFIG_MATCH_SET()		(m_unControlRegisterA = m_unControlRegisterA | ((1<<COM1A1) | (1<<COM1A0)))
#define CONFIG_CAPTURE_FALLING_EDGE()	(m_unControlRegisterB &= ~(1<<ICES1))
#define CONFIG_CAPTURE_RISING_EDGE()	(m_unControlRegisterB |= (1<<ICES1))
#define ENABLE_INT_INPUT_CAPTURE()	(m_unInterruptFlagRegister = (1<<ICF1), m_unInterruptMaskRegister = (1<<ICIE1))
#define ENABLE_INT_COMPARE_A()	(m_unInterruptFlagRegister = (1<<OCF1A), m_unInterruptMaskRegister |= (1<<OCIE1A))
#define ENABLE_INT_COMPARE_B()	(m_unInterruptFlagRegister = (1<<OCF1B), m_unInterruptMaskRegister |= (1<<OCIE1B))
#define DISABLE_INT_INPUT_CAPTURE()	(m_unInterruptMaskRegister &= ~(1<<ICIE1))
#define DISABLE_INT_COMPARE_A()	(m_unInterruptMaskRegister &= ~(1<<OCIE1A))
#define DISABLE_INT_COMPARE_B()	(m_unInterruptMaskRegister &= ~(1<<OCIE1B))
#define GET_TIMER_COUNT()		(m_unCountRegister)
#define GET_INPUT_CAPTURE()		(ICR1)
#define GET_COMPARE_A()		(OCR1A)
#define GET_COMPARE_B()		(OCR1B)
#define SET_COMPARE_A(val)		(OCR1A = (val))
#define SET_COMPARE_B(val)		(OCR1B = (val))
#define CAPTURE_INTERRUPT		TIMER1_CAPT_vect
#define COMPARE_A_INTERRUPT		TIMER1_COMPA_vect
#define COMPARE_B_INTERRUPT		TIMER1_COMPB_vect


#ifndef INPUT_PULLUP
#define INPUT_PULLUP INPUT
#endif
*/
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
   int32_t offset; // int16_t 

   //capture = GET_INPUT_CAPTURE();
   capture = m_pcTUARTController->m_ununInputCaptureRegister;
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
   state = m_unRxState;
   if (state == 0) {
      if (!bit) {
         //SET_COMPARE_B(capture + rx_stop_ticks);
         // (OCR1B = (val))
         m_pcTUARTController->m_unOutputCompareRegisterB = capture + m_pcTUARTController->m_unRxStopTicks;
         //ENABLE_INT_COMPARE_B();
         // (TIFR1 = (1<<OCF1B), TIMSK1 |= (1<<OCIE1B))
         m_pcTUARTController->m_unInterruptFlagRegister = _BV(OCF1B);
         m_pcTUARTController->m_unInterruptMaskRegister |= _BV(OCIE1B); 
         m_pcTUARTController->m_unRxTarget = capture + m_pcTUARTController->m_unTicksPerBit + m_pcTUARTController->m_unTicksPerBit/2;
         m_pcTUARTController->m_unRxState = 1;
      }
   }
   else {
      target = m_pcTUARTController->m_unRxTarget;
      while (1) {
         offset = capture - target;
         if (offset < 0) break;
         m_pcTUARTController->m_unRxByte = (m_pcTUARTController->m_unRxByte >> 1) | m_pcTUARTController->m_unRxBit;
         target += m_pcTUARTController->m_unTicksPerBit;
         state++;
         if (state >= 9) {
            //DISABLE_INT_COMPARE_B();
            // (TIMSK1 &= ~(1<<OCIE1B))
            m_pcTUARTController->m_unInterruptMaskRegister &= ~_BV(OCIE1B);
            
            head = m_pcTUARTController->m_sRxBuffer.Head + 1;
            if (head >= IO_BUFFER_SIZE) head = 0;
            if (head != m_pcTUARTController->m_sRxBuffer.Tail) {
               m_pcTUARTController->m_sRxBuffer.Buffer[head] = m_unRxByte;
               m_pcTUARTController->m_sRxBuffer.Head = head;
            }
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
            m_pcTUARTController->m_unControlRegisterA = m_pcTUARTController->m_unControlRegisterA | _BV(COM1A1) | _BV(COM1A0);

         } else {
            //CONFIG_MATCH_CLEAR();
            //CONFIG_MATCH_CLEAR() (m_unControlRegisterA = (m_unControlRegisterA | (1<<COM1A1)) & ~(1<<COM1A0))
            m_pcTUARTController->m_unControlRegisterA = m_pcTUARTController->m_unControlRegisterA | _BV(COM1A1) & ~_BV(COM1A0);
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
      m_pcTUARTController->m_unControlRegisterA = m_pcTUARTController->m_unControlRegisterA | _BV(COM1A1) | _BV(COM1A0);
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
      m_pcTUARTController->m_unControlRegisterA = m_pcTUARTController->m_unControlRegisterA & ~_BV(COM1A1) | _BV(COM1A0);
      
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
      m_pcTUARTController->m_unControlRegisterA = m_pcTUARTController->m_unControlRegisterA | _BV(COM1A1) & ~_BV(COM1A0);

      //SET_COMPARE_A(target + ticks_per_bit);
      //SET_COMPARE_A(val)		(OCR1A = (val))
      m_pcTUARTController->m_unOutputCompareRegisterA = target + m_pcTUARTController->m_unTicksPerBit; 

      // TODO: how to detect timing_error?
   }
}

void CTUARTController::COutputCompareBInterrupt::ServiceRoutine() {
   uint8_t head, state, bit;

   //DISABLE_INT_COMPARE_B();
   //DISABLE_INT_COMPARE_B()	(m_unInterruptMaskRegister &= ~(1<<OCIE1B))
   m_pcTUARTController->m_unInterruptMaskRegister &= ~_BV(OCIE1B);

   //CONFIG_CAPTURE_FALLING_EDGE();
   //CONFIG_CAPTURE_FALLING_EDGE()	(m_unControlRegisterB &= ~(1<<ICES1))
   m_pcTUARTController->m_unControlRegisterB &= ~_BV(ICES1);

   state = m_pcTUARTController->m_unRxState;
   bit = m_pcTUARTController->m_unRxBit ^ 0x80;
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
   m_bTimingError(0),
   m_unRxTarget(0),
   m_unRxStopTicks(0),
   m_unTxByte(0),
   m_unRxByte(0),
   m_unTxBit(0),
   m_unRxBit(0),
   m_cInputCaptureInterrupt(this, un_input_capt_intr_num),
   m_cOutputCompareAInterrupt(this, un_output_comp_a_intr_num),
   m_cOutputCompareBInterrupt(this, un_output_comp_b_intr_num) {

   uint32_t cycles_per_bit = (F_CPU + un_baud_rate / 2) / un_baud_rate;

   if (cycles_per_bit < PRESCALE_THRESHOLD) {
      m_unInterruptMaskRegister = 0;
      m_unControlRegisterA = 0;
      m_unControlRegisterB = _BV(ICNC1) | _BV(CS10);
   } else {
      cycles_per_bit /= 8;
      if (cycles_per_bit < PRESCALE_THRESHOLD) {
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

void CTUARTController::~CTUARTController()
{
   //DISABLE_INT_COMPARE_B();
   //DISABLE_INT_COMPARE_B()	(m_unInterruptMaskRegister &= ~(1<<OCIE1B))
   m_unInterruptMaskRegister &= ~_BV(OCIE1B);
   
   //DISABLE_INT_INPUT_CAPTURE();
   //DISABLE_INT_INPUT_CAPTURE()	(m_unInterruptMaskRegister &= ~(1<<ICIE1))
   m_unInterruptMaskRegister &= ~_BV(ICIE1)
 
   FlushInput();
   
   FlushOutput();
   

   //DISABLE_INT_COMPARE_A();
   //DISABLE_INT_COMPARE_A()	(m_unInterruptMaskRegister &= ~(1<<OCIE1A))
   m_unInterruptMaskRegister &= ~_BV(OCIE1A)
   
   // TODO: restore timer to original settings?
}


/****************************************/
/**           Transmission             **/
/****************************************/

void CTUARTController::WriteByte(uint8_t un_byte)
{
   uint8_t intr_state, head;

   head = tx_buffer_head + 1;
   if (head >= IO_BUFFER_SIZE) head = 0;
   while (m_sTxBuffer.Tail == m_sTxBuffer.Head); // wait until space in buffer // os sleep here
   intr_state = SREG;
   cli();
   if (m_unTxState) {
      m_sTxBuffer.Buffer[m_sTxBuffer.Head] = un_byte;
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
      m_unControlRegisterA = (m_unControlRegisterA | _BV(COM1A1) & ~_BV(COM1A0));
      
      //SET_COMPARE_A(GET_TIMER_COUNT() + 16);
      //SET_COMPARE_A(m_unCountRegister + 16);
      //SET_COMPARE_A(val)		(OCR1A = (val))
      m_unOutputCompareRegisterA = m_unCountRegister + 16;  
   }
   SREG = intr_state;
}

void AltSoftSerial::FlushOutput(void)
{
   while (m_unTxState); /* wait */ // os sleep
}

int AltSoftSerial::Read(void)
{
   uint8_t head, tail, out;

   head = m_sRxBuffer.Head;
   tail = m_sRxBuffer.Tail;
   if (head == tail) return -1;
   if (++tail >= IO_BUFFER_SIZE) tail = 0;
   out = m_sRxBuffer.Buffer[tail];
   m_sRxBuffer.Tail = tail;
   return out;
}

int CTUARTController::Peek(void) {
   uint8_t head, tail;

   head = m_sRxBuffer.Head;
   tail = m_sRxBuffer.Tail;
   if (head == tail) return -1;
   return m_sRxBuffer.Buffer[tail];
}

bool CTUARTController::Available(void) {
   return (m_sRxBuffer.Head != m_sRxBuffer.Tail);
}

void CTUARTController::flushInput(void) {
   m_sRxBuffer.Head = m_sRxBuffer.Tail;
}



