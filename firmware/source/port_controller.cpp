#include "port_controller.h"

#include <avr/io.h>
#include <firmware.h> // remove this, include timer instead
#include "tw_controller.h"

#define NUM_FACES 6
#define PORTC_TWCLK_MASK 0x20

/***********************************************************/
/***********************************************************/

CPortController::CPortController() { 

   /* Configure the port TW multiplexer */
   DDRC |= PORT_CTRL_MASK;

   /* Disable all ports initially */
   /* Safe state, since selected a non-connected face results in stalling the I2C bus on R/W */
   SelectPort(EPort::NULLPORT);
}


void CPortController::Init() {
   /* Configure the reset lines to the faces as outputs (driven high by default) */
   CTWController::GetInstance().Write(PCA9554_RST_ADDR, EPCA9554Register::CONFIG, 0xC0);
}

/***********************************************************/
/***********************************************************/

uint8_t CPortController::GetInterrupts() {
   uint8_t unInputBuffer = CTWController::GetInstance().Read(PCA9554_IRQ_ADDR, EPCA9554Register::INPUT);
   return ~unInputBuffer;
}

/***********************************************************/
/***********************************************************/

void CPortController::EnablePort(EPort e_target_port) {
   /* Read current configuration */
   uint8_t unPortState = CTWController::GetInstance().Read(PCA9554_RST_ADDR, EPCA9554Register::OUTPUT);
   /* Enable the target port */
   unPortState |= (1 << static_cast<uint8_t>(e_target_port));
   /* Write back updated configuration */
   CTWController::GetInstance().Write(PCA9554_RST_ADDR, EPCA9554Register::OUTPUT, unPortState);
}

/***********************************************************/
/***********************************************************/

void CPortController::DisablePort(EPort e_target_port) {
   /* Read current configuration */
   uint8_t unPortState = CTWController::GetInstance().Read(PCA9554_RST_ADDR, EPCA9554Register::OUTPUT);
   /* Enable the target port */
   unPortState &= ~(1 << static_cast<uint8_t>(e_target_port));
   /* Write back updated configuration */
   CTWController::GetInstance().Write(PCA9554_RST_ADDR, EPCA9554Register::OUTPUT, unPortState);
}

/***********************************************************/
/***********************************************************/

void CPortController::SelectPort(EPort e_target) {
   PORTC &= ~PORT_CTRL_MASK;
   PORTC |= static_cast<uint8_t>(e_target) & PORT_CTRL_MASK;
}

/***********************************************************/
/***********************************************************/

/* This function detects if the selected port in connected, 
   via testing the pull up resistors */
bool CPortController::IsPortConnected(EPort e_target) {
   /* Make sure TWController is disabled before testing */
   // To avoid confusing devices, only use the clock line
   SelectPort(e_target);
   Firmware::GetInstance().GetTimer().Delay(10);

   PORTC &= ~PORTC_TWCLK_MASK;
   DDRC |= PORTC_TWCLK_MASK;
   DDRC &= ~PORTC_TWCLK_MASK;
   PORTC |= PORTC_TWCLK_MASK;

   Firmware::GetInstance().GetTimer().Delay(10);

   return ((PINC & PORTC_TWCLK_MASK) != 0);
}

/***********************************************************/
/***********************************************************/
