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
#include <timer.h>

#define NUM_PORTS 6

/* I2C Address Space */
#define MPU6050_ADDR               0x68

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
      m_psOutputUART = ps_huart;
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

   int Exec();
      
private:

   bool bHasXbee;

   bool InitXbee();
   bool InitMPU6050();

   bool InitPCA9635();
   bool InitPN532();

   void DetectPorts();
   const char* GetPortString(CPortController::EPort ePort);

   void HardwareTestMode();
   void InteractiveMode();

   /* Test Routines */
   void TestAccelerometer();
   void TestPMIC();
   void TestLEDs();
   bool TestNFCTx();
   bool TestNFCRx();

   struct CBlockLEDRoutines {
      static void SetAllColorsOnFace(uint8_t unRed, uint8_t unGreen, uint8_t unBlue);
      static void SetAllModesOnFace(CLEDController::EMode eMode);
   };

   /* Reset */
   void Reset();

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
      m_cPortController(),
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
      m_cTWController(CTWController::GetInstance()) {}
   
   CTimer m_cTimer;

   CPortController m_cPortController;

   CPortController::EPort m_peAllPorts[NUM_PORTS] {
      CPortController::EPort::NORTH,
      CPortController::EPort::EAST,
      CPortController::EPort::SOUTH,
      CPortController::EPort::WEST,
      CPortController::EPort::TOP,
      CPortController::EPort::BOTTOM,
   };

   CPortController::EPort m_peConnectedPorts[NUM_PORTS] {
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
   };



   /* ATMega328P Controllers */
   /* TODO remove singleton and reference from HUART */
   //CHUARTController& m_cHUARTController;
   HardwareSerial& m_cHUARTController;
   
   CTUARTController m_cTUARTController;

   CTWController& m_cTWController;

   CNFCController m_cNFCController;

   CADCController m_cADCController;


   static Firmware _firmware;
   
public: // TODO, don't make these public
    /* File structs for fprintf */
   FILE* m_psTUART;
   FILE* m_psHUART;
   FILE* m_psOutputUART;

};

#endif
