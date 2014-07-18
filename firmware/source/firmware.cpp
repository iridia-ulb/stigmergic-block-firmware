#include "firmware.h"

/* initialisation of the static singleton */
Firmware Firmware::_firmware;


/* for IO via fprintf */
//void huart_putbyte(uint8_t unByte, FILE *stream) {
int huart_putbyte(char unByte, FILE *stream) {
   HardwareSerial::instance().write(unByte);
   return 1;
}

//uint8_t huart_getbyte(FILE *stream) {
int huart_getbyte(FILE *stream) {
   return HardwareSerial::instance().read();
}

int tuart_putbyte(char unByte, FILE *stream) {
   Firmware::instance().GetTUARTController().WriteByte(unByte);
   return 1;
}


int tuart_getbyte(FILE *stream) {
   return Firmware::instance().GetTUARTController().Read();
}



/* main function that runs the firmware */
int main(void)
{
   FILE tuart, huart;

   fdev_setup_stream(&tuart,
                     tuart_putbyte,
                     tuart_getbyte,
                     _FDEV_SETUP_RW);
 
   fdev_setup_stream(&huart, 
                     huart_putbyte,
                     huart_getbyte,
                     _FDEV_SETUP_RW);

   Firmware::instance().SetFilePointers(&huart, &tuart);

   return Firmware::instance().exec();
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
      CONFIG_TX,
      CONFIG_WAIT_FOR_RX,
   } eInitXbeeState = EInitXbeeState::CONFIG_TX;

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
      case EInitXbeeState::CONFIG_TX: 
#ifdef DEBUG
         fprintf(huart, "Sending command: ");
#endif
         for(uint8_t unCmdCharIdx = 0;
             ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\0';
             unCmdCharIdx++) {
            m_cTUARTController.WriteByte(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx]);
#ifdef DEBUG
            if(ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\n' and
               ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx] != '\r') {
               fprintf(huart, "%c", ppunXbeeConfig[unXbeeCmdIdx][unCmdCharIdx]);
            }
#endif
         }
#ifdef DEBUG
         fprintf(huart, "\r\n");
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
            fprintf(huart, "Got OK\r\n");
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
               fprintf(huart, "Xbee Configuration: SUCCESS\r\n");
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
         fprintf(huart, "Checking MPU6050 Address (WA)\r\n");
#endif
         CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
         CTWController::GetInstance().Write(MPU6050_WHOAMI);
         CTWController::GetInstance().EndTransmission(false);
#ifdef DEBUG
         fprintf(huart, "Checking MPU6050 Address (RD)\r\n");
#endif
         CTWController::GetInstance().Read(MPU6050_ADDR, 1, true);
         
         /* Check if the WHOAMI register contains the MPU6050 address value  */
         if(CTWController::GetInstance().Read() == MPU6050_ADDR) {
            eInitMPU6050State = EInitMPU6050State::CONFIG_PWR_MAN;
#ifdef DEBUG
            fprintf(huart, "Found MPU6050 Sensor at 0x68!\r\n");
#endif      
         } 
         else {
            bInitInProgress = false;
            bInitSuccess = false;
#ifdef DEBUG
            fprintf(huart, "Unexpected response from device at 0x68!\r\n");
            fprintf(huart, "MPU6050 Configuration: FAILED \r\n");
#endif      
         }
         continue;
         break;
      case EInitMPU6050State::CONFIG_PWR_MAN:
         CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
#ifdef DEBUG
         fprintf(huart, "Updating Power Management (WA)\r\n");
#endif
         CTWController::GetInstance().Write(MPU6050_PWR_MGMT_1);
#ifdef DEBUG
         fprintf(huart, "Updating Power Management (WD)\r\n");
#endif
         /* select internal clock, disable sleep/cycle mode, enable temperature sensor*/
         CTWController::GetInstance().Write(0x00);
         CTWController::GetInstance().EndTransmission(true);
#ifdef DEBUG
         fprintf(huart, "MPU6050 Configuration: SUCCESS\r\n");
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
   CTWController::GetInstance().BeginTransmission(LTC2990_ADDR);
   CTWController::GetInstance().Write(LTC2990_CONTROL);
   /* configure LTC2990, perform all measurements when triggered,
      in differential voltage mode, temperature in celsius */
   CTWController::GetInstance().Write(0x5E);
   CTWController::GetInstance().EndTransmission(true);

   return true;
}
