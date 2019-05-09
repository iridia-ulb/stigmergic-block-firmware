
#include "system.h"

/***********************************************************/
/***********************************************************/

#include <avr/io.h>

/***********************************************************/
/***********************************************************/

#define PWR_MON_MASK   0xC0
#define PWR_MON_PGOOD  0x40
#define PWR_MON_CHG    0x80
#define ADC_MUX_MASK   0x0F

/***********************************************************/
/***********************************************************/

CSystem::CSystem() {
   /* configure the BQ24075 monitoring pins */
   DDRD &= ~PWR_MON_MASK;
   PORTD &= ~PWR_MON_MASK;
   /* initialize the analog to digital converter */
   /* use the internal 1.1V reference, left align result */
   ADMUX |= ((1 << REFS1) | (1 << REFS0) |
             (1 << ADLAR));
   /* enable the ADC and set the prescaler to 64, (8MHz / 64 = 125kHz) */
   ADCSRA |= ((1 << ADEN) | (1 << ADPS2) | (1 << ADPS1));
}

/***********************************************************/
/***********************************************************/

bool CSystem::IsPowerConnected() {
   return (PIND & PWR_MON_PGOOD);
}

/***********************************************************/
/***********************************************************/

bool CSystem::IsBatteryCharging() {
   return (PIND & PWR_MON_CHG);
}

/***********************************************************/
/***********************************************************/

void CSystem::Reset() {
   PORTD |= 0x10;
   DDRD |= 0x10;
}

/***********************************************************/
/***********************************************************/

uint16_t CSystem::GetBatteryVoltage() {
   return ReadADC(EADCChannel::ADC7) * 17u;
}

/***********************************************************/
/***********************************************************/

uint16_t CSystem::GetProcessorTemperature() {
   /* requires calibration */
   return ReadADC(EADCChannel::TEMP);
}  

/***********************************************************/
/***********************************************************/

uint8_t CSystem::ReadADC(CSystem::EADCChannel e_channel) {
   /* select the channel to do the conversion */
   uint8_t unADMuxSetting = ADMUX;
   unADMuxSetting &= ~ADC_MUX_MASK;
   unADMuxSetting |= static_cast<uint8_t>(e_channel);
   ADMUX = unADMuxSetting;
   /* Start conversion */
   ADCSRA |= (1 << ADSC);
   /* Wait for the conversion to complete */
   while((ADCSRA & (1 << ADSC)) != 0);
   /* Return the result */
   return ADCH;
}

/***********************************************************/
/***********************************************************/


