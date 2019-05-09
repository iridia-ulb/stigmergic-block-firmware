
#include "huart_controller.h"

/***********************************************************/
/***********************************************************/

#include <avr/io.h>

/***********************************************************/
/***********************************************************/

#define HUART_RX_BUFFER_MASK ( HUART_RX_BUFFER_SIZE - 1)
#define HUART_TX_BUFFER_MASK ( HUART_TX_BUFFER_SIZE - 1)

/***********************************************************/
/***********************************************************/

CHUARTController::CTransmitInterrupt::CTransmitInterrupt(CHUARTController* pc_huart_controller) :
   m_pcHUARTController(pc_huart_controller),
   m_unHead(0u),
   m_unTail(0u) {
   Register(this, USART_UDRE_vect_num);
}

/***********************************************************/
/***********************************************************/

void CHUARTController::CTransmitInterrupt::ServiceRoutine() {
   uint8_t unTempTail;
   if(m_unHead != m_unTail) {
      /* calculate and store new buffer index */
      unTempTail = (m_unTail + 1) & HUART_TX_BUFFER_MASK;
      m_unTail = unTempTail;
      /* get one byte from buffer and write it to UART */
      UDR0 = m_punBuf[unTempTail];  /* start transmission */
   } else {
      /* tx buffer empty, disable UDRE interrupt */
      UCSR0B &= ~_BV(UDRIE0);
   }
}

/***********************************************************/
/***********************************************************/

CHUARTController::CReceiveInterrupt::CReceiveInterrupt(CHUARTController* pc_huart_controller) :
   m_pcHUARTController(pc_huart_controller),
   m_unHead(0u),
   m_unTail(0u) {
   Register(this, USART_RX_vect_num);
}

/***********************************************************/
/***********************************************************/

void CHUARTController::CReceiveInterrupt::ServiceRoutine() {
   uint8_t unTempHead;
   uint8_t unData;
   /* read UART status register and UART data register */
   unData = UDR0;
   /* calculate buffer index */
   unTempHead = (m_unHead + 1) & HUART_RX_BUFFER_MASK;
   if(unTempHead == m_unTail) {
      /* error: receive buffer overflow */
   } else {
      /* store new index */
      m_unHead = unTempHead;
      /* store received data in buffer */
      m_punBuf[unTempHead] = unData;
   }
}

/***********************************************************/
/***********************************************************/

#define UART_BAUD_SELECT(baudRate,xtalCpu)  (((xtalCpu) + 8UL * (baudRate)) / (16UL * (baudRate)) -1UL)

CHUARTController::CHUARTController() :
   m_cTransmitInterrupt(this),
   m_cReceiveInterrupt(this) {
   unsigned int unBaudrate = UART_BAUD_SELECT(57600, F_CPU);
   /* Set baud rate */
   if(unBaudrate & 0x8000) {
      UCSR0A = (1 << U2X0);  //Enable 2x speed
   }
   UBRR0H = (uint8_t)((unBaudrate >> 8) & 0x80) ;
   UBRR0L = (uint8_t) (unBaudrate & 0x00FF);
   /* Enable USART receiver and transmitter and receive complete interrupt */
   UCSR0B = _BV(RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
   /* Set frame format: asynchronous, 8data, no parity, 1stop bit */
   UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/***********************************************************/
/***********************************************************/

uint8_t CHUARTController::CReceiveInterrupt::Read() {
   uint8_t unTempTail;
   uint8_t unData;
   if (m_unHead == m_unTail) {
      return -1;   /* no data available */
   }
   /* calculate buffer index */
   unTempTail = (m_unTail + 1) & HUART_RX_BUFFER_MASK;
   /* get data from receive buffer */
   unData = m_punBuf[unTempTail];
   /* store buffer index */
   m_unTail = unTempTail;
   return unData;
}

/***********************************************************/
/***********************************************************/

bool CHUARTController::CReceiveInterrupt::HasData() {
   CInterruptController::GetInstance().Disable();
   bool bHasData = (m_unHead != m_unTail);
   CInterruptController::GetInstance().Enable();
   return bHasData;
}

/***********************************************************/
/***********************************************************/

void CHUARTController::CTransmitInterrupt::Write(uint8_t un_data) {
   uint8_t unTempHead;
   unTempHead  = (m_unHead + 1) & HUART_TX_BUFFER_MASK;
   while (unTempHead == m_unTail); /* wait for free space in buffer */
   m_punBuf[unTempHead] = un_data;
   m_unHead = unTempHead;
   /* enable UDRE interrupt */
   UCSR0B |= _BV(UDRIE0);
}

/***********************************************************/
/***********************************************************/

#include <stdio.h>

void CHUARTController::Print(const char* pch_format, ...) {
   /* set up the output (once) */
   static FILE* psOutput = fdevopen([] (char c_to_write, FILE* pf_stream) {
      CHUARTController::GetInstance().Write(c_to_write);
      return 0;
   }, nullptr);
   /* print */
   va_list tArguments;
   va_start(tArguments, pch_format);
   vfprintf(psOutput, pch_format, tArguments);
   va_end(tArguments);
}

/***********************************************************/
/***********************************************************/

