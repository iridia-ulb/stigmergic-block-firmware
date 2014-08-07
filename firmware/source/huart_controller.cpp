

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <avr/interrupt.h>

#include "huart_controller.h"

// Singleton Instance /////////////////////////////////////////////////////////////////////////
HardwareSerial HardwareSerial::_hardware_serial;

// Interrupt Routines and Data ////////////////////////////////////////////////////////////////

// Define constants and variables for buffering incoming serial data.  We're
// using a ring buffer (I think), in which head is the index of the location
// to which to write the next incoming character and tail is the index of the
// location from which to read.
#define SERIAL_BUFFER_SIZE 64

struct ring_buffer
{
  unsigned char buffer[SERIAL_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
};

ring_buffer rx_buffer  =  { { 0 }, 0, 0 };
ring_buffer tx_buffer  =  { { 0 }, 0, 0 };

/****************************************/
/****************************************/

/* receive interrupt */
ISR(USART_RX_vect)
{
   if (bit_is_clear(UCSR0A, UPE0)) {
      unsigned int i = (rx_buffer.head + 1) % SERIAL_BUFFER_SIZE;
      if (i != rx_buffer.tail) {
         rx_buffer.buffer[rx_buffer.head] = UDR0;
         rx_buffer.head = i;
      }
   } 
   else {
      unsigned char c = UDR0;
   };
}

/****************************************/
/****************************************/

/* transmit interrupt */
ISR(USART_UDRE_vect)
{
   if (tx_buffer.head == tx_buffer.tail) {
      // Buffer empty, so disable interrupts

      //cbi(UCSR0B, UDRIE0);
      UCSR0B &= ~(_BV(UDRIE0));
   }
   else {
      // There is more data in the output buffer. Send the next byte
      unsigned char c = tx_buffer.buffer[tx_buffer.tail];
      tx_buffer.tail = (tx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
      UDR0 = c;
   }
}


// Constructors ////////////////////////////////////////////////////////////////

HardwareSerial::HardwareSerial() {
   _rx_buffer = &rx_buffer;
   _tx_buffer = &tx_buffer;
   _ubrrh = &UBRR0H;
   _ubrrl = &UBRR0L;
   _ucsra = &UCSR0A;
   _ucsrb = &UCSR0B;
   _ucsrc = &UCSR0C;
   _udr = &UDR0;
   _rxen = RXEN0;
   _txen = TXEN0;
   _rxcie = RXCIE0;
   _udrie = UDRIE0;
   _u2x = U2X0;

   /* Disconnect UART (reconnected by begin()) */
   *_ucsrb = 0;

   /* start up serial */
   Begin(57600);
}

// Public Methods //////////////////////////////////////////////////////////////

void HardwareSerial::Begin(unsigned long baud)
{
  uint16_t baud_setting;
  bool use_u2x = true;

try_again:
  
  if (use_u2x) {
    *_ucsra = 1 << _u2x;
    baud_setting = (F_CPU / 4 / baud - 1) / 2;
  } else {
    *_ucsra = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }
  
  if ((baud_setting > 4095) && use_u2x)
  {
    use_u2x = false;
    goto try_again;
  }

  // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
  *_ubrrh = baud_setting >> 8;
  *_ubrrl = baud_setting;

  transmitting = false;

  //sbi(*_ucsrb, _rxen);
  //sbi(*_ucsrb, _txen);
  //sbi(*_ucsrb, _rxcie);
  *_ucsrb |= _BV(_rxen) | _BV(_txen) | _BV(_rxcie);

  //cbi(*_ucsrb, _udrie);
  *_ucsrb &= ~(_BV(_udrie));
}

/****************************************/
/****************************************/

void HardwareSerial::End()
{
  // wait for transmission of outgoing data
  while (_tx_buffer->head != _tx_buffer->tail);

  //cbi(*_ucsrb, _rxen);
  //cbi(*_ucsrb, _txen);
  //cbi(*_ucsrb, _rxcie);  
  //cbi(*_ucsrb, _udrie);

  *_ucsrb &= ~(_BV(_rxen) | _BV(_txen) | _BV(_rxcie) | _BV(_udrie));
  
  // clear any received data
  _rx_buffer->head = _rx_buffer->tail;
}

/****************************************/
/****************************************/

int HardwareSerial::Available(void)
{
  return (int)(SERIAL_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % SERIAL_BUFFER_SIZE;
}

/****************************************/
/****************************************/

int HardwareSerial::Peek(void)
{
  if (_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    return _rx_buffer->buffer[_rx_buffer->tail];
  }
}

/****************************************/
/****************************************/

int HardwareSerial::Read(void)
{
  // if the head isn't ahead of the tail, we don't have any characters
  if (_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    unsigned char c = _rx_buffer->buffer[_rx_buffer->tail];
    _rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
    return c;
  }
}

/****************************************/
/****************************************/

void HardwareSerial::Flush() {
  // UDR is kept full while the buffer is not empty, so TXC triggers when EMPTY && SENT
  while (transmitting && ! (*_ucsra & _BV(TXC0)));
  transmitting = false;
}

/****************************************/
/****************************************/

uint8_t HardwareSerial::Write(uint8_t c) {
  unsigned int i = (_tx_buffer->head + 1) % SERIAL_BUFFER_SIZE;
	
  // If the output buffer is full, there's nothing for it other than to 
  // wait for the interrupt handler to empty it a bit
  // ???: return 0 here instead?
  while (i == _tx_buffer->tail); // os sleep
	
  _tx_buffer->buffer[_tx_buffer->head] = c;
  _tx_buffer->head = i;

  //sbi(*_ucsrb, _udrie);
  *_ucsrb |= _BV(_udrie);

  // clear the TXC bit -- "can be cleared by writing a one to its bit location"
  transmitting = true;

  //sbi(*_ucsra, TXC0);
  *_ucsra |= _BV(TXC0);
  
  return 1;
}

/****************************************/
/****************************************/

