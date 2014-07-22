/*
  TwoWire.h - TWI/I2C library for Arduino & Wiring
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

  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
*/


#ifndef TW_CONTROLLER_H
#define TW_CONTROLLER_H

#include <inttypes.h>

#define TW_BUFFER_LENGTH 64
#define TW_SCL_FREQ 100000L

#define TW_STATE_READY 0
#define TW_STATE_MRX   1
#define TW_STATE_MTX   2

#define TW_BUS_NO_ERROR 0xFF

class CTWController {
private:
   uint8_t m_punRxBuffer[TW_BUFFER_LENGTH];
   uint8_t m_unRxBufferIndex;
   uint8_t m_unRxBufferLength;

   uint8_t m_unTxAddress;
   uint8_t m_punTxBuffer[TW_BUFFER_LENGTH];
   uint8_t m_unTxBufferIndex;
   uint8_t m_unTxBufferLength;

   bool m_bTransmitting;

public:
   void BeginTransmission(uint8_t);
   uint8_t EndTransmission(bool b_send_stop = true);

   virtual uint8_t Write(uint8_t un_data); // private? merge into method below
   virtual uint8_t Write(const uint8_t* pun_data, uint8_t un_num_bytes); // b_send_stop

   uint8_t Read(uint8_t un_address, uint8_t un_length, bool b_send_stop = true);

   virtual bool Available();
   virtual uint8_t Read();
   virtual uint8_t Peek();
   virtual void Flush();
  
   static CTWController& GetInstance() {
      return m_cTWController;
   }

private:

   CTWController();

   static CTWController m_cTWController;
};

#endif

