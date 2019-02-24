#ifndef HUART_CONTROLLER_H
#define HUART_CONTROLLER_H

#include "interrupt.h"

#define UART_RX_BUFFER_SIZE 32
#define UART_TX_BUFFER_SIZE 32

/* 
** high byte error return code of uart_getc()
*/
#define UART_FRAME_ERROR      0x1000              /**< @brief Framing Error by UART       */
#define UART_OVERRUN_ERROR    0x0800              /**< @brief Overrun condition by UART   */
#define UART_PARITY_ERROR     0x0400              /**< @brief Parity Error by UART        */ 
#define UART_BUFFER_OVERFLOW  0x0200              /**< @brief receive ringbuffer overflow */
#define UART_NO_DATA          0x0100              /**< @brief no receive data available   */

class CHUARTController {

public:

   static CHUARTController& GetInstance() {
      static CHUARTController cInstance;
      return cInstance;
   }

/**
 *  @return  lower byte:  received byte from ringbuffer
 *  @return  higher byte: last receive status
 *           - \b 0 successfully received data from UART
 *           - \b UART_NO_DATA           
 *             <br>no receive data available
 *           - \b UART_BUFFER_OVERFLOW   
 *             <br>Receive ringbuffer overflow.
 *             We are not reading the receive buffer fast enough, 
 *             one or more received character have been dropped 
 *           - \b UART_OVERRUN_ERROR     
 *             <br>Overrun condition by UART.
 *             A character already present in the UART UDR register was 
 *             not read by the interrupt handler before the next character arrived,
 *             one or more received characters have been dropped.
 *           - \b UART_FRAME_ERROR       
 *             <br>Framing Error by UART
 */

   unsigned int uart_getc() {
      return m_cReceiveInterrupt.Read();
   }

   void uart_putc(unsigned char data) {
      m_cTransmitInterrupt.Write(data);
   }

private:

   class CTransmitInterrupt : public CInterrupt {
   private:
      CHUARTController* m_pcHUARTController;
      volatile unsigned char m_unHead;
      volatile unsigned char m_unTail;
      volatile unsigned char m_punBuf[UART_TX_BUFFER_SIZE];

      void ServiceRoutine();
   public:
      CTransmitInterrupt(CHUARTController* pc_huart_controller);

      void Write(unsigned char data);

   } m_cTransmitInterrupt;

   class CReceiveInterrupt : public CInterrupt {
   private:
      CHUARTController* m_pcHUARTController;
      volatile unsigned char m_unHead;
      volatile unsigned char m_unTail;
      volatile unsigned char m_unLastError;
      volatile unsigned char m_punBuf[UART_RX_BUFFER_SIZE];

      void ServiceRoutine();
   public:
      CReceiveInterrupt(CHUARTController* pc_huart_controller);

      unsigned int Read();

   } m_cReceiveInterrupt;

   CHUARTController();

};

#endif

