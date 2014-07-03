#include "timer.h"

#include <avr/io.h>
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

// COverflowInterrupt Methods  ///////////////////////////////////////////////////////

void CTimer::COverflowInterrupt::ServiceRoutine() {
   // copy these to local variables so they can be stored in registers
   // (volatile variables must be read from memory on every access)
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

CTimer::COverflowInterrupt::COverflowInterrupt(CTimer* pc_timer, uint8_t un_intr_vect_num) : 
   m_pcTimer(pc_timer) {
   Register(this, un_intr_vect_num);
}

// Member functions ////////////////////////////////////////////////////////////////

CTimer::CTimer() : 
   m_unOverflowCount(0),
   m_unTimerMilliseconds(0),
   m_unTimerFraction(0),
   m_cOverflowInterrupt(this, TIMER0_OVF_vect_num) {
   /* Enable timer 0 */
   //sbi(TCCR0A, WGM01);
   //sbi(TCCR0A, WGM00);
   TCCR0A |= (_BV(WGM00) | _BV(WGM01));
   
    /* Set prescaler to 64 */
   //sbi(TCCR0B, CS01);
   //sbi(TCCR0B, CS00);
   TCCR0B |= (_BV(CS00) | _BV(CS01));

   /* Enable overflow interrupt */
   //sbi(TIMSK0, TOIE0);
   TIMSK0 |= _BV(TOIE0);
}

uint32_t CTimer::GetMilliseconds()
{
   uint32_t m;
   uint8_t oldSREG = SREG;

   // disable interrupts while we read timer0_millis or we might get an
   // inconsistent value (e.g. in the middle of a write to timer0_millis)
   cli();
   m = m_unTimerMilliseconds;
   SREG = oldSREG;

   return m;
}


uint32_t CTimer::GetMicroseconds() {
   uint32_t m;
   uint8_t oldSREG = SREG, t;
   cli();
   m = m_unOverflowCount;
   t = TCNT0;
   if ((TIFR0 & _BV(TOV0)) && (t < 255))
      m++;
   SREG = oldSREG;
   return ((m << 8) + t) * (64 / CLOCK_CYCLES_PER_MICROSECOND());
}


void CTimer::Delay(uint32_t ms)
{
   uint16_t start = (uint16_t)GetMicroseconds();
   while (ms > 0) {
      if (((uint16_t)GetMicroseconds() - start) >= 1000) {
         ms--;
         start += 1000;
      }
   }
}

