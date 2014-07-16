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
#define LTC2990_ADDR               0x4C

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

/* LTC2990 Registers */
#define LTC2990_STATUS             0x00 // R
#define LTC2990_CONTROL            0x01 // R/W
#define LTC2990_TRIGGER            0x02 // R/W

#define LTC2990_TINT_H             0x04 // R
#define LTC2990_TINT_L             0x05 // R
#define LTC2990_V1_H               0x06 // R
#define LTC2990_V1_L               0x07 // R
#define LTC2990_V2_H               0x08 // R
#define LTC2990_V2_L               0x09 // R
#define LTC2990_V3_H               0x0A // R
#define LTC2990_V3_L               0x0B // R
#define LTC2990_V4_H               0x0C // R
#define LTC2990_V4_L               0x0D // R
#define LTC2990_VCC_H              0x0E // R
#define LTC2990_VCC_L              0x0F // R


#define PORT_CTRL_MASK 0x0F

class Firmware {
public:
      
   static Firmware& instance() {
      return _firmware;
   }

   int exec() {
      bool bDataValid = false;
      bool bAlarmASet = false;
      bool bAlarmBSet = false;

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

            /* configure the LTC2990 - battery current and voltage monitoring */
            CTWController::GetInstance().BeginTransmission(LTC2990_ADDR);
            CTWController::GetInstance().Write(LTC2990_CONTROL);
            /* configure LTC2990, perform all measurements when triggered,
               in differential voltage mode, temperature in celsius */
            CTWController::GetInstance().Write(0x5E);
            CTWController::GetInstance().EndTransmission(true);
            
            eTestbenchState = ETestbenchState::TESTBENCH_INIT_DONE;
            sprintf(buffer, "Testbench Initialised!\r\n");
            HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            continue;
            break;
         case ETestbenchState::TESTBENCH_INIT_DONE:
            m_cTimer.Delay(500);
            /* Print the uptime to the Xbee port */
            sprintf(buffer, "Uptime = %lu\r\n", m_cTimer.GetMilliseconds());
            for(uint8_t i = 0; buffer[i] != '\0'; i++) {
               m_cTUARTController.WriteByte(buffer[i]);
            }

            /* Trigger battery voltage and current measurement */
            CTWController::GetInstance().BeginTransmission(LTC2990_ADDR);
            CTWController::GetInstance().Write(LTC2990_TRIGGER);
            CTWController::GetInstance().Write(0x00);
            CTWController::GetInstance().EndTransmission(true);
            /* Wait 50ms for conversion to finish */
            m_cTimer.Delay(50);
            /* Read back results */
            CTWController::GetInstance().BeginTransmission(LTC2990_ADDR);
            CTWController::GetInstance().Write(LTC2990_TINT_H);
            CTWController::GetInstance().EndTransmission(false);
            CTWController::GetInstance().Read(LTC2990_ADDR, 12, true);

            /* Read the requested 10 bytes */
            uint8_t punLTC2990Res[12];
            for(uint8_t i = 0; i < 12; i++) {
               punLTC2990Res[i] = CTWController::GetInstance().Read();
            }
            
            /*******************************************/
            /* Dump the results on the Xbee connection */
            /*******************************************/
            // Internal temperature
            bDataValid = ((punLTC2990Res[0] & 0x80) != 0);
            bAlarmASet = ((punLTC2990Res[0] & 0x40) != 0);
            bAlarmBSet = ((punLTC2990Res[0] & 0x20) != 0);
            // remove flags with sign extend
            if((punLTC2990Res[0] & 0x10) != 0)
               punLTC2990Res[0] |= 0xE0;
            else
               punLTC2990Res[0] &= ~0xE0;
            // print result
            sprintf(buffer, 
                    "[%c%c%c] Temp = %i\r\n",
                    bDataValid?'V':'-', 
                    bAlarmASet?'A':'-',
                    bAlarmBSet?'B':'-',
                    int16_t((punLTC2990Res[0] << 8) | punLTC2990Res[1]));                                    
            for(uint8_t i = 0; buffer[i] != '\0'; i++) {
               m_cTUARTController.WriteByte(buffer[i]);
            }
            // V1 - V2 Verify A
            bDataValid = ((punLTC2990Res[2] & 0x80) != 0);
            // Remove flags with sign extend
            if((punLTC2990Res[2] & 0x40) != 0)
               punLTC2990Res[2] |= 0x80;
            else
               punLTC2990Res[2] &= ~0x80;
            sprintf(buffer, 
                    "[%c] (A) I(batt) = %imA\r\n",
                    bDataValid?'V':'-', 
                    int16_t((punLTC2990Res[2] << 8) | punLTC2990Res[3]) / 3);                                    
            for(uint8_t i = 0; buffer[i] != '\0'; i++) {
               m_cTUARTController.WriteByte(buffer[i]);
            }
            // V1 - V2 Verify B
            bDataValid = ((punLTC2990Res[4] & 0x80) != 0);
            // Remove flags with sign extend
            if((punLTC2990Res[4] & 0x40) != 0)
               punLTC2990Res[4] |= 0x80;
            else
               punLTC2990Res[4] &= ~0x80;
            sprintf(buffer, 
                    "[%c] (B) I(batt) = %imA\r\n",
                    bDataValid?'V':'-', 
                    int16_t((punLTC2990Res[4] << 8) | punLTC2990Res[5]) / 3);                                    
            for(uint8_t i = 0; buffer[i] != '\0'; i++) {
               m_cTUARTController.WriteByte(buffer[i]);
            }
            // V3 - V4 Verify A
            bDataValid = ((punLTC2990Res[6] & 0x80) != 0);
            // Remove flags with sign extend
            if((punLTC2990Res[6] & 0x40) != 0)
               punLTC2990Res[6] |= 0x80;
            else
               punLTC2990Res[6] &= ~0x80;
            sprintf(buffer, 
                    "[%c] (A) V(batt) = %imV\r\n",
                    bDataValid?'V':'-', 
                    int16_t((punLTC2990Res[6] << 8) | punLTC2990Res[7]) / 3678);                                    
            for(uint8_t i = 0; buffer[i] != '\0'; i++) {
               m_cTUARTController.WriteByte(buffer[i]);
            }
            // V3 - V4 Verify B
            bDataValid = ((punLTC2990Res[8] & 0x80) != 0);
            // Remove flags with sign extend
            if((punLTC2990Res[8] & 0x40) != 0)
               punLTC2990Res[8] |= 0x80;
            else
               punLTC2990Res[8] &= ~0x80;
            sprintf(buffer, 
                    "[%c] (B) V(batt) = %imV\r\n",
                    bDataValid?'V':'-', 
                    int16_t((punLTC2990Res[8] << 8) | punLTC2990Res[9]) / 3678);                                    
            for(uint8_t i = 0; buffer[i] != '\0'; i++) {
               m_cTUARTController.WriteByte(buffer[i]);
            }
            // VCC
            bDataValid = ((punLTC2990Res[10] & 0x80) != 0);
            // Remove flags with sign extend
            if((punLTC2990Res[10] & 0x40) != 0)
               punLTC2990Res[10] |= 0x80;
            else
               punLTC2990Res[10] &= ~0x80;
            sprintf(buffer, 
                    "[%c] (B) V(d) = %imV\r\n",
                    bDataValid?'V':'-', 
                    2500 + int16_t((punLTC2990Res[10] << 8) | punLTC2990Res[11]) / 3277);                                    
            for(uint8_t i = 0; buffer[i] != '\0'; i++) {
               m_cTUARTController.WriteByte(buffer[i]);
            }
            
            /* delay 50ms to clear the TUART buffers - perhaps not required */
            m_cTimer.Delay(50);

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
