
#include <avr/io.h>
#include "uart-ref.h"

#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1)
#define UART_TX_BUFFER_MASK ( UART_TX_BUFFER_SIZE - 1)


CHUARTController::CTransmitInterrupt::CTransmitInterrupt(CHUARTController* pc_huart_controller) :
   m_pcHUARTController(pc_huart_controller),
   m_unHead(0u),
   m_unTail(0u) {
   Register(this, USART_UDRE_vect_num);
}

void CHUARTController::CTransmitInterrupt::ServiceRoutine() {
    unsigned char tmptail;
    
    if ( m_unHead != m_unTail) {
        /* calculate and store new buffer index */
        tmptail = (m_unTail + 1) & UART_TX_BUFFER_MASK;
        m_unTail = tmptail;
        /* get one byte from buffer and write it to UART */
        UDR0 = m_punBuf[tmptail];  /* start transmission */
    }else{
        /* tx buffer empty, disable UDRE interrupt */
        UCSR0B &= ~_BV(UDRIE0);
    }

}

CHUARTController::CReceiveInterrupt::CReceiveInterrupt(CHUARTController* pc_huart_controller) :
   m_pcHUARTController(pc_huart_controller),
   m_unHead(0u),
   m_unTail(0u) {
   Register(this, USART_RX_vect_num);
}

void CHUARTController::CReceiveInterrupt::ServiceRoutine() {
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;
 
     /* read UART status register and UART data register */
    usr  = UCSR0A;
    data = UDR0;
    
    /* get FEn (Frame Error) DORn (Data OverRun) UPEn (USART Parity Error) bits */
    lastRxError = usr & ( _BV(FE0) | _BV(DOR0) | _BV(UPE0) );

    /* calculate buffer index */ 
    tmphead = ( m_unHead + 1) & UART_RX_BUFFER_MASK;
    
    if ( tmphead == m_unTail ) {
        /* error: receive buffer overflow */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        /* store new index */
        m_unHead = tmphead;
        /* store received data in buffer */
        m_punBuf[tmphead] = data;
    }
    m_unLastError |= lastRxError;
}


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
    UBRR0H = (unsigned char)((baudrate>>8)&0x80) ;
    UBRR0L = (unsigned char) (baudrate&0x00FF);
      
    /* Enable USART receiver and transmitter and receive complete interrupt */
    UCSR0B = _BV(RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);

}/* uart_init */


/*************************************************************************
Function: uart_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int CHUARTController::CReceiveInterrupt::Read()
{    
    unsigned char tmptail;
    unsigned char data;
    unsigned char lastRxError;



    if ( m_unHead == m_unTail ) {
        return UART_NO_DATA;   /* no data available */
    }
    
    /* calculate buffer index */
    tmptail = (m_unTail + 1) & UART_RX_BUFFER_MASK;
    
    /* get data from receive buffer */
    data = m_punBuf[tmptail];
    lastRxError = m_unLastError;
    
    /* store buffer index */
    m_unTail = tmptail; 
    
    m_unLastError = 0;
    return (lastRxError << 8) + data;

}/* uart_getc */


/*************************************************************************
Function: uart_putc()
Purpose:  write byte to ringbuffer for transmitting via UART
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void CHUARTController::CTransmitInterrupt::Write(unsigned char data)
{
    unsigned char tmphead;

    
    tmphead  = (m_unHead + 1) & UART_TX_BUFFER_MASK;
    
    while ( tmphead == m_unTail ){
        ;/* wait for free space in buffer */
    }
    
    m_punBuf[tmphead] = data;
    m_unHead = tmphead;

    /* enable UDRE interrupt */
    UCSR0B    |= _BV(UDRIE0);

}/* uart_putc */

