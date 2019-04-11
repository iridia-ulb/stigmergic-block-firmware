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
#include <led_controller.h>
#include <port_controller.h>
#include <adc_controller.h>
#include <clock.h>

#define NUM_PORTS 6

#define PWR_MON_MASK   0xC0
#define PWR_MON_PGOOD  0x40
#define PWR_MON_CHG    0x80

#define PORT_CTRL_MASK 0x0F

#define XBEE_RST_PIN   0x20


class CFirmware {
public:
      
   static CFirmware& GetInstance() {
      static CFirmware cInstance;
      return cInstance;
   }

   void SetFilePointers(FILE* ps_huart, FILE* ps_tuart) {
      m_psHUART = ps_huart;
      m_psTUART = ps_tuart;
      m_psOutputUART = ps_huart;
   }

   int Exec();
      
private:

   /* private constructor */
   CFirmware() = default;

   bool bHasXbee;

   bool InitXbee();
   bool InitMPU6050();

   bool InitPCA9635();

   void DetectPorts();
   const char* GetPortString(CPortController::EPort ePort);

   void InteractiveMode();

   /* Test Routines */
   void TestAccelerometer();
   void TestPMIC();

   /* Reset */
   void Reset();

   struct SRxDetector : CNFCController::SRxFunctor {     
      virtual void operator()(const uint8_t* pun_data, uint8_t un_length) {
         LastRxTime = CClock::GetInstance().GetMilliseconds();
         Messages++;
      }
      uint32_t LastRxTime = 0;
      uint16_t Messages = 0;
   };

   struct SFace {
      CPortController::EPort Port;
      bool Connected;
      SRxDetector RxDetector;
      CNFCController NFC;
   };

   SFace m_psFaces[6] {
      { CPortController::EPort::NORTH },
      { CPortController::EPort::EAST },
      { CPortController::EPort::SOUTH },
      { CPortController::EPort::WEST },
      { CPortController::EPort::TOP },
      { CPortController::EPort::BOTTOM },
   };

   void Debug(const SFace& s_face);

public: 
   /* TODO remove fprintf, stdio etc, replace with operator<< */
   FILE* m_psTUART;
   FILE* m_psHUART;
   FILE* m_psOutputUART;

};

#endif
