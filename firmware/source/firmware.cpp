#include "firmware.h"

/* main function that runs the firmware */
int main(void)
{
   /* Configure the BQ24075 monitoring pins */
   DDRD &= ~PWR_MON_MASK;  // set as input
   PORTD &= ~PWR_MON_MASK; // disable pull ups

   /* FILE structs for fprintf */
   FILE tuart, huart;

   /* Set up FILE structs for fprintf */                           
   fdev_setup_stream(&huart, 
                     [](char c_to_write, FILE* pf_stream) {
                        CHUARTController::GetInstance().Write(c_to_write);
                        return 1;
                     },
                     [](FILE* pf_stream) {
                        return static_cast<int>(CHUARTController::GetInstance().Read());
                     },
                     _FDEV_SETUP_RW);

   CFirmware::GetInstance().SetFilePointers(&huart, &tuart);

   /* Execute the firmware */
   return CFirmware::GetInstance().Exec();
}

/***********************************************************/
/***********************************************************/

const char* CFirmware::GetPortString(CPortController::EPort ePort) {
   switch(ePort) {
   case CPortController::EPort::NORTH:
      return "N";
      break;
   case CPortController::EPort::EAST:
      return "E";
      break;
   case CPortController::EPort::SOUTH:
      return "S";
      break;
   case CPortController::EPort::WEST:
      return "W";
      break;
   case CPortController::EPort::TOP:
      return "T";
      break;
   case CPortController::EPort::BOTTOM:
      return "B";
      break;
   default:
      return "X";
      break;
   }
}

/***********************************************************/
/***********************************************************/

namespace Task {

#define RGB_RED_OFFSET    0
#define RGB_GREEN_OFFSET  1
#define RGB_BLUE_OFFSET   2
#define RGB_UNUSED_OFFSET 3

#define RGB_LEDS_PER_FACE 4

   void SetLEDColors(uint8_t unRed, uint8_t unGreen, uint8_t unBlue) {
      for(uint8_t unLedIdx = 0; unLedIdx < RGB_LEDS_PER_FACE; unLedIdx++) {
         CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                       RGB_RED_OFFSET, unRed);
         CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                       RGB_GREEN_OFFSET, unGreen);
         CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                       RGB_BLUE_OFFSET, unBlue);
      }
   }


   void SetLEDModes(CLEDController::EMode e_mode) {
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
}

/***********************************************************/
/***********************************************************/

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

bool CFirmware::InitMPU6050() {
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

void CFirmware::TestAccelerometer() {
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

void CFirmware::TestPMIC() {
   fprintf(m_psOutputUART,
           "Powered = %c\r\nCharging = %c\r\n",
           (PIND & PWR_MON_PGOOD)?'F':'T',
           (PIND & PWR_MON_CHG)?'F':'T');
}

/***********************************************************/
/***********************************************************/

void CFirmware::Reset() {
   PORTD |= 0x10;
   DDRD |= 0x10;
}

/***********************************************************/
/***********************************************************/

#define TEXT_BOLD "\e[1m"
#define TEXT_NORMAL "\e[0m"

void Log(const char* pch_message, bool b_bold = false) {
   uint32_t un_time = CClock::GetInstance().GetMilliseconds();
   fprintf(CFirmware::GetInstance().m_psOutputUART, "[%05lu] %s%s\e[0m\r\n", un_time, (b_bold ? "\e[1m" : "") ,pch_message);
}


/***********************************************************/
/***********************************************************/

int CFirmware::Exec() {
   /* Enable interrupts */
   CInterruptController::GetInstance().Enable();
   /* Begin Init */
   Log("Stigmergic Block Initialization", true);

   /* Configure port controller, detect ports */
   fprintf(m_psOutputUART, "[%05lu] Detecting ports\r\n", CClock::GetInstance().GetMilliseconds());

   /* Debug */
   CPortController::GetInstance().SelectPort(CPortController::EPort::NULLPORT);
   CClock::GetInstance().Delay(10);
   /* Power cycle the faces */
   CPortController::GetInstance().Init();
   for(SFace& sFace : m_psFaces) {
      CPortController::GetInstance().DisablePort(sFace.Port);
   }
   for(SFace& sFace : m_psFaces) {
      CPortController::GetInstance().EnablePort(sFace.Port);
   }
   /* Disable the TW controller during face detection */
   CTWController::GetInstance().Disable();
   /* detect faces */
   for(SFace& sFace : m_psFaces) {
      sFace.Connected = CPortController::GetInstance().IsPortConnected(sFace.Port);
   }
   CPortController::GetInstance().SelectPort(CPortController::EPort::NULLPORT);
   CTWController::GetInstance().Enable();

   fprintf(m_psOutputUART, "[%05lu] Connected faces: ", CClock::GetInstance().GetMilliseconds());
   for(SFace& sFace : m_psFaces) {
      if(sFace.Connected) {
         fprintf(m_psOutputUART, "%s ", GetPortString(sFace.Port));
      }
   }
   fprintf(m_psOutputUART, "\r\n");

   /* Configure accelerometer */
   /* Disconnect ports before running accelerometer init routine */
   CPortController::GetInstance().SelectPort(CPortController::EPort::NULLPORT);
   CClock::GetInstance().Delay(10);
   Log("Initialize MPU6050");
   InitMPU6050();

   /* Initialize faces */
   for(SFace& sFace : m_psFaces) {
      if(sFace.Connected) {
         CPortController::GetInstance().SelectPort(sFace.Port);
         CClock::GetInstance().Delay(10);
         fprintf(m_psOutputUART, "[%05lu] Initialize %s face\r\n",
                 CClock::GetInstance().GetMilliseconds(),
                 GetPortString(sFace.Port));
         /* Initialize LEDs */
         CLEDController::Init();
         Task::SetLEDModes(CLEDController::EMode::PWM);
         Task::SetLEDColors(0x00,0x00,0x00);
         /* Start initialization of NFC controllers */
         sFace.NFC.SetTargetRxFunctor(&sFace.RxDetector);
         sFace.NFC.SetInitiatorRxFunctor(&sFace.RxDetector);
         sFace.NFC.Step(CNFCController::EEvent::Init);
      }
   }

   uint32_t unLastDiag = CClock::GetInstance().GetMilliseconds();

   /* begin infinite loop */
   for(;;) {
      /* forward interrupts to faces */
      for(SFace& sFace : m_psFaces) {
         if(sFace.Connected) {
            CPortController::GetInstance().SelectPort(sFace.Port);
            CClock::GetInstance().Delay(10);
            /* check for interrupts */
            uint8_t unIRQs = CPortController::GetInstance().GetInterrupts();
            if((unIRQs >> static_cast<uint8_t>(sFace.Port)) & 0x01) {
               sFace.NFC.Step(CNFCController::EEvent::Interrupt);
            }
            else {
               sFace.NFC.Step();
            }
         }
      }

      uint8_t unActivatedFaceCount = 0;
      uint32_t unTime = CClock::GetInstance().GetMilliseconds();
      for(SFace& sFace : m_psFaces) {
         if(sFace.Connected && (unTime - sFace.RxDetector.LastRxTime < 750)) {
            unActivatedFaceCount++;
         }
      }
      for(SFace& sFace : m_psFaces) {
         if(sFace.Connected) {
            CPortController::GetInstance().SelectPort(sFace.Port);
            Task::SetLEDColors(unActivatedFaceCount == 1 ? 0x05 : 0x00,
                               unActivatedFaceCount == 2 ? 0x05 : 0x00,
                               unActivatedFaceCount == 3 ? 0x05 : 0x00);
         }
      }

      if(unTime - unLastDiag > 5000) {
         unLastDiag = unTime;
         for(SFace& sFace : m_psFaces) {
            if(sFace.Connected) {
               fprintf(m_psOutputUART, "%s: %u\r\n", GetPortString(sFace.Port), sFace.RxDetector.Messages);
               sFace.RxDetector.Messages = 0;
            }
         }
      }
   }
   return 0;
}

/***********************************************************/
/***********************************************************/

void CFirmware::Debug(const SFace& s_face) {
   fprintf(m_psOutputUART, "%s:", GetPortString(s_face.Port));
   switch(s_face.NFC.m_eState) {
   case CNFCController::EState::Standby:
      fprintf(m_psOutputUART, "S:");
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
   switch(s_face.NFC.m_eSelectedCommand) {
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
      fprintf(m_psOutputUART, "0x%02x", static_cast<uint8_t>(s_face.NFC.m_eSelectedCommand));
      break;
   }
   fprintf(m_psOutputUART, "\r\n");
}



