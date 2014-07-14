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
      char buffer[200] = {0};
      HardwareSerial::instance().write((const uint8_t*)"Smartblock MCU Online\r\n",23);

      uint8_t unXbeeCmdIdx = 0;

      enum class EConfigState {
         CONFIG_TX,
            CONFIG_WAIT_FOR_RX,
            CONN_ESTABLISHED
      } eConfigState = EConfigState::CONFIG_TX;
      
      // EConfigState eConfigState = EConfigState::CONFIG_TX;

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
         
         switch(eConfigState) {
         case EConfigState::CONFIG_TX: 
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
            eConfigState = EConfigState::CONFIG_WAIT_FOR_RX;
            sRxBuffer.Index = 0;
            continue;
            break;
         case EConfigState::CONFIG_WAIT_FOR_RX:
            m_cTimer.Delay(1000);
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
                  eConfigState = EConfigState::CONFIG_TX;
               }
               else {
                  /* all commands sent, enter connection established mode */
                  eConfigState = EConfigState::CONN_ESTABLISHED;
                  /* DEBUG */
                  sprintf(buffer, "Connection Established\r\n");
                  HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
                  /* DEBUG */
               }
            }
            // else if -- condition to catch no Xbee present would go here
            continue;
            break;
         case EConfigState::CONN_ESTABLISHED:
            m_cTimer.Delay(10);
            while(m_cTUARTController.Available()) {
               HardwareSerial::instance().write(m_cTUARTController.Read());
            }
            while(HardwareSerial::instance().available()) {
               m_cTUARTController.WriteByte(HardwareSerial::instance().read());
            }
            
#if 0            
            m_cTimer.Delay(500);
            /* DEBUG */
            //sprintf(buffer, "Waiting for RX\r\n");
            //HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
            /* DEBUG */
            bool bHasData = m_cTUARTController.Available();
            if(bHasData) {
               /* DEBUG */
               //sprintf(buffer, "Got RX: Reading\r\n");
               //HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
               /* DEBUG */
               do {
                  m_cTUARTController.Read();
               }
               while(bHasData = m_cTUARTController.Available());
               sprintf(buffer, "Got RX: Writing\r\n");
               HardwareSerial::instance().write((const uint8_t*)buffer,strlen(buffer));
               m_cTUARTController.WriteByte('B');
               m_cTUARTController.WriteByte('L');
               m_cTUARTController.WriteByte('O');
               m_cTUARTController.WriteByte('C');
               m_cTUARTController.WriteByte('K');
               m_cTUARTController.WriteByte(' ');
               m_cTUARTController.WriteByte('O');
               m_cTUARTController.WriteByte('S');
               m_cTUARTController.WriteByte('\r');
               m_cTUARTController.WriteByte('\n');
            }
#endif
            continue;
            break;
         }
         /*
         
         m_cTimer.Delay(2000);
         sprintf(bufa, "RxState = %u, RxByte = %X, RxBuffer.Head = %u, RxBuffer.Tail = %u, DetectedEdges = %u\r\n", 
                 m_cTUARTController.m_unRxState, 
                 m_cTUARTController.m_unRxByte, 
                 m_cTUARTController.m_sRxBuffer.Head,
                 m_cTUARTController.m_sRxBuffer.Tail,
                 m_cTUARTController.DetectedEdges);
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
