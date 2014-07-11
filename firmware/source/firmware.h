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
#include <tuart_controller.h>
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
      char bufa[200] = {0};
      HardwareSerial::instance().write((const uint8_t*)"Hello world\r\n",13);

      for(;;) {
         
         while(HardwareSerial::instance().available()) {
            m_cTUARTController.WriteByte(HardwareSerial::instance().read());
         }

         if(m_cTUARTController.Available()) {
            sprintf(bufa, "uptime = %lu\r\n", m_cTimer.GetMilliseconds());
            HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));

            HardwareSerial::instance().write((const uint8_t*)"TUART = [", 9);
            while( m_cTUARTController.Available()) {
               m_cTimer.Delay(1);
               sprintf(bufa, "0x%02x ", m_cTUARTController.Read());
               HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
            }
            HardwareSerial::instance().write((const uint8_t*)"]\r\n", 3);
         }
         //HardwareSerial::instance().write((const uint8_t*)"Reading from MPU6050\r\n",22);
         
         //CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
         //CTWController::GetInstance().Write(MPU6050_WHOAMI);
         //CTWController::GetInstance().EndTransmission(false);
         //CTWController::GetInstance().Read(MPU6050_ADDR, 1, true);
         //m_unResult = CTWController::GetInstance().Read();      
         //HardwareSerial::instance().write((const uint8_t*)"MPU6050 returned: ",18);
         //HardwareSerial::instance().write(m_unResult);
         //HardwareSerial::instance().write((const uint8_t*)"\r\n",2);

         // Read timer values
         /*
         sprintf(bufa, "InterruptOCAOccured = %s\r\n", m_cTUARTController.InterruptOCAOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "InterruptOCBOccured = %s\r\n", m_cTUARTController.InterruptOCBOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "InterruptICOccured = %s\r\n", m_cTUARTController.InterruptICOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "EnabledOCBInterrupt = %s\r\n", m_cTUARTController.EnabledOCBInterrupt?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagA = %s\r\n", m_cTUARTController.FlagA?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagB = %s\r\n", m_cTUARTController.FlagB?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagC = %s\r\n", m_cTUARTController.FlagC?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         */
         m_cTimer.Delay(3000);
         HardwareSerial::instance().write((const uint8_t*)"sending '+++'\r\n", 15);
         m_cTUARTController.WriteByte('+');
         m_cTUARTController.WriteByte('+');
         m_cTUARTController.WriteByte('+');

         sprintf(bufa, "TxBuffer.Head = %u, TxBuffer.Tail = %u\r\n", 
                 m_cTUARTController.m_sTxBuffer.Head, 
                 m_cTUARTController.m_sTxBuffer.Tail);
                 HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         /*
         sprintf(bufa, "InterruptOCAOccured = %s\r\n", m_cTUARTController.InterruptOCAOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "InterruptOCBOccured = %s\r\n", m_cTUARTController.InterruptOCBOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "InterruptICOccured = %s\r\n", m_cTUARTController.InterruptICOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "EnabledOCBInterrupt = %s\r\n", m_cTUARTController.EnabledOCBInterrupt?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagA = %s\r\n", m_cTUARTController.FlagA?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagB = %s\r\n", m_cTUARTController.FlagB?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagC = %s\r\n", m_cTUARTController.FlagC?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         */
         
         m_cTimer.Delay(2000);
         sprintf(bufa, "RxState = %u, RxByte = %X, RxBuffer.Head = %u, RxBuffer.Tail = %u, DetectedEdges = %u\r\n", 
                 m_cTUARTController.m_unRxState, 
                 m_cTUARTController.m_unRxByte, 
                 m_cTUARTController.m_sRxBuffer.Head,
                 m_cTUARTController.m_sRxBuffer.Tail,
                 m_cTUARTController.DetectedEdges);
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         /*
         for(uint8_t i = 0; i < 40; i++) {
            m_cTimer.Delay(1);
            sprintf(bufa, "%3u: %6u\t%u\t0x%2x\t0x%2x\r\n", 
                    m_cTUARTController.Samples[i].EdgeIndex,
                    m_cTUARTController.Samples[i].CaptureTime,
                    m_cTUARTController.Samples[i].RxState,
                    m_cTUARTController.Samples[i].RxByte,
                    m_cTUARTController.Samples[i].RxBit);
            HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         }
         
         sprintf(bufa, "\nInterruptOCAOccured = %s\r\n", m_cTUARTController.InterruptOCAOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "InterruptOCBOccured = %s\r\n", m_cTUARTController.InterruptOCBOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "InterruptICOccured = %s\r\n", m_cTUARTController.InterruptICOccured?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "EnabledOCBInterrupt = %s\r\n", m_cTUARTController.EnabledOCBInterrupt?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagA = %s\r\n", m_cTUARTController.FlagA?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagB = %s\r\n", m_cTUARTController.FlagB?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         sprintf(bufa, "FlagC = %s\r\n", m_cTUARTController.FlagC?"true":"false");
         HardwareSerial::instance().write((const uint8_t*)bufa,strlen(bufa));
         */
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
               TIMER0_OVF_vect_num),
      //m_cHUARTController(...)
      //m_cTUARTController(...)
      //m_cTWController(...)

      m_cTUARTController(9600,
                         TCCR1A,
                         TCCR1B,
                         TIMSK1,
                         TIFR1,
                         ICR1,
                         OCR1A,
                         OCR1B,
                         TCNT1,
                         TIMER1_CAPT_vect_num,
                         TIMER1_COMPA_vect_num,
                         TIMER1_COMPB_vect_num) {    
                                    

      // Enable interrupts
      sei();

      // select channel 2 for i2c output
      //DDRC |= PORT_CTRL_MASK;
      //PORTC &= ~PORT_CTRL_MASK;
      //PORTC |= 1 & PORT_CTRL_MASK;
   }

   
   CTimer m_cTimer;
   CTUARTController m_cTUARTController;

   static Firmware _firmware;
};

#endif
