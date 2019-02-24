
#include <avr/io.h>
#include "huart_controller.h"

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
    uint8_t tmptail;
    
    if ( m_unHead != m_unTail) {
        /* calculate and store new buffer index */
        tmptail = (m_unTail + 1) & HUART_TX_BUFFER_MASK;
        m_unTail = tmptail;
        /* get one byte from buffer and write it to UART */
        UDR0 = m_punBuf[tmptail];  /* start transmission */
    }else{
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
    uint8_t tmphead;
    uint8_t data;
  
    /* read UART status register and UART data register */
    data = UDR0;

    /* calculate buffer index */ 
    tmphead = ( m_unHead + 1) & HUART_RX_BUFFER_MASK;
    
    if ( tmphead == m_unTail ) {
        /* error: receive buffer overflow */
    }else{
        /* store new index */
        m_unHead = tmphead;
        /* store received data in buffer */
        m_punBuf[tmphead] = data;
    }
}

/***********************************************************/
/***********************************************************/

#define UART_BAUD_SELECT(baudRate,xtalCpu)  (((xtalCpu) + 8UL * (baudRate)) / (16UL * (baudRate)) -1UL)

CHUARTController::CHUARTController() :
   m_cTransmitInterrupt(this),
   m_cReceiveInterrupt(this) {

    unsigned int baudrate = UART_BAUD_SELECT(57600, F_CPU);

    /* Set baud rate */
    if ( baudrate & 0x8000 )
    {
        UCSR0A = (1<<U2X0);  //Enable 2x speed 
    } 
    UBRR0H = (uint8_t)((baudrate>>8)&0x80) ;
    UBRR0L = (uint8_t) (baudrate&0x00FF);
      
    /* Enable USART receiver and transmitter and receive complete interrupt */
    UCSR0B = _BV(RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

/***********************************************************/
/***********************************************************/

uint8_t CHUARTController::CReceiveInterrupt::Read() {    
    uint8_t tmptail;
    uint8_t data;

    if ( m_unHead == m_unTail ) {
        return -1;   /* no data available */
    }
    
    /* calculate buffer index */
    tmptail = (m_unTail + 1) & HUART_RX_BUFFER_MASK;
    
    /* get data from receive buffer */
    data = m_punBuf[tmptail];
    
    /* store buffer index */
    m_unTail = tmptail; 
    
    return data;
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

void CHUARTController::CTransmitInterrupt::Write(uint8_t data)
{
    uint8_t tmphead;

    
    tmphead  = (m_unHead + 1) & HUART_TX_BUFFER_MASK;
    
    while ( tmphead == m_unTail ){
        ;/* wait for free space in buffer */
    }
    
    m_punBuf[tmphead] = data;
    m_unHead = tmphead;

    /* enable UDRE interrupt */
    UCSR0B    |= _BV(UDRIE0);

}

/***********************************************************/
/***********************************************************/


