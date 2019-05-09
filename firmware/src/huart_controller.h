
#ifndef HUART_CONTROLLER_H
#define HUART_CONTROLLER_H

#include "interrupt.h"

#define HUART_RX_BUFFER_SIZE 32
#define HUART_TX_BUFFER_SIZE 32

/***********************************************************/
/***********************************************************/

class CHUARTController {

public:

   static CHUARTController& GetInstance() {
      static CHUARTController cInstance;
      return cInstance;
   }

   bool HasData() {
      return m_cReceiveInterrupt.HasData();
   }

   uint8_t Read() {
      return m_cReceiveInterrupt.Read();
   }

   void Write(uint8_t data) {
      m_cTransmitInterrupt.Write(data);
   }

   void Print(const char* pch_format, ...);

private:

   CHUARTController();

   /***********************************************************/
   /***********************************************************/

   class CTransmitInterrupt : public CInterrupt {

   public:

      CTransmitInterrupt(CHUARTController* pc_huart_controller);

      void Write(uint8_t data);

   private:
      void ServiceRoutine();

      CHUARTController* m_pcHUARTController;

      volatile uint8_t m_unHead;
      volatile uint8_t m_unTail;
      volatile uint8_t m_punBuf[HUART_TX_BUFFER_SIZE];
   } m_cTransmitInterrupt;

   /***********************************************************/
   /***********************************************************/

   class CReceiveInterrupt : public CInterrupt {

   public:

      CReceiveInterrupt(CHUARTController* pc_huart_controller);

      bool HasData();

      uint8_t Read();

   private:
      void ServiceRoutine();

      CHUARTController* m_pcHUARTController;

      volatile uint8_t m_unHead;
      volatile uint8_t m_unTail;
      volatile uint8_t m_punBuf[HUART_RX_BUFFER_SIZE];
   } m_cReceiveInterrupt;

   /***********************************************************/
   /***********************************************************/

};

#endif

