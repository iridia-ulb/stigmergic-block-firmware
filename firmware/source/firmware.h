#ifndef FIRMWARE_H
#define FIRMWARE_H

//#define DEBUG

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
#include <nfc_controller.h>
#include <port_controller.h>
#include <timer.h>

/* I2C Address Space */
#define MPU6050_ADDR               0x68
#define LTC2990_ADDR               0x4C
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

   CTimer& GetTimer() {
      return m_cTimer;
   }

   int Exec() {

      enum class ETestbenchState {
         TEST_LEDS,
         TEST_TIMER,
         TEST_ACCELEROMETER,
         TEST_NFC_TX,
         TEST_NFC_RX,
         STANDBY
      } eTestbenchState = ETestbenchState::STANDBY;

      /* correctly connected LEDs for testing */
      uint8_t punLEDIdx[6] = {0,1,2, 12, 11, 10};

      // NFC related stuff
      uint8_t punOutboundBuffer[] = {'S','M','A','R','T','B','L','O','C','K','\0'};
      uint8_t punInboundBuffer[60];
      uint8_t unRxCount = 0;

      /* Array for holding accelerometer result */
      uint8_t punMPU6050Res[8];

      fprintf(m_psHUART, "Init...");

      InitXbee();

      /* Note: due to board issue, this actually disables the LEDs - 
         although TW should still work, disable only disables the outputs */
      m_cPortController.EnablePort(CPortController::EPort::EAST);
      m_cPortController.SelectPort(CPortController::EPort::EAST);
      InitPCA9635();

      fprintf(m_psHUART, "D...");
      m_cPortController.DisablePort(CPortController::EPort::SOUTH);
      m_cTimer.Delay(1000);
      fprintf(m_psHUART, "E...");
      m_cPortController.EnablePort(CPortController::EPort::SOUTH);
      m_cTimer.Delay(1000);
      fprintf(m_psHUART, "S...");
      m_cPortController.SelectPort(CPortController::EPort::SOUTH);
      while(!InitPN532()) {
         m_cTimer.Delay(500);
         fprintf(m_psHUART, ".");
      }

      fprintf(m_psHUART, "Done\r\n");
 

      for(;;) {
         if(Firmware::GetInstance().GetTUARTController().Available()) {
            uint8_t unInput = Firmware::GetInstance().GetTUARTController().Read();
            switch(unInput) {
            case 'x':
               eTestbenchState = ETestbenchState::TEST_NFC_TX;
               break;
            case 'l':
               eTestbenchState = ETestbenchState::TEST_LEDS;
               break;
            case 't':
               eTestbenchState = ETestbenchState::TEST_TIMER;
               break;
            case 'a':
               eTestbenchState = ETestbenchState::TEST_ACCELEROMETER;
               break;
            default:
               eTestbenchState = ETestbenchState::STANDBY;
               break;
            }
            Firmware::GetInstance().GetTUARTController().FlushInput();
         }
         switch(eTestbenchState) {
         case ETestbenchState::STANDBY:
            m_cPortController.SynchronizeInterrupts();
            if(m_cPortController.HasInterrupts()) {
               fprintf(m_psTUART, "INT = 0x%02x\r\n", m_cPortController.GetInterrupts());
               m_cPortController.ClearInterrupts();
               eTestbenchState = ETestbenchState::TEST_NFC_RX;
            }
            continue;
            break;
         case ETestbenchState::TEST_TIMER:
            fprintf(m_psTUART, "Uptime = %lums\r\n", m_cTimer.GetMilliseconds());            
            eTestbenchState = ETestbenchState::STANDBY;
            continue;
            break;
         case ETestbenchState::TEST_LEDS:
            m_cPortController.SelectPort(CPortController::EPort::EAST);
            /* issue - enable port */
            m_cPortController.DisablePort(CPortController::EPort::EAST);
            for(uint8_t un_led_idx = 0; un_led_idx < 6; un_led_idx++) {
               for(uint8_t un_val = 0x00; un_val < 0xFF; un_val++) {
                  m_cTimer.Delay(1);
                  PCA9635_SetLEDBrightness(punLEDIdx[un_led_idx], un_val);
               }
               for(uint8_t un_val = 0xFF; un_val > 0x00; un_val--) {
                  m_cTimer.Delay(1);
                  PCA9635_SetLEDBrightness(punLEDIdx[un_led_idx], un_val);
               }
            }
            /* issue - disable port */
            m_cPortController.EnablePort(CPortController::EPort::EAST);
            eTestbenchState = ETestbenchState::STANDBY;
            continue;
            break;
         case ETestbenchState::TEST_ACCELEROMETER:
            Firmware::GetInstance().GetTWController().BeginTransmission(MPU6050_ADDR);
            Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EMPU6050Register::ACCEL_XOUT_H));
            Firmware::GetInstance().GetTWController().EndTransmission(false);
            Firmware::GetInstance().GetTWController().Read(MPU6050_ADDR, 8, true);
            /* Read the requested 8 bytes */
            for(uint8_t i = 0; i < 8; i++) {
               punMPU6050Res[i] = Firmware::GetInstance().GetTWController().Read();
            }
            fprintf(m_psTUART, 
                    "Acc[x] = %i\r\n"
                    "Acc[y] = %i\r\n"
                    "Acc[z] = %i\r\n"
                    "Temp = %i\r\n",
                    int16_t((punMPU6050Res[0] << 8) | punMPU6050Res[1]),
                    int16_t((punMPU6050Res[2] << 8) | punMPU6050Res[3]),
                    int16_t((punMPU6050Res[4] << 8) | punMPU6050Res[5]),
                    (int16_t((punMPU6050Res[6] << 8) | punMPU6050Res[7]) + 12412) / 340);
            eTestbenchState = ETestbenchState::STANDBY;
            continue;
            break;
         case ETestbenchState::TEST_NFC_TX:
            fprintf(m_psTUART, "\r\nTesting NFC TX\r\n");           
            m_cPortController.SelectPort(CPortController::EPort::SOUTH);
            unRxCount = 0;
            for(uint8_t cnt = 0; cnt < 100; cnt++) {
               fprintf(m_psTUART, ".");
               if(m_cNFCController.P2PInitiatorInit()) {
                  fprintf(m_psTUART, "\r\nConnected!\r\n");
                  unRxCount = m_cNFCController.P2PInitiatorTxRx(punOutboundBuffer,
                                                                11,
                                                                punInboundBuffer,
                                                                60);
                  if(unRxCount > 0) {
                     fprintf(m_psTUART, "Received %i bytes: ", unRxCount);
                     for(uint8_t i = 0; i < unRxCount; i++) {
                        fprintf(m_psTUART, "%c", punInboundBuffer[i]);
                     }
                     break;
                  }
                  else {
                     fprintf(m_psTUART, "No data\r\n");
                  }
               }
               m_cTimer.Delay(100);
            }
            m_cNFCController.PowerDown();
            eTestbenchState = ETestbenchState::STANDBY;
            continue;
            break;
         case ETestbenchState::TEST_NFC_RX:
            fprintf(m_psTUART, "\r\nTesting NFC RX\r\n");
            m_cPortController.SelectPort(CPortController::EPort::SOUTH);
            unRxCount = 0;
            for(uint8_t cnt = 0; cnt < 100; cnt++) {
               fprintf(m_psTUART, ".");
               if(m_cNFCController.P2PTargetInit()) {
                  unRxCount = m_cNFCController.P2PInitiatorTxRx(punOutboundBuffer,
                                                                11,
                                                                punInboundBuffer,
                                                                60);
                  if(unRxCount > 0) {
                     fprintf(m_psTUART, "Received %i bytes: ", unRxCount);
                     for(uint8_t i = 0; i < unRxCount; i++) {
                        fprintf(m_psTUART, "%c", punInboundBuffer[i]);
                     }
                     break;
                  }
                  else {
                     fprintf(m_psTUART, "No data\r\n");
                  }
               }
               m_cTimer.Delay(100);
            }
            m_cNFCController.PowerDown();
            eTestbenchState = ETestbenchState::STANDBY;
            continue;
            break;
         }
      }
      return 0;
   }
      
private:

   bool InitXbee();
   bool InitMPU6050();

   bool InitPCA9635();
   bool InitPN532();

   /* LED commands */
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

      m_cPortController.Init();

      // configure the BQ24075 monitoring pins
      DDRD &= ~PWR_MON_MASK;  // set as input
      PORTD &= ~PWR_MON_MASK; // disable pull ups
   }
   
   CTimer m_cTimer;

   CPortController m_cPortController;

   /* ATMega328P Controllers */
   /* TODO remove singleton and reference from HUART */
   //CHUARTController& m_cHUARTController;
   HardwareSerial& m_cHUARTController;
   
   CTUARTController m_cTUARTController;

   CTWController& m_cTWController;

   CNFCController m_cNFCController;

   static Firmware _firmware;

public: // TODO, don't make these public
    /* File structs for fprintf */
   FILE* m_psTUART;
   FILE* m_psHUART;



};

#endif
