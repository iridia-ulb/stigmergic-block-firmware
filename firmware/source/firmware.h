#ifndef FIRMWARE_H
#define FIRMWARE_H

/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>

/* debug */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Firmware Headers */
#include <huart_controller.h>
#include <tw_controller.h>
#include <timer.h>

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
      char bufa[25] = {0};
   
      for(;;) {
         if(HardwareSerial::instance().available()) {
            HardwareSerial::instance().write((const uint8_t*)"Hello world\r\n",13);
            while(HardwareSerial::instance().available()) {
               HardwareSerial::instance().read();
            }
         }
         HardwareSerial::instance().write((const uint8_t*)"Reading from MPU6050\r\n",22);
         
         CTwoWireController::GetInstance().BeginTransmission(MPU6050_ADDR);
         CTwoWireController::GetInstance().Write(MPU6050_WHOAMI);
         CTwoWireController::GetInstance().EndTransmission(false);
         CTwoWireController::GetInstance().Read(MPU6050_ADDR, 1, true);
         m_unResult = CTwoWireController::GetInstance().Read();      
         HardwareSerial::instance().write((const uint8_t*)"MPU6050 returned: ",18);
         HardwareSerial::instance().write(m_unResult);
         HardwareSerial::instance().write((const uint8_t*)"\r\n",2);

         // Read timer values
         sprintf(bufa, "ms = %lu\r\n", m_cTimer.GetMilliseconds());
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));

         m_cTimer.Delay(1000);
         

      }
      return 0;         
   }
      
private:

   uint8_t m_unResult;

   /* private constructor */
   Firmware() :
      m_cTimer(TCCR0A,
               TCCR0A | (_BV(WGM00) | _BV(WGM01)),
               TCCR0B,
               TCCR0B | (_BV(CS00) | _BV(CS01)),
               TIMSK0,
               TIMSK0 | _BV(TOIE0),
               TIFR0,
               TCNT0,
               TIMER0_OVF_vect_num) {
      //m_cHUARTController(...)
      //m_cTUARTController(...)
      //m_cTWController(...)


      // Enable interrupts
      sei();

      // select channel 2 for i2c output
      DDRC |= PORT_CTRL_MASK;
      PORTC &= ~PORT_CTRL_MASK;
      PORTC |= 1 & PORT_CTRL_MASK;
   }


   


   CTimer m_cTimer;

   static Firmware _firmware;
};

#endif
