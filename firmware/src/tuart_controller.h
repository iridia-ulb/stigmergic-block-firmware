
#ifndef TUART_CONTROLLER_H
#define TUART_CONTROLLER_H

#include <stdint.h>

#include "interrupt.h"

#define IO_BUFFER_SIZE 64

class CTUARTController //: public Stream
{
public:
   CTUARTController(uint32_t un_baud_rate,
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
                    uint8_t un_output_comp_b_intr_num);
   ~CTUARTController();

   void Write(uint8_t byte);

   uint8_t Peek();
   uint8_t Read();
   bool Available();
   void Flush() { FlushOutput(); }

   void FlushInput();
   void FlushOutput();

public:

   /* Registers */   	
   volatile uint8_t& m_unControlRegisterA;
   volatile uint8_t& m_unControlRegisterB;
   volatile uint8_t& m_unInterruptMaskRegister;
   volatile uint8_t& m_unInterruptFlagRegister;
   volatile uint16_t& m_unInputCaptureRegister;
   volatile uint16_t& m_unOutputCompareRegisterA;
   volatile uint16_t& m_unOutputCompareRegisterB;
   volatile uint16_t& m_unCountRegister;

   volatile uint8_t  m_unRxState;
   volatile uint8_t  m_unTxState;
   uint16_t          m_unTicksPerBit;
   bool              m_bTimingError;
   uint16_t          m_unRxTarget;
   uint16_t          m_unRxStopTicks;
   uint8_t           m_unTxByte;
   uint8_t           m_unRxByte;
   uint8_t           m_unTxBit;
   uint8_t           m_unRxBit;

   /* DEBUG */

   /*
   bool InterruptOCAOccured;
   bool InterruptOCBOccured;
   bool InterruptICOccured;
   bool EnabledOCBInterrupt;
   bool FlagA;
   bool FlagB;
   bool FlagC;
   */

   uint16_t DetectedEdges;
   //uint16_t CaptureTimes[200];
   //uint8_t  TrackRxBit[90];
   //uint8_t  TrackRxByte[90];
   //uint8_t  StateTotal;

   /*

   struct Event {
      uint8_t EdgeIndex;
      uint16_t CaptureTime;
      uint8_t RxState;
      uint8_t RxByte;
      uint8_t RxBit;
   } Samples[40];

   */ 

   /* DEBUG */

   /* IO Buffers */
   volatile struct SRingBuffer {
      SRingBuffer() {
         Head = 0;
         Tail = 0;
      }
      uint8_t Head;
      uint8_t Tail;
      uint8_t Buffer[IO_BUFFER_SIZE];
   } m_sRxBuffer, m_sTxBuffer;

   /* Input capture interrupt for Rx */
   class CInputCaptureInterrupt : public CInterrupt {
   private:
      CTUARTController* m_pcTUARTController;
      void ServiceRoutine();
   public:
      CInputCaptureInterrupt(CTUARTController* pc_tuart_controller, uint8_t un_intr_vect_num);
   } m_cInputCaptureInterrupt;
   friend CInputCaptureInterrupt;

   /* Output compare A interrupt for Tx */
   class COutputCompareAInterrupt : public CInterrupt {
   private:
      CTUARTController* m_pcTUARTController;
      void ServiceRoutine();
   public:
      COutputCompareAInterrupt(CTUARTController* pc_tuart_controller, uint8_t un_intr_vect_num);
   } m_cOutputCompareAInterrupt;
   friend COutputCompareAInterrupt;

   /* Output compare B interrupt for Tx */
   class COutputCompareBInterrupt : public CInterrupt {
   private:
      CTUARTController* m_pcTUARTController;
      void ServiceRoutine();
   public:
      COutputCompareBInterrupt(CTUARTController* pc_tuart_controller, uint8_t un_intr_vect_num);
   } m_cOutputCompareBInterrupt;
   friend COutputCompareBInterrupt;

};

#endif
