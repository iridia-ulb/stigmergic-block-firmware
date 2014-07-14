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
#define MPU6050_ADDR               0x68

/* MPU6050 Registers */
#define MPU6050_WHOAMI             0x75
#define MPU6050_PWR_MGMT_1         0x6B // R/W
#define MPU6050_PWR_MGMT_2         0x6C // R/W

#define MPU6050_ACCEL_XOUT_H       0x3B // R  
#define MPU6050_ACCEL_XOUT_L       0x3C // R  
#define MPU6050_ACCEL_YOUT_H       0x3D // R  
#define MPU6050_ACCEL_YOUT_L       0x3E // R  
#define MPU6050_ACCEL_ZOUT_H       0x3F // R  
#define MPU6050_ACCEL_ZOUT_L       0x40 // R  

#define MPU6050_TEMP_OUT_H         0x41 // R  
#define MPU6050_TEMP_OUT_L         0x42 // R  

#define PORT_CTRL_MASK 0x0F

class Firmware {
public:
      
   static Firmware& instance() {
      return _firmware;
   }

   int exec() {
      char buffer[200] = {0};
      HardwareSerial::instance().write((const uint8_t*)"Smartblock MCU Online\r\n",23);

      uint8_t unXbeeCmdIdx = 0;

      enum class ETestbenchState {
         XBEE_INIT_TX,
         XBEE_INIT_WAIT_FOR_RX,
         MPU6050_INIT,
         TESTBENCH_INIT_DONE,
         TESTBENCH_INIT_FAIL
      } eTestbenchState = ETestbenchState::XBEE_INIT_TX;
 
      struct SRxBuffer {
         uint8_t Buffer[8];
         uint8_t Index;
      } sRxBuffer = {{}, 0};

      const uint8_t *ppunXbeeConfig[] = {
         (const uint8_t*)"+++",
         (const uint8_t*)"ATRE\r\n",
         (const uint8_t*)"ATID 2001\r\n",
         (const uint8_t*)"ATDH 0013A200\r\n",
         (const uint8_t*)"ATDL 40AA1A2C\r\n",
         (const uint8_t*)"ATCN\r\n"};

      for(;;) {
         
         switch(eTestbenchState) {
         case ETestbenchState::XBEE_INIT_TX: 
            /* DEBUG */
            sprintf(buffer, "Sending command: ");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            /* DEBUG */
            for(uint8_t unCmdCharIdx = 0; 
                ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\0'; 
                unCmdCharIdx++) {
               m_cTUARTController.WriteByte(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx]);
               /* DEBUG */
               if(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\n' and
                  ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\r')
                  HardwareSerial::instance().write(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx]);
               /* DEBUG */
            }
            /* DEBUG */
            sprintf(buffer, "\r\n");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            /* DEBUG */
            eTestbenchState = ETestbenchState::XBEE_INIT_WAIT_FOR_RX;
            sRxBuffer.Index = 0;
            continue;
            break;
         case ETestbenchState::XBEE_INIT_WAIT_FOR_RX:
            m_cTimer.Delay(10);
            while(m_cTUARTController.Available()) {
               sRxBuffer.Buffer[sRxBuffer.Index++] = m_cTUARTController.Read();
            }
            /* DEBUG */
            sprintf(buffer, "Waiting for RX, buffer contains (%u): %s\r\n", sRxBuffer.Index, (const char*)sRxBuffer.Buffer);
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            /* DEBUG */

            if(sRxBuffer.Index > 2 &&
               sRxBuffer.Buffer[0] == 'O' &&
               sRxBuffer.Buffer[1] == 'K' &&
               sRxBuffer.Buffer[2] == '\r') {
               /* DEBUG */
               sprintf(buffer, "Got OK\r\n");
               HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
               /* DEBUG */
               if(unXbeeCmdIdx < 5) {
                  /* send next AT command */
                  unXbeeCmdIdx++;
                  eTestbenchState = ETestbenchState::XBEE_INIT_TX;
               }
               else {
                  /* XBEE configured successfully, configure MPU6050 */
                  eTestbenchState = ETestbenchState::MPU6050_INIT;
                  /* DEBUG */
                  sprintf(buffer, "Xbee Configured\r\n");
                  HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
                  /* DEBUG */
               }
            }
            // else if -- condition to catch no Xbee present would go here
            continue;
            break;
         case ETestbenchState::MPU6050_INIT:
            m_cTimer.Delay(10);
            /*
            while(m_cTUARTController.Available()) {
               HardwareSerial::instance().write(m_cTUARTController.Read());
            }
            while(HardwareSerial::instance().available()) {
               m_cTUARTController.WriteByte(HardwareSerial::instance().read());
            }
            */
            sprintf(buffer, "Checking MPU6050 Address (WA)\r\n");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));


            CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
            CTWController::GetInstance().Write(MPU6050_WHOAMI);

            CTWController::GetInstance().EndTransmission(false);

            sprintf(buffer, "Checking MPU6050 Address (RD)\r\n");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));

            CTWController::GetInstance().Read(MPU6050_ADDR, 1, true);

            
            /* Check if the WHOAMI register contains the MPU6050 address value  */
            if(CTWController::GetInstance().Read() == MPU6050_ADDR) {
               sprintf(buffer, "Found MPU6050 Sensor at 0x68!\r\n");
               HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            }
            else {
               eTestbenchState = ETestbenchState::TESTBENCH_INIT_FAIL;
               sprintf(buffer, "Hardware failure! Unexpected response from TWController / MPU6050\r\n");
               HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
               continue;
            }
            
            /* select internal clock, disable sleep/cycle mode, enable temperature sensor*/

            CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
            sprintf(buffer, "Updating Power Management (WA)\r\n");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            CTWController::GetInstance().Write(MPU6050_PWR_MGMT_1);
            sprintf(buffer, "Updating Power Management (WD)\r\n");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            CTWController::GetInstance().Write(0x00);
            CTWController::GetInstance().EndTransmission(true);
            
            eTestbenchState = ETestbenchState::TESTBENCH_INIT_DONE;
            sprintf(buffer, "Testbench Initialised - Press any key on WiFi connection to sample MPU6050\r\n");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            continue;
            break;
         case ETestbenchState::TESTBENCH_INIT_DONE:
            m_cTimer.Delay(500);
            if(m_cTUARTController.Available()) {
               while(m_cTUARTController.Available()) m_cTUARTController.Read();
               /* Sample sensor */
               CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
               CTWController::GetInstance().Write(MPU6050_ACCEL_XOUT_H);
               CTWController::GetInstance().EndTransmission(false);
               CTWController::GetInstance().Read(MPU6050_ADDR, 8, true);
               /* Read the requested 8 bytes */
               uint8_t punRes[8];
               for(uint8_t i = 0; i < 8; i++) {
                  punRes[i] = CTWController::GetInstance().Read();
               }          
               sprintf(buffer, 
                       "Acc[x] = %i\r\n"
                       "Acc[y] = %i\r\n"
                       "Acc[z] = %i\r\n"
                       "Temp = %i\r\n",
                       int16_t((punRes[0] << 8) | punRes[1]),
                       int16_t((punRes[2] << 8) | punRes[3]),
                       int16_t((punRes[4] << 8) | punRes[5]),
                       (int16_t((punRes[6] << 8) | punRes[7]) + 12412) / 340);
               for(uint8_t i = 0; buffer[i] != '\0'; i++) {
                  m_cTUARTController.WriteByte(buffer[i]);
               }
            }
            continue;
            break;
         case ETestbenchState::TESTBENCH_INIT_FAIL:
            m_cTimer.Delay(5000);
            printf(buffer, "Testbench Initisation Failed - Check connections and reboot\r\n");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            continue;
            break;
         } // switch(eTestbenchState)
      } // for(;;)
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
      DDRC |= PORT_CTRL_MASK;
      PORTC &= ~PORT_CTRL_MASK;
      PORTC |= 1 & PORT_CTRL_MASK;
   }

   
   CTimer m_cTimer;
   CTUARTController m_cTUARTController;

   static Firmware _firmware;
};

#endif
