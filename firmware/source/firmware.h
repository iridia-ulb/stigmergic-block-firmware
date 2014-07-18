#ifndef FIRMWARE_H
#define FIRMWARE_H

#define DEBUG

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

#define PWR_MON_MASK   0xC0
#define PWR_MON_PGOOD  0x40
#define PWR_MON_CHG    0x80

#define PORT_CTRL_MASK 0x0F

class Firmware {
public:
      
   static Firmware& instance() {
      return _firmware;
   }

   bool InitXbee();
   bool InitMPU6050();
   bool InitLTC2990();

   CTUARTController& GetTUARTController() {
      return m_cTUARTController;
   }

   void SetFilePointers(FILE* pf_huart, FILE* pf_tuart) {
      huart = pf_huart;
      tuart = pf_tuart;
   }

   int exec() {
      bool bDataValid, bAlarmASet, bAlarmBSet;

      enum class ETestbenchState {
         TEST_ACCELOMETER,
         TEST_BATT_I_V,
         TEST_WIFI,        
      } eTestbenchState = ETestbenchState::TEST_ACCELOMETER;

      InitXbee();
      InitMPU6050();
      InitLTC2990();

      fprintf(huart, "Testbench Initialisation : SUCCESS\r\n");

      /* Variables for holding results */
      uint8_t punLTC2990Res[12];
      uint8_t punMPU6050Res[8];

      for(;;) {
         switch(eTestbenchState) {
         case ETestbenchState::TEST_ACCELOMETER:
            CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
            CTWController::GetInstance().Write(MPU6050_ACCEL_XOUT_H);
            CTWController::GetInstance().EndTransmission(false);
            CTWController::GetInstance().Read(MPU6050_ADDR, 8, true);
            /* Read the requested 8 bytes */
            for(uint8_t i = 0; i < 8; i++) {
               punMPU6050Res[i] = CTWController::GetInstance().Read();
            }          
            eTestbenchState = ETestbenchState::TEST_BATT_I_V;
            continue;
            break;
         case ETestbenchState::TEST_BATT_I_V:
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
            /* Read the requested 12 bytes */
            for(uint8_t i = 0; i < 12; i++) {
               punLTC2990Res[i] = CTWController::GetInstance().Read();
            }
            eTestbenchState = ETestbenchState::TEST_WIFI;
            continue;
            break;
         case ETestbenchState::TEST_WIFI:
            fprintf(tuart, "Uptime = %lu\r\n", m_cTimer.GetMilliseconds());

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
            fprintf(tuart, 
                    "[%c%c%c] Temp = %i\r\n",
                    bDataValid?'V':'-', 
                    bAlarmASet?'A':'-',
                    bAlarmBSet?'B':'-',
                    int16_t((punLTC2990Res[0] << 8) | punLTC2990Res[1]) / 16);                                    
            // V1 - V2
            bDataValid = ((punLTC2990Res[2] & 0x80) != 0);
            // Remove flags with sign extend
            if((punLTC2990Res[2] & 0x40) != 0)
               punLTC2990Res[2] |= 0x80;
            else
               punLTC2990Res[2] &= ~0x80;
            fprintf(tuart, 
                    "[%c] I(batt) = %limA\r\n",
                    bDataValid?'V':'-', 
                    (int16_t((punLTC2990Res[2] << 8) | punLTC2990Res[3]) * 323666) / 1000000);
            // V3 - V4
            bDataValid = ((punLTC2990Res[6] & 0x80) != 0);
            // Remove flags with sign extend
            if((punLTC2990Res[6] & 0x40) != 0)
               punLTC2990Res[6] |= 0x80;
            else
               punLTC2990Res[6] &= ~0x80;
            fprintf(tuart, 
                    "[%c] V(batt) = %limV\r\n",
                    bDataValid?'V':'-', 
                    (int16_t((punLTC2990Res[6] << 8) | punLTC2990Res[7]) * 271880) / 1000000);
            
            // VCC
            bDataValid = ((punLTC2990Res[10] & 0x80) != 0);
            // Remove flags with sign extend
            if((punLTC2990Res[10] & 0x40) != 0)
               punLTC2990Res[10] |= 0x80;
            else
               punLTC2990Res[10] &= ~0x80;
            fprintf(tuart, 
                    "[%c] V(dd) = %limV\r\n",
                    bDataValid?'V':'-', 
                    2500 + (int16_t((punLTC2990Res[10] << 8) | punLTC2990Res[11]) * 305180) / 1000000);

            // Display PGOOD and CHG pin logic state  */
            fprintf(tuart, 
                    "PGOOD = %c || CHG = %c\r\n",
                    (PIND & PWR_MON_PGOOD)?'F':'T',
                    (PIND & PWR_MON_CHG)?'F':'T');

            fprintf(tuart, 
                    "Acc[x] = %i\r\n"
                    "Acc[y] = %i\r\n"
                    "Acc[z] = %i\r\n"
                    "Temp = %i\r\n",
                    int16_t((punMPU6050Res[0] << 8) | punMPU6050Res[1]),
                    int16_t((punMPU6050Res[2] << 8) | punMPU6050Res[3]),
                    int16_t((punMPU6050Res[4] << 8) | punMPU6050Res[5]),
                    (int16_t((punMPU6050Res[6] << 8) | punMPU6050Res[7]) + 12412) / 340);

            eTestbenchState = ETestbenchState::TEST_ACCELOMETER;
            continue;
            break;
         }
      }
      return 0;
   }
      
private:

   FILE * huart;
   FILE * tuart;


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
      PORTC |= 0x08 & PORT_CTRL_MASK; // DISABLE

      // configure the BQ24075 monitoring pins
      DDRD &= ~PWR_MON_MASK;  // set as input
      PORTD &= ~PWR_MON_MASK; // disable pull ups
   }
   
   CTimer m_cTimer;
   CTUARTController m_cTUARTController;

   static Firmware _firmware;
};

#endif
