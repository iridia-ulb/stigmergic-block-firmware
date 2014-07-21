/*
  HardwareSerial.h - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus
*/

#ifndef HardwareSerial_h
#define HardwareSerial_h

#include <inttypes.h>

//#include <Stream.h>
//#include <Timer.h>

struct ring_buffer;

class HardwareSerial //: public Stream
{
private:
   ring_buffer *_rx_buffer;
   ring_buffer *_tx_buffer;
   volatile uint8_t *_ubrrh;
   volatile uint8_t *_ubrrl;
   volatile uint8_t *_ucsra;
   volatile uint8_t *_ucsrb;
   volatile uint8_t *_ucsrc;
   volatile uint8_t *_udr;
   uint8_t _rxen;
   uint8_t _txen;
   uint8_t _rxcie;
   uint8_t _udrie;
   uint8_t _u2x;
   bool transmitting;

public:

   static HardwareSerial& instance() {
      return _hardware_serial;
   }

   void Begin(unsigned long);
   void End();
   virtual int Available(void);
   virtual int Peek(void);
   virtual int Read(void);
   virtual void Flush(void);
   
   /*
   uint8_t Write(const uint8_t* data, uint8_t count) {
      for(uint8_t i = 0; i < count; i++) {
         write(data[i]);
      }
      return count;
   }
   */

   virtual uint8_t Write(uint8_t);
   //inline size_t write(unsigned long n) { return write((uint8_t)n); }
   //inline size_t write(long n) { return write((uint8_t)n); }
   //inline size_t write(unsigned int n) { return write((uint8_t)n); }
   //inline size_t write(int n) { return write((uint8_t)n); }
   //using Print::write; // pull in write(str) and write(buf, size) from Print
   

   //operator bool();

private:
   
   /* singleton instance */
   static HardwareSerial _hardware_serial;

   /* constructor */
   HardwareSerial();
};

#endif
