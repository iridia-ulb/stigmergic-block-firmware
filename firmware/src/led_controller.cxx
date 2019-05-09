
#include "led_controller.h"

/***********************************************************/
/***********************************************************/

#include "tw_controller.h"

/***********************************************************/
/***********************************************************/

#define PCA963X_RST_ADDR  0x03 
#define PCA963X_DEV_ADDR  0x18
#define PCA963X_LEDOUTX_MASK 0x03

const uint8_t CLEDController::m_punResetSequence[] = {
   0xA5, /* reset byte 1 */
   0x5A, /* reset byte 2 */
};  

/***********************************************************/
/***********************************************************/

bool CLEDController::Init() {
   /* Write the reset sequence to the reset address */
   bool bStatus = CTWController::GetInstance().Write(PCA963X_RST_ADDR, sizeof m_punResetSequence, m_punResetSequence);
   /* Wake up the internal oscillator, disable group addressing and auto-increment */
   bStatus = bStatus && CTWController::GetInstance().WriteRegister(PCA963X_DEV_ADDR, ERegister::MODE1, 0x00);
   /* Enable group blinking */
   bStatus = bStatus && CTWController::GetInstance().WriteRegister(PCA963X_DEV_ADDR, ERegister::MODE2, 0x25);
   /* Default blink configuration 1s period, 50% duty cycle */
   bStatus = bStatus && SetBlinkRate(0x18, 0x80);
   /* Default all leds to off */
   for(uint8_t unLED = 0; unLED < 4; unLED++) {
      bStatus = bStatus && SetMode(unLED, EMode::Off);
      bStatus = bStatus && SetBrightness(unLED, 0x00);
   }
   return bStatus;
}

/***********************************************************/
/***********************************************************/

bool CLEDController::SetMode(uint8_t un_led, EMode e_mode) {
   /* get the register responsible for LED un_led */
   uint8_t unRegisterAddr = static_cast<uint8_t>(ERegister::LEDOUT0) + (un_led / 4u);
   /* read current register value */
   uint8_t unRegisterVal;
   bool bStatus = CTWController::GetInstance().ReadRegister(PCA963X_DEV_ADDR, unRegisterAddr, 1, &unRegisterVal);
   /* clear and set target bits in unRegisterVal */
   unRegisterVal &= ~(PCA963X_LEDOUTX_MASK << ((un_led % 4) * 2));
   unRegisterVal |= (static_cast<uint8_t>(e_mode) << ((un_led % 4) * 2));
   /* write back */
   bStatus = bStatus && CTWController::GetInstance().WriteRegister(PCA963X_DEV_ADDR, unRegisterAddr, unRegisterVal);
   return bStatus;
}

/***********************************************************/
/***********************************************************/

bool CLEDController::SetBrightness(uint8_t un_led, uint8_t un_val) {
   /* get the register responsible for LED un_led */
   uint8_t unRegisterAddr = static_cast<uint8_t>(ERegister::PWM0) + un_led;
   /* write value */
   return CTWController::GetInstance().WriteRegister(PCA963X_DEV_ADDR, unRegisterAddr, un_val);
}

/***********************************************************/
/***********************************************************/

bool CLEDController::SetBlinkRate(uint8_t un_period, uint8_t un_duty_cycle) {
   bool bStatus = CTWController::GetInstance().WriteRegister(PCA963X_DEV_ADDR, ERegister::GRPFREQ, un_period);
   bStatus = bStatus && CTWController::GetInstance().WriteRegister(PCA963X_DEV_ADDR, ERegister::GRPPWM, un_duty_cycle);
   return bStatus;
}

/***********************************************************/
/***********************************************************/

