
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

class CSystem {
public:

   static CSystem& GetInstance() {
      static CSystem cInstance;
      return cInstance;
   }

   bool IsPowerConnected();

   bool IsBatteryCharging();

   void Reset();

   uint16_t GetBatteryVoltage();

   uint16_t GetProcessorTemperature();

private: 
   
   enum class EADCChannel : uint8_t {
      ADC0 = 0x00, ADC1 = 0x01, ADC2 = 0x02, ADC3 = 0x03,
      ADC4 = 0x04, ADC5 = 0x05, ADC6 = 0x06, ADC7 = 0x07,
      TEMP = 0x08, AREF = 0x0E, GND = 0x0F
   };

   uint8_t ReadADC(EADCChannel e_channel);

   CSystem();

};

#endif
