#ifndef TIMER_H
#define TIMER_H
 
#include "interrupt.h"

class CTimer {
public:
   /* constructor */
   CTimer();

   uint32_t GetMilliseconds();
   uint32_t GetMicroseconds();
   void Delay(uint32_t ms);

private:
   volatile uint32_t m_unOverflowCount;
   volatile uint32_t m_unTimerMilliseconds;
   volatile uint8_t  m_unTimerFraction;

private:   

   class COverflowInterrupt : public CInterrupt {
   private:
      CTimer* m_pcTimer;
      void ServiceRoutine();
   public:
      COverflowInterrupt(CTimer* pc_timer, uint8_t un_intr_vect_num);
   } m_cOverflowInterrupt;

   friend COverflowInterrupt;
};

#endif
