
#include "task_scheduler.h"

/***********************************************************/
/***********************************************************/

#include "clock.h"

#define RESET_PERIOD 100
#define STEP_PERIOD 100

/***********************************************************/
/***********************************************************/

CTaskScheduler::CTaskScheduler() {
   /* for each connected port */
   for(CPortController::EPort e_port : 
       CPortController::GetInstance().GetConnectedPorts()) {
      /* create a controller */
      m_cControllers.Insert(e_port);
   }
}

/***********************************************************/
/***********************************************************/

void CTaskScheduler::Execute() {
   for(;;) {
      uint32_t unTimestamp = CClock::GetInstance().GetMilliseconds();
      uint8_t unIRQs = CPortController::GetInstance().GetInterrupts();
      SController* psSelectedController = nullptr;
      /***********************************************************************/
      /* Respond to a controller that (i) is enabled, (ii) has an interrupt,
         and (iii) has been waiting the longest                              */ 
      /***********************************************************************/
      for(SController& s_controller : m_cControllers) {
         bool bHasInterrupt = 
            (((unIRQs >> static_cast<uint8_t>(s_controller.Port)) & 0x01) != 0);
         if(s_controller.Enabled && bHasInterrupt) {
            if(psSelectedController == nullptr ||
               s_controller.LastUpdate < psSelectedController->LastUpdate) {
               psSelectedController = &s_controller;
            }
         }
      }      
      if(psSelectedController != nullptr) {
         /* update the timestamp */
         psSelectedController->LastUpdate = unTimestamp;
         /* select the port */
         CPortController::GetInstance().Select(psSelectedController->Port);
         /* step finite state machine */
         if(psSelectedController->NFC.Step(CNFCController::EEvent::Interrupt) == false) {
            /* unrecoverable error detected, trigger hard reset */
            CPortController::GetInstance().Disable(psSelectedController->Port);
            psSelectedController->Enabled = false;
         }
         /* restart the main loop */
         continue;
      }
      /***********************************************************************/
      /* Enable a disabled controller after RESET_PERIOD milliseconds        */ 
      /***********************************************************************/
      for(SController& s_controller : m_cControllers) {
         if(!s_controller.Enabled && (unTimestamp - s_controller.LastUpdate > RESET_PERIOD)) {
            if(psSelectedController == nullptr ||
               s_controller.LastUpdate < psSelectedController->LastUpdate) {
               psSelectedController = &s_controller;
            }
         }
      }
      if(psSelectedController != nullptr) {
         /* update the timestamp */
         psSelectedController->LastUpdate = unTimestamp;
         /* enable port */
         CPortController::GetInstance().Enable(psSelectedController->Port);
         psSelectedController->Enabled = true;
         /* restart the main loop */
         continue;
      }
      /***********************************************************************/
      /* Step an inactive controller after STEP_PERIOD milliseconds          */ 
      /***********************************************************************/
      for(SController& s_controller : m_cControllers) {
         if(s_controller.Enabled && (unTimestamp - s_controller.LastUpdate > STEP_PERIOD)) {
            if(psSelectedController == nullptr ||
               s_controller.LastUpdate < psSelectedController->LastUpdate) {
               psSelectedController = &s_controller;
            }
         }
      }
      if(psSelectedController != nullptr) {
         /* update the timestamp */
         psSelectedController->LastUpdate = unTimestamp;
         /* select the port */
         CPortController::GetInstance().Select(psSelectedController->Port);
         /* step the controller */
         if(psSelectedController->NFC.Step() == false) {
            /* unrecoverable error detected, trigger hard reset */
            CPortController::GetInstance().Disable(psSelectedController->Port);
            psSelectedController->Enabled = false;
         }
         /* restart the main loop */
         continue;
      }
      /***********************************************************************/
      /* No time sensitive tasks to execute -> execute user code             */
      /***********************************************************************/
      if(m_psUserFunctor) {
         (*m_psUserFunctor)(unTimestamp);
      }
   }
}

/***********************************************************/
/***********************************************************/

