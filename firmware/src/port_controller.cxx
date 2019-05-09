
#include "port_controller.h"

/***********************************************************/
/***********************************************************/

#include <avr/io.h>

#include "clock.h"
#include "tw_controller.h"

/***********************************************************/
/***********************************************************/

#define PORTC_TWCLK_MASK 0x20
#define PORT_MUX_MASK 0x0F

/***********************************************************/
/***********************************************************/

CPortController::EPort CPortController::Ports[] = {
   EPort::North, EPort::East, EPort::South, EPort::West, EPort::Top, EPort::Bottom,
};

/***********************************************************/
/***********************************************************/

char CPortController::PortToChar(EPort e_port) {
   switch(e_port) {
   case CPortController::EPort::North:
      return 'N';
      break;
   case CPortController::EPort::East:
      return 'E';
      break;
   case CPortController::EPort::South:
      return 'S';
      break;
   case CPortController::EPort::West:
      return 'W';
      break;
   case CPortController::EPort::Top:
      return 'T';
      break;
   case CPortController::EPort::Bottom:
      return 'B';
      break;
   default:
      return 'X';
      break;
   }
}

/***********************************************************/
/***********************************************************/

CPortController::CPortController() {
   /* configure the port multiplexer */
   DDRC |= PORT_MUX_MASK;
   /* disable all ports initially */
   /* safe state, since selected a non-connected face results in stalling the I2C bus on R/W */
   Select(EPort::Disconnect);
   /* configure the reset lines to the faces as outputs */
   CTWController::GetInstance().WriteRegister(m_unResetDeviceAddress, ERegister::Configuration, 0xC0);
   /* disable all ports (hold the NFC controllers in reset) */
   CTWController::GetInstance().WriteRegister(m_unResetDeviceAddress, ERegister::Output, 0x00);
   /* detect which ports are actually connected */
   /* disable the TW controller */
   CTWController::GetInstance().Disable();
   for(EPort e_port : Ports) {
      Select(e_port);
      /* delay a bit */
      CClock::GetInstance().Delay(10);
      /* detect the presence of the pull-ups on a connected port */
      PORTC &= ~PORTC_TWCLK_MASK;
      DDRC |= PORTC_TWCLK_MASK;
      DDRC &= ~PORTC_TWCLK_MASK;
      PORTC |= PORTC_TWCLK_MASK;
      /* delay a bit more */
      CClock::GetInstance().Delay(10);
      /* test if a port is connected */
      if((PINC & PORTC_TWCLK_MASK) != 0) {
         m_cConnectedPorts.Insert(e_port);
      }
   }
   /* note that the following line re-enables the CTWController */
   Select(EPort::Disconnect);
}

/***********************************************************/
/***********************************************************/

uint8_t CPortController::GetInterrupts() {
   uint8_t unInputBuffer = 0;
   CTWController::GetInstance().ReadRegister(m_unInterruptDeviceAddress, ERegister::Input, 1, &unInputBuffer);
   return ~unInputBuffer;
}

/***********************************************************/
/***********************************************************/

bool CPortController::Enable(EPort e_port) {
   /* Read current configuration */
   uint8_t unPortState;
   bool bStatus = CTWController::GetInstance().ReadRegister(m_unResetDeviceAddress, ERegister::Output, 1, &unPortState);
   /* Enable the target port */
   unPortState |= (1 << static_cast<uint8_t>(e_port));
   /* Write back updated configuration */
   bStatus = bStatus && CTWController::GetInstance().WriteRegister(m_unResetDeviceAddress, ERegister::Output, unPortState);
   return bStatus;
}

/***********************************************************/
/***********************************************************/

bool CPortController::Disable(EPort e_port) {
   /* Read current configuration */
   uint8_t unPortState;
   bool bStatus = CTWController::GetInstance().ReadRegister(m_unResetDeviceAddress, ERegister::Output, 1, &unPortState);
   /* Disable the target port */
   unPortState &= ~(1 << static_cast<uint8_t>(e_port));
   /* Write back updated configuration */
   bStatus = bStatus && CTWController::GetInstance().WriteRegister(m_unResetDeviceAddress, ERegister::Output, unPortState);
   return bStatus;
}

/***********************************************************/
/***********************************************************/

void CPortController::Select(EPort e_port) {
   uint8_t unPortSelector = PORTC;
   unPortSelector &= ~PORT_MUX_MASK;
   unPortSelector |= static_cast<uint8_t>(e_port) & PORT_MUX_MASK;
   PORTC = unPortSelector;
}

/***********************************************************/
/***********************************************************/

