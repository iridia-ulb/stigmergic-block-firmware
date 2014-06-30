#ifndef FIRMWARE_H
#define FIRMWARE_H

/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>

/* Firmware Headers */
#include <HardwareSerial.h>

class Firmware {
public:
      
   static Firmware& instance() {
      return _firmware;
   }

   int exec() {         
      for(;;) {
         if(HardwareSerial::instance().available()) {
            HardwareSerial::instance().write("hello world\n");
            while(HardwareSerial::instance().available()) {
               HardwareSerial::instance().read();
            }
         }
      }
      return 0;         
   }
      
private:
  
   Firmware() {
      // Enable interrupts
      sei();      
   }

   static Firmware _firmware;
};

#endif
