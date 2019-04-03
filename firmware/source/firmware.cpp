#include "firmware.h"

/* initialisation of the static singleton */
Firmware Firmware::_firmware;

/* main function that runs the firmware */
int main(void)
{
   /* FILE structs for fprintf */
   FILE tuart, huart;

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
                        CHUARTController::GetInstance().Write(c_to_write);
                        return 1;
                     },
                     [](FILE* pf_stream) {
                        return static_cast<int>(CHUARTController::GetInstance().Read());
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
         fprintf(m_psOutputUART, "XB_RST\r\n");
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
         fprintf(m_psOutputUART, "XB_CFG_TX\r\n");
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
         fprintf(m_psOutputUART, "XB_CFG_RX\r\n");
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
           "X = %i\r\n"
           "Y = %i\r\n"
           "Z = %i\r\n"
           "T = %i\r\n",
           int16_t((punBuffer[0] << 8) | punBuffer[1]),
           int16_t((punBuffer[2] << 8) | punBuffer[3]),
           int16_t((punBuffer[4] << 8) | punBuffer[5]),
           (int16_t((punBuffer[6] << 8) | punBuffer[7]) + 12412) / 340);  
}

/***********************************************************/
/***********************************************************/

void Firmware::TestPMIC() {
   fprintf(m_psOutputUART,
           "Powered = %c\r\nCharging = %c\r\n",
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
   CInterruptController::GetInstance().Enable();
  
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
         CBlockLEDRoutines::SetAllModesOnFace(CLEDController::EMode::PWM);
         CBlockLEDRoutines::SetAllColorsOnFace(0x00,0x00,0x00);
      }
   }

   InteractiveMode();

   return 0;
}

/***********************************************************/
/***********************************************************/

void Firmware::InteractiveMode() {
   CNFCController cNFCController;

   uint8_t unInput = 0;

   for(;;) {
      if(bHasXbee) {
         if(Firmware::GetInstance().GetTUARTController().Available()) {
            unInput = Firmware::GetInstance().GetTUARTController().Read();
            /* flush */
            while(Firmware::GetInstance().GetTUARTController().Available()) {
               Firmware::GetInstance().GetTUARTController().Read();
            }
         }
         else {
            unInput = 0;
         }
      }
      else {
         if(CHUARTController::GetInstance().HasData()) {
            unInput = CHUARTController::GetInstance().Read();
            /* flush */
            while(CHUARTController::GetInstance().HasData()) {
               CHUARTController::GetInstance().Read();
            }
         }
         else {
            unInput = 0;
         }
      }
      switch(unInput) {
      case 'a':
         TestAccelerometer();
         break;
      case 'b':
         // assumes divider with 330k and 1M
         fprintf(m_psOutputUART,
                 "Battery = %umV\r\n", 
                 m_cADCController.GetValue(CADCController::EChannel::ADC7) * 17);
         break;
      case '1':
         /* Q1 (U+, V+) */
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort != CPortController::EPort::NULLPORT) {
               m_cPortController.SelectPort(eConnectedPort);
               CBlockLEDRoutines::SetAllColorsOnFace(0x03,0x00,0x03);
            }
         }
         break;
      case '2':
         /* Q2 (U-, V+) */
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort != CPortController::EPort::NULLPORT) {
               m_cPortController.SelectPort(eConnectedPort);
               CBlockLEDRoutines::SetAllColorsOnFace(0x05,0x01,0x00);
            }
         }
         break;
      case '3':
         /* Q3 (U-, V-) */
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort != CPortController::EPort::NULLPORT) {
               m_cPortController.SelectPort(eConnectedPort);
               CBlockLEDRoutines::SetAllColorsOnFace(0x01,0x05,0x00);
            }
         }
         break;
      case '4':
         /* Q4 (U+, V-) */        
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort != CPortController::EPort::NULLPORT) {
               m_cPortController.SelectPort(eConnectedPort);
               CBlockLEDRoutines::SetAllColorsOnFace(0x00,0x03,0x03);
            }
         }
         break;
      case 'i':
         fprintf(m_psOutputUART, "IRQ: 0x%02X\r\n", m_cPortController.GetInterrupts());
         break;
      case 'r':
         Reset();
         break;
      case 's':
         fprintf(m_psOutputUART, "STT: 0x%02X\r\n", static_cast<uint8_t>(cNFCController.m_eState));
         fprintf(m_psOutputUART, "CMD: 0x%02X\r\n", static_cast<uint8_t>(cNFCController.m_eSelectedCommand));
         break;
      case 't':
         cNFCController.AppendEvent(CNFCController::EEvent::Transceive);
         break;
      case 'u':
         fprintf(m_psOutputUART, "Uptime = %lums\r\n", m_cTimer.GetMilliseconds());
         break;
      default:
         uint8_t unIRQs = m_cPortController.GetInterrupts();
         if(unIRQs != 0x00) {
            switch(cNFCController.m_eState) {
            case CNFCController::EState::Standby:
               fprintf(m_psOutputUART, "S:");
               break;
            case CNFCController::EState::Ready:
               fprintf(m_psOutputUART, "R:");
               break;
            case CNFCController::EState::WaitingForAck:
               fprintf(m_psOutputUART, "WA:");
               break;
            case CNFCController::EState::WaitingForResp:
               fprintf(m_psOutputUART, "WR:");
               break;
            case CNFCController::EState::Failed:
               fprintf(m_psOutputUART, "F:");
               break;
            }
            switch(cNFCController.m_eSelectedCommand) {
            case CNFCController::ECommand::GetFirmwareVersion:
               fprintf(m_psOutputUART, "GFV:");
               break;
            case CNFCController::ECommand::ConfigureSAM:
               fprintf(m_psOutputUART, "CSAM:");
               break;
            case CNFCController::ECommand::InJumpForDEP:
               fprintf(m_psOutputUART, "IJFD:");
               break;
            case CNFCController::ECommand::InDataExchange:
               fprintf(m_psOutputUART, "IDE:");
               break;
            case CNFCController::ECommand::TgInitAsTarget:
               fprintf(m_psOutputUART, "IAT:");
               break;
            case CNFCController::ECommand::TgGetData:
               fprintf(m_psOutputUART, "TGD:");
               break;
            case CNFCController::ECommand::TgSetData:
               fprintf(m_psOutputUART, "TSD:");
               break;
            default:
               fprintf(m_psOutputUART, "0x%02x", static_cast<uint8_t>(cNFCController.m_eSelectedCommand));
               break;
            }
            fprintf(m_psOutputUART, "\r\n");
            for(CPortController::EPort eRxPort : m_peConnectedPorts) {
               if(eRxPort != CPortController::EPort::NULLPORT) {
                  if((unIRQs >> static_cast<uint8_t>(eRxPort)) & 0x01) {
                     m_cPortController.SelectPort(eRxPort);
                     cNFCController.AppendEvent(CNFCController::EEvent::Interrupt);
                  }
               }
            }
         }
         break;
      }
   }
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



