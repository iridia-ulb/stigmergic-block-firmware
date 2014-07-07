#include "timer.h"

#include <avr/interrupt.h>

#define CLOCK_CYCLES_PER_MICROSECOND() ( F_CPU / 1000000L )
#define CLOCK_CYCLES_TO_MICROSECONDS(a) ( (a) / CLOCK_CYCLES_PER_MICROSECOND() )
#define MICROSECONDS_TO_CLOCK_CYCLES(a) ( (a) * CLOCK_CYCLES_PER_MICROSECOND() )

/* 
 * The prescaler is set so that timer0 ticks every 64 clock cycles, and the
 * the overflow handler is called every 256 ticks.
 */
#define MICROSECONDS_PER_TIMER0_OVERFLOW (CLOCK_CYCLES_TO_MICROSECONDS(64 * 256))

/* 
 * The whole number of milliseconds per timer0 overflow
 */
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

/* The fractional number of milliseconds per timer0 overflow. we shift right
 * by three to fit these numbers into a byte. (for the clock speeds we care
 * about - 8 and 16 MHz - this doesn't lose precision.)
 */
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

/****************************************/
/****************************************/

void CTimer::COverflowInterrupt::ServiceRoutine() {
   /* copy readings to local variables so they can be stored in registers
      (volatile variables must be read from memory on every access) */
   uint32_t unTimerMilliseconds = m_pcTimer->m_unTimerMilliseconds;
   uint8_t  unTimerFraction = m_pcTimer->m_unTimerFraction;
   unTimerMilliseconds += MILLIS_INC;
   unTimerFraction += FRACT_INC;
   if (unTimerFraction >= FRACT_MAX) {
      unTimerFraction -= FRACT_MAX;
      unTimerMilliseconds += 1;
   }
   m_pcTimer->m_unTimerFraction = unTimerFraction;
   m_pcTimer->m_unTimerMilliseconds = unTimerMilliseconds;
   m_pcTimer->m_unOverflowCount++;
}

/****************************************/
/****************************************/

CTimer::COverflowInterrupt::COverflowInterrupt(CTimer* pc_timer, uint8_t un_intr_vect_num) : 
   m_pcTimer(pc_timer) {
   Register(this, un_intr_vect_num);
}

/****************************************/
/****************************************/

CTimer::CTimer(volatile uint8_t& un_ctrl_reg_a,
               uint8_t un_ctrl_reg_a_config,
               volatile uint8_t& un_ctrl_reg_b,
               uint8_t un_ctrl_reg_b_config,
               volatile uint8_t& un_intr_mask_reg,
               uint8_t un_intr_mask_reg_config,
               volatile uint8_t& un_intr_flag_reg,
               volatile uint8_t& un_cnt_reg,
               uint8_t un_intr_num) :
   m_unControlRegisterA(un_ctrl_reg_a),
   m_unControlRegisterB(un_ctrl_reg_b),
   m_unInterruptMaskRegister(un_intr_mask_reg),
   m_unInterruptFlagRegister(un_intr_flag_reg),
   m_unCountRegister(un_cnt_reg),
   m_unOverflowCount(0),
   m_unTimerMilliseconds(0),
   m_unTimerFraction(0),
   m_cOverflowInterrupt(this, un_intr_num) {
   m_unControlRegisterA = un_ctrl_reg_a_config;
   m_unControlRegisterB = un_ctrl_reg_b_config;
   m_unInterruptMaskRegister = un_intr_mask_reg_config;


   /* Enable timer 0 */
   //sbi(TCCR0A, WGM01);
   //sbi(TCCR0A, WGM00);
   //TCCR0A |= (_BV(WGM00) | _BV(WGM01));
    /* Set prescaler to 64 */
   //sbi(TCCR0B, CS01);
   //sbi(TCCR0B, CS00);
   //TCCR0B |= (_BV(CS00) | _BV(CS01));
   /* Enable overflow interrupt */
   //sbi(TIMSK0, TOIE0);
   //TIMSK0 |= _BV(TOIE0);
}

/****************************************/
/****************************************/

uint32_t CTimer::GetMilliseconds() {
   uint32_t m;
   uint8_t oldSREG = SREG;

   // disable interrupts while we read m_unTimerMilliseconds or we might get an
   // inconsistent value (e.g. in the middle of a write to m_unTimerMilliseconds)
   cli();
   m = m_unTimerMilliseconds;
   SREG = oldSREG;

   return m;
}

/****************************************/
/****************************************/

uint32_t CTimer::GetMicroseconds() {
   uint32_t m;
   uint8_t oldSREG = SREG, t;
   cli();
   m = m_unOverflowCount;
   t = m_unCountRegister;
   if ((m_unInterruptFlagRegister & _BV(TOV0)) && (t < 255))
      m++;
   SREG = oldSREG;
   return ((m << 8) + t) * (64 / CLOCK_CYCLES_PER_MICROSECOND());
}

/****************************************/
/****************************************/

void CTimer::Delay(uint32_t un_delay_ms) {
   uint16_t unStart = (uint16_t)GetMicroseconds();
   while (un_delay_ms > 0) {
      if (((uint16_t)GetMicroseconds() - unStart) >= 1000) {
         un_delay_ms--;
         unStart += 1000;
      }
   }
}

