#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

class CPowerMonitor {
public:

   static CPowerMonitor& GetInstance() {
      static CPowerController cInstance;
      return cInstance;
   }

   bool IsPowerConnected() {
      return (PIND & PWR_MON_PGOOD);
   }

   bool IsBatteryCharging() {
      return (PIND & PWR_MON_CHG);
   }

   void Reset() {
      PORTD |= 0x10;
      DDRD |= 0x10;
   }

   uint16_t GetBatteryVoltage() {
      /* TODO merge the code from ADC class to here */
      return 0;
   }

   uint16_t GetProcessorTemperature() {
      /* TODO merge the code from ADC class to here */
      return 0;
   }  

private:
   CPowerMonitor() {
      /* Configure the BQ24075 monitoring pins */
      DDRD &= ~PWR_MON_MASK;
      PORTD &= ~PWR_MON_MASK;
   }
};


#endif
