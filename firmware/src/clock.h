
#ifndef CLOCK_H
#define CLOCK_H
 
#include <avr/io.h>
#include <avr/interrupt.h>

#include "interrupt.h"

class CClock {
public:

   static const CClock& GetInstance() {
      static CClock cInstance(TCCR0A,
                              TCCR0A | (_BV(WGM00) | _BV(WGM01)),
                              TCCR0B,
                              TCCR0B | (_BV(CS00) | _BV(CS01)),
                              TIMSK0,
                              TIMSK0 | _BV(TOIE0),
                              TIFR0,
                              TCNT0,
                              TIMER0_OVF_vect_num);
      return cInstance;
   }

   uint32_t GetMilliseconds() const;
   uint32_t GetMicroseconds() const;
   void Delay(uint32_t ms) const;

private:

   /* constructor */
   CClock(volatile uint8_t& un_ctrl_reg_a,
          uint8_t un_ctrl_reg_a_config,
          volatile uint8_t& un_ctrl_reg_b,
          uint8_t un_ctrl_reg_b_config,
          volatile uint8_t& un_intr_mask_reg,
          uint8_t un_intr_mask_reg_config,
          volatile uint8_t& un_intr_flag_reg,
          volatile uint8_t& un_cnt_reg,
          uint8_t un_intr_num);

   volatile uint8_t& m_unControlRegisterA;
   volatile uint8_t& m_unControlRegisterB;
   volatile uint8_t& m_unInterruptMaskRegister;
   volatile uint8_t& m_unInterruptFlagRegister;
   volatile uint8_t& m_unCountRegister;

   volatile uint32_t m_unOverflowCount;
   volatile uint32_t m_unTimerMilliseconds;
   volatile uint8_t  m_unTimerFraction;

private:   

   class COverflowInterrupt : public CInterrupt {
   private:
      CClock* m_pcClock;
      void ServiceRoutine();
   public:
      COverflowInterrupt(CClock* pc_timer, uint8_t un_intr_vect_num);
   } m_cOverflowInterrupt;

   friend COverflowInterrupt;
};

#endif
