#include "port_controller.h"

#include <avr/io.h>
#include <firmware.h>

#define NUM_FACES 6
#define PORTC_TWCLK_MASK 0x20

CPortController::CPortInterrupt::CPortInterrupt(CPortController* pc_port_controller, uint8_t un_intr_vect_num) :
   m_pcPortController(pc_port_controller) {
   Register(this, un_intr_vect_num);
}

/***********************************************************/
/***********************************************************/

void CPortController::CPortInterrupt::ServiceRoutine() {
   m_pcPortController->bSynchronizeRequired = true;
}

/***********************************************************/
/***********************************************************/

CPortController::CPortController() :
   m_unInterrupts(0x00),
   m_unLastRegisterState(0xFF),
   bSynchronizeRequired(true),
   m_cPortInterrupt(this, 1) { 

   /* Configure the port TW multiplexer */
   DDRC |= PORT_CTRL_MASK;

   /* Disable all ports initially */
   /* Safe state, since selected a non-connected face results in stalling the tw bus on R/W */
   SelectPort(EPort::NULLPORT);
}


void CPortController::Init() {
   /* Configure the port reset register */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::CONFIG));
   /* Configure the reset lines to the faces as outputs (driven high by default) */
   Firmware::GetInstance().GetTWController().Write(0xC0);
   Firmware::GetInstance().GetTWController().EndTransmission(true);

   /* Note: in next hardware revision disable all faces on init */

   /* Enable external interrupt */
   EICRA &= ~EXT_INT0_SENSE_MASK;
   EICRA |= EXT_INT0_FALLING_EDGE;
   EIMSK |= EXT_INT0_ENABLE;
}

/***********************************************************/
/***********************************************************/

void CPortController::SynchronizeInterrupts() {
   if(bSynchronizeRequired) {
      /* read port */
      Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_IRQ_ADDR);
      Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::INPUT));
      Firmware::GetInstance().GetTWController().EndTransmission(false);  
      Firmware::GetInstance().GetTWController().Read(PCA9554_IRQ_ADDR, 1, true);
      bSynchronizeRequired = false;
      uint8_t unRegisterState = Firmware::GetInstance().GetTWController().Read();
      /* append detected falling edges to the interrupt vector */
      m_unInterrupts |= (unRegisterState ^ m_unLastRegisterState) & (~unRegisterState);
      /* store the synchronized state for the next sync */
      m_unLastRegisterState = unRegisterState;
   }
}

/***********************************************************/
/***********************************************************/

void CPortController::ClearInterrupts(uint8_t un_clear_mask) {
   m_unInterrupts &= (~un_clear_mask);
}

/***********************************************************/
/***********************************************************/

bool CPortController::HasInterrupts() {
   return (m_unInterrupts != 0x00);
}

/***********************************************************/
/***********************************************************/

uint8_t CPortController::GetInterrupts() {
   return m_unInterrupts;
}

/***********************************************************/
/***********************************************************/

void CPortController::EnablePort(EPort e_target_port) {
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::OUTPUT));
   Firmware::GetInstance().GetTWController().EndTransmission(false);
   Firmware::GetInstance().GetTWController().Read(PCA9554_RST_ADDR, 1, true);
   uint8_t unPortState = Firmware::GetInstance().GetTWController().Read();
   /* Enable the target port */
   unPortState |= (1 << static_cast<uint8_t>(e_target_port));
   /* Write configuration back */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::OUTPUT));
   Firmware::GetInstance().GetTWController().Write(unPortState);
   Firmware::GetInstance().GetTWController().EndTransmission(true);
}

/***********************************************************/
/***********************************************************/

void CPortController::DisablePort(EPort e_target_port) {
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::OUTPUT));
   Firmware::GetInstance().GetTWController().EndTransmission(false);
   Firmware::GetInstance().GetTWController().Read(PCA9554_RST_ADDR, 1, true);
   uint8_t unPortState = Firmware::GetInstance().GetTWController().Read();
   /* Disable the target port */
   unPortState &= ~(1 << static_cast<uint8_t>(e_target_port));
   /* Write configuration back */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::OUTPUT));
   Firmware::GetInstance().GetTWController().Write(unPortState);
   Firmware::GetInstance().GetTWController().EndTransmission(true);
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
