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
                        //Firmware::GetInstance().GetTUARTController().Write(c_to_write);
                        return 1;
                     },
                     [](FILE* pf_stream) {
                        // return int(Firmware::GetInstance().GetTUARTController().Read());
                        return 1;
                     },
                     _FDEV_SETUP_RW);
 
   fdev_setup_stream(&huart, 
                     [](char c_to_write, FILE* pf_stream) {
                        Firmware::GetInstance().GetHUARTController().Write(c_to_write);
                        return 1;
                     },
                     [](FILE* pf_stream) {
                        return int(Firmware::GetInstance().GetHUARTController().Read());
                     },
                     _FDEV_SETUP_RW);

   Firmware::GetInstance().SetFilePointers(&huart, &tuart);

   /* Execute the firmware */
   return Firmware::GetInstance().Exec();
}


/***********************************************************/
/***********************************************************/

bool Firmware::InitXbee() {

   const uint8_t *ppunXbeeConfig[] = {
      (const uint8_t*)"+++",
      (const uint8_t*)"ATRE\r\n",
      (const uint8_t*)"ATID 2001\r\n",
      (const uint8_t*)"ATDH 0013A200\r\n",
      (const uint8_t*)"ATDL 40AA1A2C\r\n",
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
   
   struct SRxBuffer {
      uint8_t Buffer[8];
      uint8_t Index;
   } sRxBuffer = {{}, 0};
   
   while(bInitInProgress) {
      switch(eInitXbeeState) {
      case EInitXbeeState::RESET:
#ifdef DEBUG
         fprintf(m_psHUART, "Resetting Xbee\r\n");
#endif
         PORTD &= ~XBEE_RST_PIN; // drive xbee reset pin low (enable)
         DDRD |= XBEE_RST_PIN;   // set xbee reset pin as output
         m_cTimer.Delay(500);
         PORTD |= XBEE_RST_PIN;  // drive xbee reset pin high (disable)
         m_cTimer.Delay(1000);   // allow time for Xbee to boot
         eInitXbeeState = EInitXbeeState::CONFIG_TX;
         continue;
         break;
      case EInitXbeeState::CONFIG_TX: 
#ifdef DEBUG
         fprintf(m_psHUART, "Sending command: ");
#endif
         for(uint8_t unCmdCharIdx = 0;
             ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\0';
             unCmdCharIdx++) {
            m_cTUARTController.Write(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx]);
#ifdef DEBUG
            if(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\n' and
               ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\r') {
               fprintf(m_psHUART, "%c", ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx]);
            }
#endif
         }
#ifdef DEBUG
         fprintf(m_psHUART, "\r\n");
#endif
         eInitXbeeState = EInitXbeeState::CONFIG_WAIT_FOR_RX;
         sRxBuffer.Index = 0;
         unTimeout = 0;
         continue;
         break;
      case EInitXbeeState::CONFIG_WAIT_FOR_RX:
         m_cTimer.Delay(250);
         while(m_cTUARTController.Available() && sRxBuffer.Index < 8) {
            sRxBuffer.Buffer[sRxBuffer.Index++] = m_cTUARTController.Read();
         }
         
         if(sRxBuffer.Index > 2 &&
            sRxBuffer.Buffer[0] == 'O' &&
            sRxBuffer.Buffer[1] == 'K' &&
            sRxBuffer.Buffer[2] == '\r') {
#ifdef DEBUG
            fprintf(m_psHUART, "Got OK\r\n");
#endif
            if(unXbeeCmdIdx < 5) {
               /* send next AT command */
               unXbeeCmdIdx++;
               eInitXbeeState = EInitXbeeState::CONFIG_TX;
            }
            else {
               /* XBEE configured successfully */
               bInitInProgress = false;           
#ifdef DEBUG
               fprintf(m_psHUART, "Xbee Configuration: SUCCESS\r\n");
#endif
            }
         }
         else {
            unTimeout++;
            if(unTimeout > 15)
               eInitXbeeState = EInitXbeeState::CONFIG_TX;
         }
         continue;
         break;
      }
   }
   return bInitSuccess;
}

/***********************************************************/
/***********************************************************/

bool Firmware::InitMPU6050() {

   enum class EInitMPU6050State {
      PROBE,
      CONFIG_PWR_MAN,
   } eInitMPU6050State = EInitMPU6050State::PROBE;

   bool bInitInProgress = true;
   bool bInitSuccess = true;

   while(bInitInProgress) {
      switch(eInitMPU6050State) {
      case EInitMPU6050State::PROBE:
#ifdef DEBUG
         fprintf(m_psHUART, "Checking MPU6050 Address (WA)\r\n");
#endif
         Firmware::GetInstance().GetTWController().BeginTransmission(MPU6050_ADDR);
         Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EMPU6050Register::WHOAMI));
         Firmware::GetInstance().GetTWController().EndTransmission(false);
#ifdef DEBUG
         fprintf(m_psHUART, "Checking MPU6050 Address (RD)\r\n");
#endif
         Firmware::GetInstance().GetTWController().Read(MPU6050_ADDR, 1, true);
         
         /* Check if the WHOAMI register contains the MPU6050 address value  */
         if(Firmware::GetInstance().GetTWController().Read() == MPU6050_ADDR) {
            eInitMPU6050State = EInitMPU6050State::CONFIG_PWR_MAN;
#ifdef DEBUG
            fprintf(m_psHUART, "Found MPU6050 Sensor at 0x68!\r\n");
#endif      
         } 
         else {
            bInitInProgress = false;
            bInitSuccess = false;
#ifdef DEBUG
            fprintf(m_psHUART, "Unexpected response from device at 0x68!\r\n");
            fprintf(m_psHUART, "MPU6050 Configuration: FAILED \r\n");
#endif      
         }
         continue;
         break;
      case EInitMPU6050State::CONFIG_PWR_MAN:
         Firmware::GetInstance().GetTWController().BeginTransmission(MPU6050_ADDR);
#ifdef DEBUG
         fprintf(m_psHUART, "Updating Power Management (WA)\r\n");
#endif
         Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EMPU6050Register::PWR_MGMT_1));
#ifdef DEBUG
         fprintf(m_psHUART, "Updating Power Management (WD)\r\n");
#endif
         /* select internal clock, disable sleep/cycle mode, enable temperature sensor*/
         Firmware::GetInstance().GetTWController().Write(0x00);
         Firmware::GetInstance().GetTWController().EndTransmission(true);
#ifdef DEBUG
         fprintf(m_psHUART, "MPU6050 Configuration: SUCCESS\r\n");
#endif
         bInitInProgress = false;
         continue;
         break;
      }
   }
   return bInitSuccess;
}

/***********************************************************/
/***********************************************************/

bool Firmware::InitLTC2990() {
   /* configure the LTC2990 - battery current and voltage monitoring */
   Firmware::GetInstance().GetTWController().BeginTransmission(LTC2990_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(ELTC2990Register::CONTROL));
   /* configure LTC2990, perform all measurements when triggered,
      in differential voltage mode, temperature in celsius */
   Firmware::GetInstance().GetTWController().Write(0x5E);
   Firmware::GetInstance().GetTWController().EndTransmission(true);

   return true;
}

/***********************************************************/
/***********************************************************/
 
bool Firmware::InitPCA9554_RST() {
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::CONFIG));
   /* Configure the reset lines to the faces as outputs (driven high by default) */
   Firmware::GetInstance().GetTWController().Write(0xC0);
   Firmware::GetInstance().GetTWController().EndTransmission(true);   
   return true;
}

/***********************************************************/
/***********************************************************/

bool Firmware::InitPCA9554_IRQ() {
   /* no configuration required - input with pull up and IRQ enabled by default */
   return true;
}


bool Firmware::InitPCA9635() {
   /* Reset the PCA9635 using the special reset address and byte pattern  */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9635_RST);
   Firmware::GetInstance().GetTWController().Write(0xA5);
   Firmware::GetInstance().GetTWController().Write(0x5A);
   Firmware::GetInstance().GetTWController().EndTransmission(true);   
   m_cTimer.Delay(1);

   /* Wake up the internal oscillator */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9635_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9635Register::MODE1));
   Firmware::GetInstance().GetTWController().Write(0x00);
   Firmware::GetInstance().GetTWController().EndTransmission(true);
   m_cTimer.Delay(1);

   return true;
}

void Firmware::PCA9635_SetLEDMode(uint8_t un_led, EPCA9635LEDMode e_mode) {
   /* get the register responsible for LED un_led */
   uint8_t unRegisterAddr = static_cast<uint8_t>(EPCA9635Register::LEDOUT0) + (un_led / 4u);

   /* read current register value */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9635_ADDR);
   Firmware::GetInstance().GetTWController().Write(unRegisterAddr);
   Firmware::GetInstance().GetTWController().EndTransmission(false);
   Firmware::GetInstance().GetTWController().Read(PCA9635_ADDR, 1, true);
   uint8_t unRegisterVal = Firmware::GetInstance().GetTWController().Read();

   /* clear and set target bits in unRegisterVal */
   unRegisterVal &= (LEDOUTX_MASK << ((un_led % 4) * 2));
   unRegisterVal |= (static_cast<uint8_t>(e_mode) << ((un_led % 4) * 2));

   /* write back */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9635_ADDR);
   Firmware::GetInstance().GetTWController().Write(unRegisterAddr);
   Firmware::GetInstance().GetTWController().Write(unRegisterVal);
   Firmware::GetInstance().GetTWController().EndTransmission(true);
}

void Firmware::PCA9635_SetLEDBrightness(uint8_t un_led, uint8_t un_val) {
   uint8_t unRegisterAddr = static_cast<uint8_t>(EPCA9635Register::PWM0) + un_led;

   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9635_ADDR);
   Firmware::GetInstance().GetTWController().Write(unRegisterAddr);
   Firmware::GetInstance().GetTWController().Write(un_val);
   Firmware::GetInstance().GetTWController().EndTransmission(true);

   /* Ensure that the LED is in PWM mode */
   PCA9635_SetLEDMode(un_led, EPCA9635LEDMode::PWM);
}

void Firmware::Reset(uint8_t un_reset_target) {
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::OUTPUT));
   Firmware::GetInstance().GetTWController().EndTransmission(false);
   Firmware::GetInstance().GetTWController().Read(PCA9554_RST_ADDR, 1, true);
   uint8_t unFaceReset = Firmware::GetInstance().GetTWController().Read();
   /* Assert reset on the selected faces */
   unFaceReset &= ~un_reset_target;
   /* Write configuration back */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::OUTPUT));
   Firmware::GetInstance().GetTWController().Write(unFaceReset);
   Firmware::GetInstance().GetTWController().EndTransmission(true);

   /* Allow 1ms reset time */
   m_cTimer.Delay(1); // os sleep
   /* Recheck register state, may have changed while we slept */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::OUTPUT));
   Firmware::GetInstance().GetTWController().EndTransmission(false);
   Firmware::GetInstance().GetTWController().Read(PCA9554_RST_ADDR, 1, true);
   unFaceReset = Firmware::GetInstance().GetTWController().Read();
   /* Deassert reset on the selected faces */
   unFaceReset |= un_reset_target;
   /* Write configuration back */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9554_RST_ADDR);
   Firmware::GetInstance().GetTWController().Write(static_cast<uint8_t>(EPCA9554Register::OUTPUT));
   /* Configure the reset lines to the faces as ouputs (driven high by default) */
   Firmware::GetInstance().GetTWController().Write(unFaceReset);
   Firmware::GetInstance().GetTWController().EndTransmission(true);
}

