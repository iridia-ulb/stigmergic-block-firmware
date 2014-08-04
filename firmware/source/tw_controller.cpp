/*
  TW.cpp - TWI/I2C library for Wiring & Arduino
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


#include <string.h>
#include <inttypes.h>
#include <util/twi.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "tw_controller.h"
#include "firmware.h"

// Preinstantiate Objects //////////////////////////////////////////////////////

CTWController CTWController::m_cTWController;

// Initialize Class Variables //////////////////////////////////////////////////

//uint8_t CTWController::m_unRxBuffer[TW_BUFFER_LENGTH];
//uint8_t CTWController::m_unRxBufferIndex = 0;
//uint8_t CTWController::m_unRxBufferLength = 0;

//uint8_t CTWController::m_unTxAddress = 0;
//uint8_t CTWController::m_unTxBuffer[TW_BUFFER_LENGTH];
//uint8_t CTWController::m_unTxBufferIndex = 0;
//uint8_t CTWController::m_unTxBufferLength = 0;

//uint8_t CTWController::m_bTransmitting = false;

// Interrupt Variables //////////////////////////////////////////////////

static volatile uint8_t unState;
static volatile uint8_t unSlarw;
static volatile bool    bSendStop;			// should the transaction end with a stop
static volatile bool    bInRepStart;			// in the middle of a repeated start

static uint8_t          punMasterBuffer[TW_BUFFER_LENGTH];
static volatile uint8_t unMasterBufferIndex;
static volatile uint8_t unMasterBufferLength;

//static uint8_t          punTxBuffer[TW_BUFFER_LENGTH];
static volatile uint8_t unTxBufferIndex;
static volatile uint8_t unTxBufferLength;

//static uint8_t          punRxBuffer[TW_BUFFER_LENGTH];
static volatile uint8_t unRxBufferIndex;

static volatile uint8_t unError;

// Interrupt Routine ////////////////////////////////////////////////////////////////

ISR(TWI_vect)
{
   switch(TW_STATUS) {
      // All Master
   case TW_START:     // sent start condition
   case TW_REP_START: // sent repeated start condition
      // copy device address and r/w bit to output register and ack
      TWDR = unSlarw;
      TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA); // reply with ack
      break;
      // Master Transmitter
   case TW_MT_SLA_ACK:  // slave receiver acked address
   case TW_MT_DATA_ACK: // slave receiver acked data
      // if there is data to send, send it, otherwise stop 
      if(unMasterBufferIndex < unMasterBufferLength) {
         // copy data to output register and ack
         TWDR = punMasterBuffer[unMasterBufferIndex++];
         TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA); // reply with ack
      } 
      else {
         if (bSendStop) {
            TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO); // stop
            while(TWCR & _BV(TWSTO)){
               continue;
            }
            unState = TW_STATE_READY;
         }
         else {
            bInRepStart = true;	// we're gonna send the START
            // don't enable the interrupt. We'll generate the start, but we 
            // avoid handling the interrupt until we're in the next transaction,
            // at the point where we would normally issue the start.
            TWCR = _BV(TWINT) | _BV(TWSTA)| _BV(TWEN);
            unState = TW_STATE_READY;
         }
      }
      break;
   case TW_MT_SLA_NACK:  // address sent, nack received
      unError = TW_MT_SLA_NACK;
      TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO); // stop
      while(TWCR & _BV(TWSTO)){
         continue;
      }
      unState = TW_STATE_READY;
      break;
   case TW_MT_DATA_NACK: // data sent, nack received
      unError = TW_MT_DATA_NACK;
      TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO); // stop
      while(TWCR & _BV(TWSTO)){
         continue;
      }
      unState = TW_STATE_READY;
      break;
   case TW_MT_ARB_LOST: // lost bus arbitration
      unError = TW_MT_ARB_LOST;
      TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT); // release bus
      unState = TW_STATE_READY;
      break;

      // Master Receiver
   case TW_MR_DATA_ACK: // data received, ack sent
      // put byte into buffer
      punMasterBuffer[unMasterBufferIndex++] = TWDR;
   case TW_MR_SLA_ACK:  // address sent, ack received
      // ack if more bytes are expected, otherwise nack
      if(unMasterBufferIndex < unMasterBufferLength){
         TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA); // reply with ack
      } 
      else {
         TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT); // reply without ack
      }
      break;
   case TW_MR_DATA_NACK: // data received, nack sent
      // put final byte into buffer
      punMasterBuffer[unMasterBufferIndex++] = TWDR;
      if (bSendStop) {
         TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO); // stop
         while(TWCR & _BV(TWSTO)){
            continue;
         }
         unState = TW_STATE_READY;
      }
      else {
         bInRepStart = true;	// we're gonna send the START
         // don't enable the interrupt. We'll generate the start, but we 
         // avoid handling the interrupt until we're in the next transaction,
         // at the point where we would normally issue the start.
         TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) ;
         unState = TW_STATE_READY;
      }    
      break;
   case TW_MR_SLA_NACK: // address sent, nack received
      TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO); // stop
      while(TWCR & _BV(TWSTO)){
         continue;
      }
      unState = TW_STATE_READY;
      break;
      // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case

      // All
   case TW_NO_INFO:   // no state information
      break;
   case TW_BUS_ERROR: // bus error, illegal stop/start
      unError = TW_BUS_ERROR;
      TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO); // stop
      while(TWCR & _BV(TWSTO)){
         continue;
      }
      unState = TW_STATE_READY;
      break;
   }
}

// Constructors ////////////////////////////////////////////////////////////////

CTWController::CTWController()
{
  m_unRxBufferIndex = 0;
  m_unRxBufferLength = 0;

  m_unTxAddress = 0;
  m_unTxBufferIndex = 0;
  m_unTxBufferLength = 0;

  m_bTransmitting = false;

  // interrupt globals
  unState = TW_STATE_READY;
  bSendStop = true;		// default value
  bInRepStart = false;
  
  // NOT REQUIRED, external pull ups are present, ports are input by default
  //digitalWrite(SDA, 1);
  //digitalWrite(SCL, 1);

  // initialize twi prescaler and bit rate
  //cbi(TWSR, TWPS0);
  //cbi(TWSR, TWPS1);
  TWSR &= ~(_BV(TWPS0) | _BV(TWPS1));

  // prescaler, TWBR is 32 for 8MHz external clock and 100KHz SCL
  TWBR = ((F_CPU / TW_SCL_FREQ) - 16) / 2;

  // enable i2c hardware, acks, and interrupt
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

// Public Methods //////////////////////////////////////////////////////////////

uint8_t CTWController::Read(uint8_t un_address, uint8_t un_length, bool b_send_stop)
{
  // clamp to buffer length
  if(un_length > TW_BUFFER_LENGTH) {
    un_length = TW_BUFFER_LENGTH;
  }
  
  // ensure data will fit into buffer
  if(TW_BUFFER_LENGTH < un_length) {
    return 0;
  }

  // wait until I2C is ready, become master receiver
  while(unState != TW_STATE_READY) {
     continue; // os sleep
  }
  unState = TW_STATE_MRX;
  bSendStop = b_send_stop;

  // reset error state
  unError = TW_BUS_NO_ERROR;

  unMasterBufferIndex = 0;
  unMasterBufferLength = un_length - 1;
  unSlarw = TW_READ;
  unSlarw |= un_address << 1;

  if (bInRepStart == true) {
    // if we're in the repeated start state, then we've already sent the start,
    // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
    // We need to remove ourselves from the repeated start state before we enable interrupts,
    // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
    // up. Also, don't enable the START interrupt. There may be one pending from the 
    // repeated start that we sent outselves, and that would really confuse things.
    bInRepStart = false;			// remember, we're dealing with an ASYNC ISR
    TWDR = unSlarw;
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);	// enable INTs, but not START
  }
  else
    // send start condition
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);

  // wait for read operation to complete
  while(unState == TW_STATE_MRX) {
    continue;
  }

  if (unMasterBufferIndex < un_length)
    un_length = unMasterBufferIndex;

  // copy buffer into class data buffer
  //some unrequired copying happening here / remove after testing
  for(uint8_t i = 0; i < un_length; ++i) {
    m_punRxBuffer[i] = punMasterBuffer[i];
  }
	
  // set rx buffer iterator vars
  m_unRxBufferIndex = 0;
  m_unRxBufferLength = un_length;

  return un_length;
}


void CTWController::BeginTransmission(uint8_t un_tx_address) {
  // indicate that we are transmitting
  m_bTransmitting = true;
  // set address of targeted slave
  m_unTxAddress = un_tx_address;
  // reset TX buffer iterator vars
  m_unTxBufferIndex = 0;
  m_unTxBufferLength = 0;
}

//
//	Originally, 'endTransmission' was an f(void) function.
//	It has been modified to take one parameter indicating
//	whether or not a STOP should be performed on the bus.
//	Calling endTransmission(false) allows a sketch to 
//	perform a repeated start. 
//
//	WARNING: Nothing in the library keeps track of whether
//	the bus tenure has been properly ended with a STOP. It
//	is very possible to leave the bus in a hung state if
//	no call to endTransmission(true) is made. Some I2C
//	devices will behave oddly if they do not see a STOP.
//
uint8_t CTWController::EndTransmission(bool b_send_stop) {
   // transmit buffer (blocking)
   //int8_t ret = twi_writeTo(txAddress, txBuffer, txBufferLength, 1, b_send_stop);
   // uint8_t address, uint8_t* data, uint8_t length, uint8_t wait, uint8_t sendStop

   // ensure data will fit into buffer
   if(TW_BUFFER_LENGTH < m_unTxBufferLength){
      return 1;
   }

   // wait until twi is ready, become master transmitter
   while(unState != TW_STATE_READY) {
      continue; // os sleep
   }

   unState = TW_STATE_MTX;
   bSendStop = b_send_stop;

   // reset error state (0xFF.. no error occured)
   unError = TW_BUS_NO_ERROR;

   // initialize buffer iteration vars
   unMasterBufferIndex = 0;
   unMasterBufferLength = m_unTxBufferLength;
  
   // copy data to twi buffer
   for(uint8_t i = 0; i < m_unTxBufferLength; ++i){
      punMasterBuffer[i] = m_punTxBuffer[i];
   }
  
   // build sla+w, slave device address + w bit
   unSlarw = TW_WRITE;
   unSlarw |= m_unTxAddress << 1;
  
   // if we're in a repeated start, then we've already sent the START
   // in the ISR. Don't do it again.
   //
   if (bInRepStart == true) {
      // if we're in the repeated start state, then we've already sent the start,
      // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
      // We need to remove ourselves from the repeated start state before we enable interrupts,
      // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
      // up. Also, don't enable the START interrupt. There may be one pending from the 
      // repeated start that we sent outselves, and that would really confuse things.
      bInRepStart = false;			// remember, we're dealing with an ASYNC ISR
      TWDR = unSlarw;				
      TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE); // enable INTs, but not START
   }
   else {
      // send start condition
      TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA); // enable INTs
   }

   // wait for write operation to complete
   while(unState == TW_STATE_MTX) {
      continue; // os sleep
   }

   // reset tx buffer iterator vars
   m_unTxBufferIndex = 0;
   m_unTxBufferLength = 0;

   // indicate that we are done transmitting
   m_bTransmitting = false;
  
   if (unError == TW_BUS_NO_ERROR) // clean up with case statement
      return 0;	// success
   else if (unError == TW_MT_SLA_NACK)
      return 2;	// error: address send, nack received
   else if (unError == TW_MT_DATA_NACK)
      return 3;	// error: data send, nack received
  
   return 4;	// other twi error
}
   
   
// must be called in:
// slave tx event callback
// or after beginTransmission(address)
   
uint8_t CTWController::Write(uint8_t un_data) {
   if(m_unTxBufferLength >= TW_BUFFER_LENGTH) {
      //SetWriteError();
      return 0;
   }
   // put byte in tx buffer
   m_punTxBuffer[m_unTxBufferIndex] = un_data;
   ++m_unTxBufferIndex;
   // update amount in buffer   
   m_unTxBufferLength = m_unTxBufferIndex;
   return 1;
}
   
// must be called in:
// slave tx event callback
// or after beginTransmission(address)
uint8_t CTWController::Write(const uint8_t* pun_data, uint8_t un_quantity) {
   for(uint8_t i = 0; i < un_quantity; ++i){
      Write(pun_data[i]);
   }
   return un_quantity;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
bool CTWController::Available() {
   return (m_unRxBufferLength - m_unRxBufferIndex > 0);
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
uint8_t CTWController::Read() {
  uint8_t un_value = -1;
  
  // get each successive byte on each call
  if(m_unRxBufferIndex < m_unRxBufferLength){
    un_value = m_punRxBuffer[m_unRxBufferIndex];
    ++m_unRxBufferIndex;
  }
  return un_value;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
uint8_t CTWController::Peek(void) {
  uint8_t un_value = -1;
  
  if(m_unRxBufferIndex < m_unRxBufferLength) {
    un_value = m_punRxBuffer[m_unRxBufferIndex];
  }

  return un_value;
}

void CTWController::Flush(void)
{
  // XXX: to be implemented.
}


