
#ifndef TUART_CONTROLLER_H
#define TUART_CONTROLLER_H

#include <inttypes.h>

#define IO_BUFFER_SIZE 64

class CTUARTController //: public Stream
{
public:
   CTUARTController();
   ~CTUARTController();

   void WriteByte(uint8_t byte);

   uint8_t Peek();
   uint8_t Read();
   bool Available();
   void Flush() { FlushOutput(); }

   void FlushInput();
   void FlushOutput();

private:

   /* Registers */   	
   volatile uint8_t& m_unControlRegisterA;
   volatile uint8_t& m_unControlRegisterB;
   volatile uint8_t& m_unInterruptMaskRegister;
   volatile uint8_t& m_unInterruptFlagRegister;
   volatile uint8_t& m_unInputCaptureRegister;
   volatile uint8_t& m_unOutputCompareRegisterA;
   volatile uint8_t& m_unOutputCompareRegisterB;
   volatile uint8_t& m_unCountRegister;

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
