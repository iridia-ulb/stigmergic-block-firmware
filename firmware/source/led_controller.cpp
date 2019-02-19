
#include "led_controller.h"

#include <firmware.h>

#define PCA963X_RST_ADDR  0x03 
#define PCA963X_RST_BYTE1 0xA5
#define PCA963X_RST_BYTE2 0x5A
#define PCA963X_DEV_ADDR  0x18
#define PCA963X_LEDOUTX_MASK 0x03

void CLEDController::Init() {
   /* Send the reset sequence */
   const uint8_t punResetSequence[] = {PCA963X_RST_BYTE1, PCA963X_RST_BYTE2};

   CTWController::GetInstance().Write(PCA963X_RST_ADDR, sizeof(punResetSequence), punResetSequence);
   /* Wake up the internal oscillator, disable group addressing and auto-increment */
   CTWController::GetInstance().Write(PCA963X_DEV_ADDR, ERegister::MODE1, 0x00);
   /* Enable group blinking */
   CTWController::GetInstance().Write(PCA963X_DEV_ADDR, ERegister::MODE2, 0x25);
   /* Default blink configuration 1s period, 50% duty cycle */
   SetBlinkRate(0x18, 0x80);
   /* Default all leds to off */
   for(uint8_t unLED = 0; unLED < 4; unLED++) {
      SetMode(unLED, EMode::OFF);
      SetBrightness(unLED, 0x00);
   }
}

void CLEDController::SetMode(uint8_t un_led, EMode e_mode) {
   /* get the register responsible for LED un_led */
   uint8_t unRegisterAddr = static_cast<uint8_t>(ERegister::LEDOUT0) + (un_led / 4u);
   /* read current register value */
   uint8_t unRegisterVal = CTWController::GetInstance().Read(PCA963X_DEV_ADDR, unRegisterAddr);
   /* clear and set target bits in unRegisterVal */
   unRegisterVal &= ~(PCA963X_LEDOUTX_MASK << ((un_led % 4) * 2));
   unRegisterVal |= (static_cast<uint8_t>(e_mode) << ((un_led % 4) * 2));
   /* write back */
   CTWController::GetInstance().Write(PCA963X_DEV_ADDR, unRegisterAddr, unRegisterVal);
}

void CLEDController::SetBrightness(uint8_t un_led, uint8_t un_val) {
   /* get the register responsible for LED un_led */
   uint8_t unRegisterAddr = static_cast<uint8_t>(ERegister::PWM0) + un_led;
   /* write value */
   CTWController::GetInstance().Write(PCA963X_DEV_ADDR, unRegisterAddr, un_val);
}

void CLEDController::SetBlinkRate(uint8_t un_period, uint8_t un_duty_cycle) {
   CTWController::GetInstance().Write(PCA963X_DEV_ADDR, ERegister::GRPFREQ, un_period);
   CTWController::GetInstance().Write(PCA963X_DEV_ADDR, ERegister::GRPPWM, un_duty_cycle);
}
