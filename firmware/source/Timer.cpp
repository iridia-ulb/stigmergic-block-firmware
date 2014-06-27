#include "Timer.h"

#include <Microcontroller.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdarg.h>

// Singleton instance //////////////////////////////////////////////////////////////

Timer Timer::_timer;

// Interrupt Data ////////////////////////////////////////////////////////////////

volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;

// Useful Macros ////////////////////////////////////////////////////////////////

#define CLOCK_CYCLES_PER_MICROSECOND() ( F_CPU / 1000000L )
#define CLOCK_CYCLES_TO_MICROSECONDS(a) ( (a) / CLOCK_CYCLES_PER_MICROSECOND() )
#define MICROSECONDS_TO_CLOCK_CYCLES(a) ( (a) * CLOCK_CYCLES_PER_MICROSECOND() )

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (CLOCK_CYCLES_TO_MICROSECONDS(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

// Overflow interrupt routine ////////////////////////////////////////////////////////////////

ISR(TIMER0_OVF_vect)
{
   // copy these to local variables so they can be stored in registers
   // (volatile variables must be read from memory on every access)
   unsigned long m = timer0_millis;
   unsigned char f = timer0_fract;

   m += MILLIS_INC;
   f += FRACT_INC;
   if (f >= FRACT_MAX) {
      f -= FRACT_MAX;
      m += 1;
   }

   timer0_fract = f;
   timer0_millis = m;
   timer0_overflow_count++;
}

// Member functions ////////////////////////////////////////////////////////////////

Timer::Timer() {
   /* Enable timer 0 */
   sbi(TCCR0A, WGM01);
   sbi(TCCR0A, WGM00);
   
    /* Set prescaler to 64 */
   sbi(TCCR0B, CS01);
   sbi(TCCR0B, CS00);

   /* Enable overflow interrupt */
   sbi(TIMSK0, TOIE0);
}

unsigned long Timer::millis()
{
   unsigned long m;
   uint8_t oldSREG = SREG;

   // disable interrupts while we read timer0_millis or we might get an
   // inconsistent value (e.g. in the middle of a write to timer0_millis)
   cli();
   m = timer0_millis;
   SREG = oldSREG;

   return m;
}

unsigned long Timer::micros() {
   unsigned long m;
   uint8_t oldSREG = SREG, t;
   cli();
   m = timer0_overflow_count;
   t = TCNT0;
   if ((TIFR0 & _BV(TOV0)) && (t < 255))
      m++;
   SREG = oldSREG;
   return ((m << 8) + t) * (64 / CLOCK_CYCLES_PER_MICROSECOND());
}

void Timer::delay(unsigned long ms)
{
   uint16_t start = (uint16_t)micros();
   while (ms > 0) {
      if (((uint16_t)micros() - start) >= 1000) {
         ms--;
         start += 1000;
      }
   }
}

/* Delay for the given number of microseconds.  Assumes a 8 or 16 MHz clock. */
void Timer::delayMicroseconds(unsigned int us)
{
   // for the 8 MHz internal clock

   // for a one- or two-microsecond delay, simply return.  the overhead of
   // the function calls takes more than two microseconds.  can't just
   // subtract two, since us is unsigned; we'd overflow.
   if (--us == 0)
      return;
   if (--us == 0)
      return;

   // the following loop takes half of a microsecond (4 cycles)
   // per iteration, so execute it twice for each microsecond of
   // delay requested.
   us <<= 1;
    
   // partially compensate for the time taken by the preceeding commands.
   // we can't subtract any more than this or we'd overflow w/ small delays.
   us--;

   // busy wait
   __asm__ __volatile__ (
      "1: sbiw %0,1" "\n\t" // 2 cycles
      "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
   );
}

