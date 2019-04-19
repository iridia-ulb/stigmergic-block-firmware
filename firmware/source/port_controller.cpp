// TODO remove
#include <firmware.h>

#include "port_controller.h"

#include <avr/io.h>
#include "tw_controller.h"

#define PCA9554_RST_ADDR 0x20
#define PCA9554_IRQ_ADDR 0x21

#define PORTC_TWCLK_MASK 0x20
#define PORT_MUX_MASK 0x0F

/***********************************************************/
/***********************************************************/

CPortController::CPortController() { 

   /* Configure the port multiplexer */
   DDRC |= PORT_MUX_MASK;

   /* Disable all ports initially */
   /* Safe state, since selected a non-connected face results in stalling the I2C bus on R/W */
   SelectPort(EPort::NULLPORT);
}


void CPortController::Init() {
   // TODO is method called anywhere? reset would be improved if we power cycled NFC chips on reset

   /* Configure the reset lines to the faces as outputs (driven high by default) */
   CTWController::GetInstance().WriteRegister(PCA9554_RST_ADDR, EPCA9554Register::CONFIG, 0xC0);
}

/***********************************************************/
/***********************************************************/

uint8_t CPortController::GetInterrupts() {
   uint8_t unInputBuffer = 0;
   CTWController::GetInstance().ReadRegister(PCA9554_IRQ_ADDR, EPCA9554Register::INPUT, 1, &unInputBuffer);
   return ~unInputBuffer;
}

/***********************************************************/
/***********************************************************/

bool CPortController::EnablePort(EPort e_port) {
   /* Read current configuration */
   uint8_t unPortState;
   bool bStatus = CTWController::GetInstance().ReadRegister(PCA9554_RST_ADDR, EPCA9554Register::OUTPUT, 1, &unPortState);
   /* Enable the target port */
   unPortState |= (1 << static_cast<uint8_t>(e_port));
   /* Write back updated configuration */
   bStatus = bStatus && CTWController::GetInstance().WriteRegister(PCA9554_RST_ADDR, EPCA9554Register::OUTPUT, unPortState);
   return bStatus;
}

/***********************************************************/
/***********************************************************/

bool CPortController::DisablePort(EPort e_port) {
   /* Read current configuration */
   uint8_t unPortState;
   bool bStatus = CTWController::GetInstance().ReadRegister(PCA9554_RST_ADDR, EPCA9554Register::OUTPUT, 1, &unPortState);
   /* Disable the target port */
   unPortState &= ~(1 << static_cast<uint8_t>(e_port));
   /* Write back updated configuration */
   bStatus = bStatus && CTWController::GetInstance().WriteRegister(PCA9554_RST_ADDR, EPCA9554Register::OUTPUT, unPortState);
   return bStatus;
}

/***********************************************************/
/***********************************************************/

void CPortController::SelectPort(EPort e_port) {
   uint8_t unPortSelector = PORTC;
   unPortSelector &= ~PORT_MUX_MASK;
   unPortSelector |= static_cast<uint8_t>(e_port) & PORT_MUX_MASK;
   PORTC = unPortSelector;
}

/***********************************************************/
/***********************************************************/

/* This function detects if the selected port in connected, 
   via testing the pull up resistors */
bool CPortController::IsPortConnected(EPort e_port) {
   /* Make sure TWController is disabled before testing */
   // To avoid confusing devices, only use the clock line
   SelectPort(e_port);
   CClock::GetInstance().Delay(10);

   PORTC &= ~PORTC_TWCLK_MASK;
   DDRC |= PORTC_TWCLK_MASK;
   DDRC &= ~PORTC_TWCLK_MASK;
   PORTC |= PORTC_TWCLK_MASK;

   CClock::GetInstance().Delay(10);

   return ((PINC & PORTC_TWCLK_MASK) != 0);
}

/***********************************************************/
/***********************************************************/
