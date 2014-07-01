#ifndef FIRMWARE_H
#define FIRMWARE_H

/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>

/* Firmware Headers */
#include <HardwareSerial.h>
#include <I2CController.h>
#include <Timer.h>

/* I2C Address Space */
#define MPU6050_ADDR 0x68
#define MPU6050_WHOAMI 0x75

#define PORT_CTRL_MASK 0x0F

class Firmware {
public:
      
   static Firmware& instance() {
      return _firmware;
   }

   int exec() {         
      for(;;) {
         if(HardwareSerial::instance().available()) {
            HardwareSerial::instance().write("Hello world\r\n");
            while(HardwareSerial::instance().available()) {
               HardwareSerial::instance().read();
            }
         }
         HardwareSerial::instance().write("Reading from MPU6050\r\n");
         
         CTwoWireController::GetInstance().BeginTransmission(MPU6050_ADDR);
         CTwoWireController::GetInstance().Write(MPU6050_WHOAMI);
         CTwoWireController::GetInstance().EndTransmission(false);
         CTwoWireController::GetInstance().Read(MPU6050_ADDR, 1, true);
         m_unResult = CTwoWireController::GetInstance().Read();      
         HardwareSerial::instance().write("MPU6050 returned: ");
         HardwareSerial::instance().write(m_unResult);
         HardwareSerial::instance().write("\r\n");
         Timer::instance().delay(1000);
      }
      return 0;         
   }
      
private:

   uint8_t m_unResult;
  
   Firmware() {
      // Enable interrupts
      sei();

      // select channel 2 for i2c output
      DDRC |= PORT_CTRL_MASK;
      PORTC &= ~PORT_CTRL_MASK;
      PORTC |= 1 & PORT_CTRL_MASK;
   }

   static Firmware _firmware;
};

#endif
