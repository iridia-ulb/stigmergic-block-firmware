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
         PORTD &= ~XBEE_RST_PIN; // drive xbee reset pin low (enable)
         DDRD |= XBEE_RST_PIN;   // set xbee reset pin as output
         m_cTimer.Delay(50);
         PORTD |= XBEE_RST_PIN;  // drive xbee reset pin high (disable)
         m_cTimer.Delay(1000);   // allow time for Xbee to boot
         eInitXbeeState = EInitXbeeState::CONFIG_TX;
         continue;
         break;
      case EInitXbeeState::CONFIG_TX: 
         for(uint8_t unCmdCharIdx = 0;
             ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\0';
             unCmdCharIdx++) {
            m_cTUARTController.Write(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx]);
         }
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

bool Firmware::InitPN532() {
   bool bNFCInitSuccess = false;
   if(m_cNFCController.Probe() == true) {
      if(m_cNFCController.ConfigureSAM() == true) {
         if(m_cNFCController.PowerDown() == true) {
            bNFCInitSuccess = true;
         }
      }    
   }
   fprintf(m_psHUART, "NFC Init %s\r\n",bNFCInitSuccess?"passed":"failed");

   return bNFCInitSuccess;
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

/***********************************************************/
/***********************************************************/

void Firmware::TestAccelerometer() {
   /* Array for holding accelerometer result */
   uint8_t punMPU6050Res[8];

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
}

/***********************************************************/
/***********************************************************/

void Firmware::TestPMIC() {
   fprintf(m_psTUART,
           "Power Connected = %c\r\nCharging = %c\r\n",
           (PIND & PWR_MON_PGOOD)?'F':'T',
           (PIND & PWR_MON_CHG)?'F':'T');
}

/***********************************************************/
/***********************************************************/

void Firmware::TestLEDs() {
   /* correctly connected LEDs for testing */
   uint8_t punLEDIdx[6] = {0,1,2, 12, 11, 10};

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
}

/***********************************************************/
/***********************************************************/

void Firmware::TestNFCTx() {
   uint8_t punOutboundBuffer[] = {'S','M','A','R','T','B','L','K','0','2'};
   uint8_t punInboundBuffer[20];
   uint8_t unRxCount = 0;

   fprintf(m_psTUART, "Testing NFC TX\r\n");           
   m_cPortController.SelectPort(CPortController::EPort::SOUTH);
   unRxCount = 0;
   if(m_cNFCController.P2PInitiatorInit()) {
      fprintf(m_psTUART, "Connected!\r\n");
      unRxCount = m_cNFCController.P2PInitiatorTxRx(punOutboundBuffer,
                                                    10,
                                                    punInboundBuffer,
                                                    20);
      if(unRxCount > 0) {
         fprintf(m_psTUART, "Received %i bytes: ", unRxCount);
         for(uint8_t i = 0; i < unRxCount; i++) {
            fprintf(m_psTUART, "%c", punInboundBuffer[i]);
         }
         fprintf(m_psTUART, "\r\n");
      }
      else {
         fprintf(m_psTUART, "No data\r\n");
      }
   }
   m_cNFCController.PowerDown();
   /* Once an response for a command is ready, an interrupt is generated
      The last interrupt for the power down reply is cleared here */
   m_cTimer.Delay(100);
   m_cPortController.ClearInterrupts();
}

/***********************************************************/
/***********************************************************/

void Firmware::TestNFCRx() {
   uint8_t punOutboundBuffer[] = {'S','M','A','R','T','B','L','K','0','2'};
   uint8_t punInboundBuffer[20];
   uint8_t unRxCount = 0;

   fprintf(m_psTUART, "Testing NFC RX\r\n");
   m_cPortController.SelectPort(CPortController::EPort::SOUTH);
   if(m_cNFCController.P2PTargetInit()) {
      fprintf(m_psTUART, "Connected!\r\n");
      unRxCount = m_cNFCController.P2PTargetTxRx(punOutboundBuffer,
                                                 10,
                                                 punInboundBuffer,
                                                 20);
      if(unRxCount > 0) {
         fprintf(m_psTUART, "Received %i bytes: ", unRxCount);
         for(uint8_t i = 0; i < unRxCount; i++) {
            fprintf(m_psTUART, "%c", punInboundBuffer[i]);
         }
         fprintf(m_psTUART, "\r\n");
      }
      else {
         fprintf(m_psTUART, "No data\r\n");
      }
   }
   /* This delay is important - entering power down too soon causes issues
      with future communication */
   m_cTimer.Delay(60);
   m_cNFCController.PowerDown();
   /* Once an response for a command is ready, an interrupt is generated
      The last interrupt for the power down reply is cleared here */
   m_cTimer.Delay(100);
   m_cPortController.ClearInterrupts();
}

/***********************************************************/
/***********************************************************/

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
   unRegisterVal &= ~(LEDOUTX_MASK << ((un_led % 4) * 2));
   unRegisterVal |= (static_cast<uint8_t>(e_mode) << ((un_led % 4) * 2));
   /* write back */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9635_ADDR);
   Firmware::GetInstance().GetTWController().Write(unRegisterAddr);
   Firmware::GetInstance().GetTWController().Write(unRegisterVal);
   Firmware::GetInstance().GetTWController().EndTransmission(true);
}

/***********************************************************/
/***********************************************************/

void Firmware::PCA9635_SetLEDBrightness(uint8_t un_led, uint8_t un_val) {
   /* get the register responsible for LED un_led */
   uint8_t unRegisterAddr = static_cast<uint8_t>(EPCA9635Register::PWM0) + un_led;
   /* write value */
   Firmware::GetInstance().GetTWController().BeginTransmission(PCA9635_ADDR);
   Firmware::GetInstance().GetTWController().Write(unRegisterAddr);
   Firmware::GetInstance().GetTWController().Write(un_val);
   Firmware::GetInstance().GetTWController().EndTransmission(true);
   /* Ensure that the LED is in PWM mode */
   PCA9635_SetLEDMode(un_led, EPCA9635LEDMode::PWM);
}


