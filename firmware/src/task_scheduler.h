
#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <stdint.h>

#include "nfc_controller.h"
#include "port_controller.h"

class CTaskScheduler {
public:
   /* user code interface */
   struct SUserFunctor {
      virtual void operator()(uint32_t un_timestamp) = 0;
   };

   // STask // SRxTxTask // SNFCTask
   struct SController {
      SController(CPortController::EPort e_port) :
         Enabled(false),
         LastUpdate(0u),
         Port(e_port) {}
      bool Enabled;
      uint32_t LastUpdate;
      CPortController::EPort Port;
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
 
};


#endif




