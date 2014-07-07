#ifndef TIMER_H
#define TIMER_H
 
#include "interrupt.h"

class CTimer {
public:
   /* constructor */
   CTimer(volatile uint8_t& un_ctrl_reg_a,
          uint8_t un_ctrl_reg_a_config,
          volatile uint8_t& un_ctrl_reg_b,
          uint8_t un_ctrl_reg_b_config,
          volatile uint8_t& un_intr_mask_reg,
          uint8_t un_intr_mask_reg_config,
          volatile uint8_t& un_intr_flag_reg,
          volatile uint8_t& un_cnt_reg,
          uint8_t un_intr_num);

   uint32_t GetMilliseconds();
   uint32_t GetMicroseconds();
   void Delay(uint32_t ms);

private:
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
      CTimer* m_pcTimer;
      void ServiceRoutine();
   public:
      COverflowInterrupt(CTimer* pc_timer, uint8_t un_intr_vect_num);
   } m_cOverflowInterrupt;

   friend COverflowInterrupt;
};

#endif
