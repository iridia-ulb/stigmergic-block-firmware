#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <nfc_controller.h>
#include <port_controller.h>

#include <stdint.h>


class CTaskScheduler {
public:
   /* user code interface */
   struct SUserFunctor {
      virtual void operator(uint32_t un_timestamp) {}
   };

   // STask // SRxTxTask // SNFCTask
   struct SController {
      SController(EPort e_port) :
         Enabled(false),
         LastUpdateTimestamp(0u),
         Port(e_port) {}
      bool Enabled;
      uint32_t LastUpdate;
      EPort Port;
      CNFCController NFC;
   };

public:
   static CTaskScheduler& GetInstance() {
      static CTaskScheduler cInstance;
      return cInstance;
   }

   CContainer<SController, 6>& GetControllers() {
      return m_cControllers;
   }

   void SetUserFunctor(SUserFunctor& s_user_functor) {
      m_psUserFunctor = &s_user_functor;
   }

   void Execute();

private:
   /* constructor */
   CTaskScheduler();

   SUserFunctor* m_psUserFunctor;

   CContainer<SController, 6> m_cControllers;

   
}


#endif




