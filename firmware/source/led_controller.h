#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdint.h>

class CLEDController {
public:

   enum class EMode : uint8_t {
      OFF  = 0x00,
      ON   = 0x01,
      PWM  = 0x02,
      BLINK = 0x03,
   };

   static void Init();

   static void SetMode(uint8_t un_led, EMode e_mode);

   static void SetBrightness(uint8_t un_led, uint8_t un_val);

   static void SetBlinkRate(uint8_t un_period, uint8_t un_duty_cycle);

private:

   enum class ERegister : uint8_t {
      MODE1          = 0x00,
      MODE2          = 0x01,
      PWM0           = 0x02,
      PWM1           = 0x03,
      PWM2           = 0x04,
      PWM3           = 0x05,
      PWM4           = 0x06,
      PWM5           = 0x07,
      PWM6           = 0x08,
      PWM7           = 0x09,
      PWM8           = 0x0A,
      PWM9           = 0x0B,
      PWM10          = 0x0C,
      PWM11          = 0x0D,
      PWM12          = 0x0E,
      PWM13          = 0x0F,
      PWM14          = 0x10,
      PWM15          = 0x11,
      GRPPWM         = 0x12,
      GRPFREQ        = 0x13,
      LEDOUT0        = 0x14,
      LEDOUT1        = 0x15,
      LEDOUT2        = 0x16,
      LEDOUT3        = 0x17,
      SUBADR1        = 0x18,
      SUBADR2        = 0x19,
      SUBADR3        = 0x1A,
      ALLCALLADR     = 0x1B
   };
};

#endif
