#ifndef FIRMWARE_H
#define FIRMWARE_H

/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>

/* Firmware Headers */
#include <huart_controller.h>
#include <tuart_controller.h>
#include <tw_controller.h>
#include <nfc_controller.h>
#include <led_controller.h>
#include <port_controller.h>
#include <adc_controller.h>
#include <clock.h>

#define PWR_MON_MASK   0xC0
#define PWR_MON_PGOOD  0x40
#define PWR_MON_CHG    0x80

#define XBEE_RST_PIN   0x20


class CFirmware {
public:
      
   static CFirmware& GetInstance() {
      static CFirmware cInstance;
      return cInstance;
   }

   void Execute();
      
private:

   /* private constructor */
   CFirmware() {}

   void Log(const char* pch_message, bool b_bold = false);
};

#endif
