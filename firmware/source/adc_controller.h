#define ADC_CONTROLLER_H

#include <stdint.h>

#define ADC_MUX_MASK 0x0F

class CADCController {
public:

   enum class EChannel {
      ADC0 = 0x00,
      ADC1 = 0x01,
      ADC2 = 0x02,
      ADC3 = 0x03,
      ADC4 = 0x04,
      ADC5 = 0x05,
      ADC6 = 0x06,
      ADC7 = 0x07,
      TEMP = 0x08,
      AREF = 0x0E,
      GND = 0x0F
   };

   static CADCController& GetInstance() {
      static CADCController cInstance;
      return cInstance;
   }

   uint8_t Read(EChannel e_channel) {
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

private:

   CADCController() {
      /* Initialize the analog to digital converter */
      /* Use the internal 1.1V reference, left align result */
      ADMUX |= ((1 << REFS1) | (1 << REFS0) |
                (1 << ADLAR));
      /* Enable the ADC and set the prescaler to 64, (8MHz / 64 = 125kHz) */
      ADCSRA |= ((1 << ADEN) | (1 << ADPS2) | (1 << ADPS1));
   }
};
