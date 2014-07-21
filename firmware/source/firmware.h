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
#define PCA9554_RST_ADDR           0x20
#define PCA9554_IRQ_ADDR           0x21
#define PCA9635_ADDR               0x15
#define PCA9635_RST                0x03

enum class EMPU6050Register : uint8_t {
   /* MPU6050 Registers */
   PWR_MGMT_1     = 0x6B, // R/W
   PWR_MGMT_2     = 0x6C, // R/W
   ACCEL_XOUT_H   = 0x3B, // R  
   ACCEL_XOUT_L   = 0x3C, // R  
   ACCEL_YOUT_H   = 0x3D, // R  
   ACCEL_YOUT_L   = 0x3E, // R  
   ACCEL_ZOUT_H   = 0x3F, // R  
   ACCEL_ZOUT_L   = 0x40, // R  
   TEMP_OUT_H     = 0x41, // R  
   TEMP_OUT_L     = 0x42, // R  
   WHOAMI         = 0x75  // R
};

enum class ELTC2990Register : uint8_t {
   /* LTC2990 Registers */
   STATUS         = 0x00, // R
   CONTROL        = 0x01, // R/W
   TRIGGER        = 0x02, // R/W
   TINT_H         = 0x04, // R
   TINT_L         = 0x05, // R
   V1_H           = 0x06, // R
   V1_L           = 0x07, // R
   V2_H           = 0x08, // R
   V2_L           = 0x09, // R
   V3_H           = 0x0A, // R
   V3_L           = 0x0B, // R
   V4_H           = 0x0C, // R
   V4_L           = 0x0D, // R
   VCC_H          = 0x0E, // R
   VCC_L          = 0x0F  // R
};

enum class EPCA9554Register : uint8_t {
   /* PCA9554 Registers */
   INPUT          = 0x00, // R
   OUTPUT         = 0x01, // R/W
   CONFIG         = 0x03  // R/W
};

enum class EPCA9635LEDMode : uint8_t {
   OFF  = 0x00,
   ON   = 0x01,
   PWM  = 0x02
};

#define LEDOUTX_MASK  0x03

enum class EPCA9635Register : uint8_t {
   MODE1          = 0x00, // R/W
   MODE2          = 0x01, // R/W
   PWM0           = 0x02, // R/W
   PWM1           = 0x03, // R/W
   PWM2           = 0x04, // R/W
   PWM3           = 0x05, // R/W
   PWM4           = 0x06, // R/W
   PWM5           = 0x07, // R/W
   PWM6           = 0x08, // R/W
   PWM7           = 0x09, // R/W
   PWM8           = 0x0A, // R/W
   PWM9           = 0x0B, // R/W
   PWM10          = 0x0C, // R/W
   PWM11          = 0x0D, // R/W
   PWM12          = 0x0E, // R/W
   PWM13          = 0x0F, // R/W
   PWM14          = 0x10, // R/W
   PWM15          = 0x11, // R/W
   GRPPWM         = 0x12, // R/W
   GRPFREQ        = 0x13, // R/W
   LEDOUT0        = 0x14, // R/W
   LEDOUT1        = 0x15, // R/W
   LEDOUT2        = 0x16, // R/W
   LEDOUT3        = 0x17, // R/W
   SUBADR1        = 0x18, // R/W
   SUBADR2        = 0x19, // R/W
   SUBADR3        = 0x1A, // R/W
   ALLCALLADR     = 0x1B // R/W
};

#define PWR_MON_MASK   0xC0
#define PWR_MON_PGOOD  0x40
#define PWR_MON_CHG    0x80

#define PORT_CTRL_MASK 0x0F

#define XBEE_RST_PIN   0x20

enum class EFaceBoard : uint8_t {
   NORTH  = 0x01,
   EAST   = 0x02,
   SOUTH  = 0x04,
   WEST   = 0x08,
   TOP    = 0x10,
   BOTTOM = 0x20
};

class Firmware {
public:
      
   static Firmware& GetInstance() {
      return _firmware;
   }

   void SetFilePointers(FILE* ps_huart, FILE* ps_tuart) {
      m_psHUART = ps_huart;
      m_psTUART = ps_tuart;
   }

   CTUARTController& GetTUARTController() {
      return m_cTUARTController;
   }

   /*
   CHUARTController& GetHUARTController() {
      return m_cHUARTController;
   }
   */
   HardwareSerial& GetHUARTController() {
      return m_cHUARTController;
   }

   CTWController& GetTWController() {
      return m_cTWController;
   }

   int Exec() {
      bool bDataValid, bAlarmASet, bAlarmBSet;

      enum class ETestbenchState {
         TEST_ACCELEROMETER,
         TEST_BATT_I_V,
         TEST_WIFI,
         TEST_FACE_RESET,
         TEST_LEDS,
      } eTestbenchState = ETestbenchState::TEST_LEDS;

      //InitXbee();
      InitMPU6050();
      InitLTC2990();
      InitPCA9554_RST();
      InitPCA9554_IRQ();
      InitPCA9635();

      IssueFaceReset(uint8_t(EFaceBoard::EAST));

      fprintf(m_psHUART, "Testbench Initialisation : SUCCESS\r\n");

      /* Variables for holding results */
      uint8_t punLTC2990Res[12];
      uint8_t punMPU6050Res[8];

      for(;;) {
         switch(eTestbenchState) {
         case ETestbenchState::TEST_ACCELEROMETER:
            Firmware::GetInstance().GetTWController().BeginTransmission(MPU6050_ADDR);
            Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EMPU6050Register::ACCEL_XOUT_H));
            Firmware::GetInstance().GetTWController().EndTransmission(false);
            Firmware::GetInstance().GetTWController().Read(MPU6050_ADDR, 8, true);
            /* Read the requested 8 bytes */
            for(uint8_t i = 0; i < 8; i++) {
               punMPU6050Res[i] = Firmware::GetInstance().GetTWController().Read();
            }          
            eTestbenchState = ETestbenchState::TEST_BATT_I_V;
            continue;
            break;
         case ETestbenchState::TEST_BATT_I_V:
            Firmware::GetInstance().GetTWController().BeginTransmission(LTC2990_ADDR);
            Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(ELTC2990Register::TRIGGER));
            Firmware::GetInstance().GetTWController().Write(0x00);
            Firmware::GetInstance().GetTWController().EndTransmission(true);
            /* Wait 50ms for conversion to finish */
            m_cTimer.Delay(50);
            /* Read back results */
            Firmware::GetInstance().GetTWController().BeginTransmission(LTC2990_ADDR);
            Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(ELTC2990Register::TINT_H));
            Firmware::GetInstance().GetTWController().EndTransmission(false);
            Firmware::GetInstance().GetTWController().Read(LTC2990_ADDR, 12, true);
            /* Read the requested 12 bytes */
            for(uint8_t i = 0; i < 12; i++) {
               punLTC2990Res[i] = Firmware::GetInstance().GetTWController().Read();
            }
            eTestbenchState = ETestbenchState::TEST_WIFI;
            continue;
            break;
         case ETestbenchState::TEST_WIFI:
            fprintf(m_psTUART, "Uptime = %lu\r\n", m_cTimer.GetMilliseconds());

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
            fprintf(m_psTUART, 
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
            fprintf(m_psTUART, 
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
            fprintf(m_psTUART, 
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
            fprintf(m_psTUART, 
                    "[%c] V(dd) = %limV\r\n",
                    bDataValid?'V':'-', 
                    2500 + (int16_t((punLTC2990Res[10] << 8) | punLTC2990Res[11]) * 305180) / 1000000);

            // Display PGOOD and CHG pin logic state  */
            fprintf(m_psTUART, 
                    "PGOOD = %c || CHG = %c\r\n",
                    (PIND & PWR_MON_PGOOD)?'F':'T',
                    (PIND & PWR_MON_CHG)?'F':'T');

            fprintf(m_psTUART, 
                    "Acc[x] = %i\r\n"
                    "Acc[y] = %i\r\n"
                    "Acc[z] = %i\r\n"
                    "Temp = %i\r\n",
                    int16_t((punMPU6050Res[0] << 8) | punMPU6050Res[1]),
                    int16_t((punMPU6050Res[2] << 8) | punMPU6050Res[3]),
                    int16_t((punMPU6050Res[4] << 8) | punMPU6050Res[5]),
                    (int16_t((punMPU6050Res[6] << 8) | punMPU6050Res[7]) + 12412) / 340);

            eTestbenchState = ETestbenchState::TEST_FACE_RESET;
            continue;
            break;
         case ETestbenchState::TEST_FACE_RESET:
            fprintf(m_psTUART, "Asserting reset on EAST face\r\n");
            IssueFaceReset(uint8_t(EFaceBoard::EAST));
            eTestbenchState = ETestbenchState::TEST_LEDS;
            continue;
            break;
         case ETestbenchState::TEST_LEDS:
            uint8_t punLEDIdx[] = {0,1,2, 12, 11, 10};
            for(uint8_t un_led_idx = 0; un_led_idx < 6; un_led_idx++) {
               for(uint8_t un_val = 0x00; un_val < 0xFF; un_val++) {
                  m_cTimer.Delay(5);
                  PCA9635_SetLEDBrightness(punLEDIdx[un_led_idx], un_val);
               }
               for(uint8_t un_val = 0xFF; un_val > 0x00; un_val--) {
                  m_cTimer.Delay(5);
                  PCA9635_SetLEDBrightness(punLEDIdx[un_led_idx], un_val);
               }
            }
            eTestbenchState = ETestbenchState::TEST_ACCELEROMETER;
            continue;
            break;

         }
      }
      return 0;
   }
      
private:

   bool InitXbee();
   bool InitMPU6050();
   bool InitLTC2990();

   bool InitPCA9554_RST();
   bool InitPCA9554_IRQ();

   bool InitPCA9635();

   /* Faceboard commands */
   void IssueFaceReset(uint8_t un_reset_target);
   void PCA9635_SetLEDMode(uint8_t un_led, EPCA9635LEDMode e_mode);
   void PCA9635_SetLEDBrightness(uint8_t un_led, uint8_t un_val);


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
      m_cHUARTController(HardwareSerial::instance()),
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
                         TIMER1_COMPB_vect_num),
      m_cTWController(CTWController::GetInstance()) {    

      /* Enable interrupts */
      sei();

      // select channel 2 for i2c output 
      DDRC |= PORT_CTRL_MASK;
      PORTC &= ~PORT_CTRL_MASK;
      PORTC |= 1 & PORT_CTRL_MASK; // DISABLE

      // configure the BQ24075 monitoring pins
      DDRD &= ~PWR_MON_MASK;  // set as input
      PORTD &= ~PWR_MON_MASK; // disable pull ups
   }
   
   CTimer m_cTimer;

   /* File structs for fprintf */
   FILE* m_psHUART;
   FILE* m_psTUART;

   /* ATMega328P Controllers */
   /* TODO remove singleton and reference from HUART */
   //CHUARTController& m_cHUARTController;
   HardwareSerial& m_cHUARTController;
   
   CTUARTController m_cTUARTController;

   CTWController& m_cTWController;

   static Firmware _firmware;
};

#endif
