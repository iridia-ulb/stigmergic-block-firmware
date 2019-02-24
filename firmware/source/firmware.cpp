#include "firmware.h"
#include "uart-ref.h"

/* initialisation of the static singleton */
Firmware Firmware::_firmware;

/* main function that runs the firmware */
int main(void)
{
   /* FILE structs for fprintf */
   FILE tuart, huart;

   uart_init();
   

   /* Set up FILE structs for fprintf */                           
   fdev_setup_stream(&tuart,
                     [](char c_to_write, FILE* pf_stream) {
                        Firmware::GetInstance().GetTUARTController().Write(c_to_write);
                        return 1;
                     },
                     [](FILE* pf_stream) {
                        return int(Firmware::GetInstance().GetTUARTController().Read());
                     },
                     _FDEV_SETUP_RW);
 
   fdev_setup_stream(&huart, 
                     [](char c_to_write, FILE* pf_stream) {
                        uart_putc(c_to_write);
                        //Firmware::GetInstance().GetHUARTController().Write(c_to_write);
                        return 1;
                     },
                     [](FILE* pf_stream) {
                        unsigned int res = uart_getc();                                              
                        return static_cast<int>(res & 0x00ff);
                     },
                     _FDEV_SETUP_RW);

   Firmware::GetInstance().SetFilePointers(&huart, &tuart);

   /* Execute the firmware */
   return Firmware::GetInstance().Exec();
}

/***********************************************************/
/***********************************************************/

const char* Firmware::GetPortString(CPortController::EPort ePort) {
   switch(ePort) {
   case CPortController::EPort::NORTH:
      return "NORTH";
      break;
   case CPortController::EPort::EAST:
      return "EAST";
      break;
   case CPortController::EPort::SOUTH:
      return "SOUTH";
      break;
   case CPortController::EPort::WEST:
      return "WEST";
      break;
   case CPortController::EPort::TOP:
      return "TOP";
      break;
   case CPortController::EPort::BOTTOM:
      return "BOTTOM";
      break;
   default:
      return "INVALID";
      break;
   }
}

/***********************************************************/
/***********************************************************/

void Firmware::DetectPorts() {
   for(CPortController::EPort& ePort : m_peAllPorts) {
      if(m_cPortController.IsPortConnected(ePort)) {
#ifdef DEBUG
         fprintf(m_psOutputUART, "%s connected\r\n", GetPortString(ePort));
#endif
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort == CPortController::EPort::NULLPORT) {
               eConnectedPort = ePort;
               break;
            }
         }         
      }
      else {
#ifdef DEBUG
         fprintf(m_psOutputUART, "%s disconnected\r\n", GetPortString(ePort));
#endif
      }
   }
   m_cPortController.SelectPort(CPortController::EPort::NULLPORT);
}

/***********************************************************/
/***********************************************************/

bool Firmware::InitXbee() {

   const uint8_t *ppunXbeeConfig[] = {
      (const uint8_t*)"+++",
      (const uint8_t*)"ATRE\r\n",
      (const uint8_t*)"ATID 2003\r\n",
      (const uint8_t*)"ATDH 0013A200\r\n",
      (const uint8_t*)"ATDL 40C262B9\r\n",
      (const uint8_t*)"ATCN\r\n"};

   enum class EInitXbeeState {
      RESET,
      CONFIG_TX,
      CONFIG_WAIT_FOR_RX,
   } eInitXbeeState = EInitXbeeState::RESET;

   bool bInitInProgress = true;
   bool bInitSuccess = true;

   uint8_t unXbeeCmdIdx = 0;
   uint8_t unTimeout = 0;
   uint8_t unAttempts = 0;
   
   struct SRxBuffer {
      uint8_t Buffer[8];
      uint8_t Index;
   } sRxBuffer = {{}, 0};
   
   while(bInitInProgress) {
      switch(eInitXbeeState) {
      case EInitXbeeState::RESET:
         fprintf(m_psOutputUART, "XBEE RESET\r\n");
         PORTD &= ~XBEE_RST_PIN; // drive xbee reset pin low (enable)
         DDRD |= XBEE_RST_PIN;   // set xbee reset pin as output
         m_cTimer.Delay(50);
         PORTD |= XBEE_RST_PIN;  // drive xbee reset pin high (disable)
         m_cTimer.Delay(1000);   // allow time for Xbee to boot
         eInitXbeeState = EInitXbeeState::CONFIG_TX;
         continue;
         break;
      case EInitXbeeState::CONFIG_TX:
         if(unAttempts > 3) {
            
         }
         fprintf(m_psOutputUART, "CONFIG_TX\r\n");
         for(uint8_t unCmdCharIdx = 0;
             ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\0';
             unCmdCharIdx++) {
            m_cTUARTController.Write(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx]);
         }
         eInitXbeeState = EInitXbeeState::CONFIG_WAIT_FOR_RX;
         unAttempts++;
         sRxBuffer.Index = 0;
         unTimeout = 0;
         continue;
         break;
      case EInitXbeeState::CONFIG_WAIT_FOR_RX: // RX_ACK
         fprintf(m_psOutputUART, "CONFIG_WAIT_FOR_RX\r\n");
         m_cTimer.Delay(250);
         while(m_cTUARTController.Available() && sRxBuffer.Index < 8) {
            sRxBuffer.Buffer[sRxBuffer.Index++] = m_cTUARTController.Read();
         }
         
         if(sRxBuffer.Index > 2 &&
            sRxBuffer.Buffer[0] == 'O' &&
            sRxBuffer.Buffer[1] == 'K' &&
            sRxBuffer.Buffer[2] == '\r') {
            if(unXbeeCmdIdx < 5) {
               /* send next AT command */
               unXbeeCmdIdx++;
               eInitXbeeState = EInitXbeeState::CONFIG_TX;
            }
            else {
               /* XBEE configured successfully */
               bInitInProgress = false;           
            }
         }
         else {
            unTimeout++;
            if(unTimeout > 10) {
               if(unAttempts > 3) {
                  bInitInProgress = false;
                  bInitSuccess = false;
               }
               else {
                  eInitXbeeState = EInitXbeeState::CONFIG_TX;
               }
            }
         }
         continue;
         break;
      }
   }
   /* Flush the input buffer */
   while(Firmware::GetInstance().GetTUARTController().Available()) {
      Firmware::GetInstance().GetTUARTController().Read();
   }
   return bInitSuccess;
}

/***********************************************************/
/***********************************************************/

bool Firmware::InitPN532() {
   bool bNFCInitSuccess = false;
   if(m_cNFCController.Probe() == true) {
      //fprintf(m_psOutputUART, "Probe: SUCCESS\r\n");
      if(m_cNFCController.ConfigureSAM() == true) {
         //fprintf(m_psOutputUART, "SAM: SUCCESS\r\n");
         if(m_cNFCController.PowerDown() == true) {
            //fprintf(m_psOutputUART, "PWDN: SUCCESS\r\n");
            bNFCInitSuccess = true;
         }
      }    
   }
   return bNFCInitSuccess;
}

/***********************************************************/
/***********************************************************/

bool Firmware::InitMPU6050() {
   /* probe */
   if(CTWController::GetInstance().Read(MPU6050_ADDR, EMPU6050Register::WHOAMI) == MPU6050_ADDR) {
      /* select internal clock, disable sleep/cycle mode, enable temperature sensor */
      CTWController::GetInstance().Write(MPU6050_ADDR, EMPU6050Register::PWR_MGMT_1, 0x00);
      return true;
   }
   else return false;
}

/***********************************************************/
/***********************************************************/

void Firmware::TestAccelerometer() {
   /* buffer for holding accelerometer result */
   uint8_t punBuffer[8];
   CTWController::GetInstance().Read(MPU6050_ADDR, EMPU6050Register::ACCEL_XOUT_H, 8, punBuffer);
   fprintf(m_psOutputUART, 
           "Acc[x] = %i\r\n"
           "Acc[y] = %i\r\n"
           "Acc[z] = %i\r\n"
           "Temp = %i\r\n",
           int16_t((punBuffer[0] << 8) | punBuffer[1]),
           int16_t((punBuffer[2] << 8) | punBuffer[3]),
           int16_t((punBuffer[4] << 8) | punBuffer[5]),
           (int16_t((punBuffer[6] << 8) | punBuffer[7]) + 12412) / 340);  
}

/***********************************************************/
/***********************************************************/

void Firmware::TestPMIC() {
   fprintf(m_psOutputUART,
           "Power Connected = %c\r\nCharging = %c\r\n",
           (PIND & PWR_MON_PGOOD)?'F':'T',
           (PIND & PWR_MON_CHG)?'F':'T');
}

/***********************************************************/
/***********************************************************/

void Firmware::TestLEDs() {
   for(uint8_t unColor = 0; unColor < 3; unColor++) {
      for(uint8_t unVal = 0x00; unVal < 0x40; unVal++) {
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort != CPortController::EPort::NULLPORT) {
               m_cPortController.SelectPort(eConnectedPort);
               CLEDController::SetBrightness(unColor + 0, unVal);
               CLEDController::SetBrightness(unColor + 4, unVal);
               CLEDController::SetBrightness(unColor + 8, unVal);
               CLEDController::SetBrightness(unColor + 12, unVal);
            }
         }
         m_cTimer.Delay(1);
      }
      for(uint8_t unVal = 0x40; unVal > 0x00; unVal--) {
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort != CPortController::EPort::NULLPORT) {
               m_cPortController.SelectPort(eConnectedPort);
               CLEDController::SetBrightness(unColor + 0, unVal);
               CLEDController::SetBrightness(unColor + 4, unVal);
               CLEDController::SetBrightness(unColor + 8, unVal);
               CLEDController::SetBrightness(unColor + 12, unVal);
            }  
         }
         m_cTimer.Delay(1);
      }
      for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
         if(eConnectedPort != CPortController::EPort::NULLPORT) {
            CLEDController::SetBrightness(unColor + 0, 0x00);
            CLEDController::SetBrightness(unColor + 4, 0x00);
            CLEDController::SetBrightness(unColor + 8, 0x00);
            CLEDController::SetBrightness(unColor + 12, 0x00);
         }
      }
   }
}

#define NFC_DEBUG

/***********************************************************/
/***********************************************************/

CNFCController::EStatus Firmware::TestNFCTx() {
   uint8_t punOutboundBuffer[] = {'2'};
   uint8_t punInboundBuffer[20];
   uint8_t unRxCount = 0;

   if(m_cNFCController.GetStatus() != CNFCController::EStatus::BUSY) {
      if(!m_cNFCController.P2PInitiatorInit()) {
         m_cNFCController.PowerDown();
         m_cTimer.Delay(100);
         m_cPortController.ClearInterrupts();
         return CNFCController::EStatus::FAILED;
      }
   }
   unRxCount =
      m_cNFCController.P2PInitiatorTxRx(punOutboundBuffer,
                                        1,
                                        punInboundBuffer,
                                        20);

   CNFCController::EStatus eStatus = m_cNFCController.GetStatus();

   if(eStatus != CNFCController::EStatus::BUSY) {
      m_cNFCController.PowerDown();
      m_cTimer.Delay(100);
      m_cPortController.ClearInterrupts();
   }
   return eStatus;
}

/***********************************************************/
/***********************************************************/

bool Firmware::TestNFCRx() {
   uint8_t punOutboundBuffer[] = {'S','M','A','R','T','B','L','K','0','2'};
   uint8_t punInboundBuffer[20];
   uint8_t unRxCount = 0;

#ifdef NFC_DEBUG
   fprintf(m_psOutputUART, "Testing NFC RX\r\n");
#endif
   //m_cPortController.SelectPort(CPortController::EPort::EAST);
   if(m_cNFCController.P2PTargetInit()) {
#ifdef NFC_DEBUG
      fprintf(m_psOutputUART, "Connected!\r\n");
#endif
      unRxCount = m_cNFCController.P2PTargetTxRx(punOutboundBuffer,
                                                 10,
                                                 punInboundBuffer,
                                                 20);
#ifdef NFC_DEBUG
      if(unRxCount > 0) {
         fprintf(m_psOutputUART, "Received %i bytes: ", unRxCount);
         for(uint8_t i = 0; i < unRxCount; i++) {
            fprintf(m_psOutputUART, "%c", punInboundBuffer[i]);
         }
         fprintf(m_psOutputUART, "\r\n");
      }
      else {
         fprintf(m_psOutputUART, "No data\r\n");
      }
#endif
   }
   // This delay is important - entering power down too soon causes issues with future communication
   m_cTimer.Delay(60);
   m_cNFCController.PowerDown();
   // Once an response for a command is ready, an interrupt is generated. The last interrupt for the power down reply is cleared here
   m_cTimer.Delay(100);
   m_cPortController.ClearInterrupts();
   // An unRxCount of greater than zero normally indicates success
   return (unRxCount > 0);
}

/***********************************************************/
/***********************************************************/

void Firmware::Reset() {
   PORTD |= 0x10;
   DDRD |= 0x10;
}

/***********************************************************/
/***********************************************************/

int Firmware::Exec() {
   /* Configure the BQ24075 monitoring pins */
   DDRD &= ~PWR_MON_MASK;  // set as input
   PORTD &= ~PWR_MON_MASK; // disable pull ups

   /* Enable interrupts */
   sei();
  
   /* Begin Init */
   fprintf(m_psOutputUART, "[%05lu] Stigmergic Block Initialization\r\n", m_cTimer.GetMilliseconds());

   /* Configure port controller, detect ports */
   fprintf(m_psOutputUART, "[%05lu] Detecting Ports\r\n", m_cTimer.GetMilliseconds());

   /* Debug */
   m_cPortController.SelectPort(CPortController::EPort::NULLPORT);
   m_cTimer.Delay(10);
   m_cPortController.Init();
   CTWController::GetInstance().Disable();
   DetectPorts();
   CTWController::GetInstance().Enable();

   fprintf(m_psOutputUART, "[%05lu] Connected Ports: ", m_cTimer.GetMilliseconds());
   for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         fprintf(m_psOutputUART, "%s ", GetPortString(eConnectedPort));
      }
   }
   fprintf(m_psOutputUART, "\r\n");

   /* Configure accelerometer */
   /* Disconnect ports before running accelerometer init routine */
   m_cPortController.SelectPort(CPortController::EPort::NULLPORT);
   m_cTimer.Delay(10);
   fprintf(m_psOutputUART, 
           "[%05lu] Configuring MPU6050: %s\r\n", 
           m_cTimer.GetMilliseconds(), 
           InitMPU6050()?"success":"failed");

   /* Init on face devices */
   for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         m_cPortController.SelectPort(eConnectedPort);
         fprintf(m_psOutputUART, "[%05lu] Init PCA9635 on %s\r\n",
                 m_cTimer.GetMilliseconds(),
                 GetPortString(eConnectedPort));
         CLEDController::Init();
         CBlockLEDRoutines::SetAllModesOnFace(CLEDController::EMode::BLINK);
         CBlockLEDRoutines::SetAllColorsOnFace(0x05,0x00,0x00);
         fprintf(m_psOutputUART, "[%05lu] Init PN532 on %s\r\n", 
                 m_cTimer.GetMilliseconds(), 
                 GetPortString(eConnectedPort));
         m_cPortController.EnablePort(eConnectedPort);
         for(uint8_t unAttempts = 3; unAttempts > 0; unAttempts--) {
            if(InitPN532() == true) {
               CBlockLEDRoutines::SetAllModesOnFace(CLEDController::EMode::PWM);
               CBlockLEDRoutines::SetAllColorsOnFace(0x00,0x03,0x03);
               break;
            }
            m_cTimer.Delay(100);
         }
      }
   }

   /* Do Xbee detection */
   bHasXbee = false;
   /*
   if((bHasXbee = InitXbee()) == true) {
      fprintf(m_psOutputUART, "[%05lu] Xbee IO Selected!\r\n", m_cTimer.GetMilliseconds());
      m_psOutputUART = m_psTUART;
      for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
         m_cPortController.SelectPort(eConnectedPort);
         CBlockLEDRoutines::SetAllColorsOnFace(0x00,0x00,0x05);
      }
   }
   */

   for(;;) {
      if(unsigned char c = uart_getc() & 0x00ff) {
         uart_putc(c);
         uart_putc(' ');
      }
   }

   return 0;
}

/***********************************************************/
/***********************************************************/

#define RGB_RED_OFFSET    0
#define RGB_GREEN_OFFSET  1
#define RGB_BLUE_OFFSET   2
#define RGB_UNUSED_OFFSET 3

#define RGB_LEDS_PER_FACE 4

/***********************************************************/
/***********************************************************/


void Firmware::CBlockLEDRoutines::SetAllColorsOnFace(uint8_t unRed, 
                                                     uint8_t unGreen, 
                                                     uint8_t unBlue) {
   for(uint8_t unLedIdx = 0; unLedIdx < RGB_LEDS_PER_FACE; unLedIdx++) {
      CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                    RGB_RED_OFFSET, unRed);
      CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                    RGB_GREEN_OFFSET, unGreen);
      CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                    RGB_BLUE_OFFSET, unBlue);
   }
}


/***********************************************************/
/***********************************************************/

void Firmware::CBlockLEDRoutines::SetAllModesOnFace(CLEDController::EMode e_mode) {
   for(uint8_t unLedIdx = 0; unLedIdx < RGB_LEDS_PER_FACE; unLedIdx++) {
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE 
                              + RGB_RED_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE +
                              RGB_GREEN_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE +
                              RGB_BLUE_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE +
                              RGB_UNUSED_OFFSET, CLEDController::EMode::OFF);
   }
}



