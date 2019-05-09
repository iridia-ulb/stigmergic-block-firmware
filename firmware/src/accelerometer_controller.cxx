
#include "accelerometer_controller.h"

/***********************************************************/
/***********************************************************/

CAccelerometerController::CAccelerometerController() :
   m_bStatus(true) {
   uint8_t unReadByte;
   m_bStatus = m_bStatus && 
      CTWController::GetInstance().ReadRegister(m_unDeviceAddress, ERegister::WhoAmI, 1, &unReadByte);
   m_bStatus = m_bStatus && 
      (unReadByte == m_unDeviceAddress);
   /* select internal clock, disable sleep/cycle mode, enable temperature sensor */
   m_bStatus = m_bStatus && 
      CTWController::GetInstance().WriteRegister(m_unDeviceAddress, ERegister::PowerManagement1, 0x00);
}

/***********************************************************/
/***********************************************************/

CAccelerometerController::SSample CAccelerometerController::GetSample() {
   /* buffer for holding accelerometer result */
   uint8_t punBuffer[6];
   CTWController::GetInstance().ReadRegister(m_unDeviceAddress, ERegister::AccelerometerXHighByte, 6, punBuffer);
   return SSample(reinterpret_cast<int16_t>((punBuffer[0] << 8) | punBuffer[1]),
                  reinterpret_cast<int16_t>((punBuffer[2] << 8) | punBuffer[3]),
                  reinterpret_cast<int16_t>((punBuffer[4] << 8) | punBuffer[5]));
}

/***********************************************************/
/***********************************************************/

