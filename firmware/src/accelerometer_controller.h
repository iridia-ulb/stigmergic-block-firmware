
#ifndef ACCELEROMETER_CONTROLLER_H
#define ACCELEROMETER_CONTROLLER_H

#include "tw_controller.h"

class CAccelerometerController {
public:
   enum class ERegister : uint8_t {
      PowerManagement1        = 0x6B,
      PowerManagement2        = 0x6C,
      AccelerometerXHighByte  = 0x3B,
      AccelerometerXLowByte   = 0x3C,
      AccelerometerYHighByte  = 0x3D,
      AccelerometerYLowByte   = 0x3E,
      AccelerometerZHighByte  = 0x3F,
      AccelerometerZLowByte   = 0x40,
      TemperatureHighByte     = 0x41,
      TemperatureLowByte      = 0x42,
      WhoAmI                  = 0x75,
   };

   struct SSample {
      SSample(int16_t n_x,
              int16_t n_y,
              int16_t n_z) :
         X(n_x),
         Y(n_y),
         Z(n_z) {}
      int16_t X;
      int16_t Y;
      int16_t Z;
   };

   static CAccelerometerController& GetInstance() {
      static CAccelerometerController cInstance;
      return cInstance;
   }

   bool GetStatus() {
      return m_bStatus;
   }

   SSample GetSample();

private:
   CAccelerometerController();

   bool m_bStatus;

   const uint8_t m_unDeviceAddress = 0x68;
};


#endif
