
/***********************************************************/
/***********************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "clock.h"
#include "huart_controller.h"
#include "task_scheduler.h"
#include "nfc_controller.h"

/***********************************************************/
/***********************************************************/

#define FONT_BOLD   "\e[1m"
#define FONT_NORMAL "\e[0m"

/***********************************************************/
/***********************************************************/

struct SRxCounter : CNFCController::SRxFunctor {     
   virtual void operator()(const uint8_t* pun_data, uint8_t un_length) {
      Count++;
   }
   uint16_t Count = 0;
};

/***********************************************************/
/***********************************************************/

struct SMyUserFunctor : CTaskScheduler::SUserFunctor {
   /* user defined struct for tracking the state of each face */
   struct SFace {
      CTaskScheduler::SController& Controller;
      SRxCounter RxInitiatorCounter;
      SRxCounter RxTargetCounter;
      uint32_t RxTotal = 0;
      bool IsInitiator = false;

      SFace(CTaskScheduler::SController& s_controller) :
         Controller(s_controller) {}
   };

   /* a collection of the faces */
   CContainer<SFace, 6> Faces;
   uint32_t LastTimestamp = 0;
   uint32_t LastDiagnosticsTimestamp = 0;

   /* constructor */
   SMyUserFunctor() {
      /* create faces on the connected ports */
      for(CTaskScheduler::SController& s_controller : CTaskScheduler::GetInstance().GetControllers()) {
         /* create a face instance */
         SFace* psFace = Faces.Insert(s_controller);
         if(psFace != nullptr) {
            /* set up the functors */
            psFace->Controller.NFC.SetInitiatorRxFunctor(psFace->RxInitiatorCounter);
            psFace->Controller.NFC.SetTargetRxFunctor(psFace->RxTargetCounter);
         }
      }
   }

   virtual void operator()(uint32_t un_timestamp) override {
      /* print diagnostics information and reset the counters each second */
      if(un_timestamp - LastDiagnosticsTimestamp > 1000) {
         CHUARTController::GetInstance().Print("[%05lu] ", un_timestamp);
         for(SFace& s_face : Faces) {
            char pchBuffer[6];
            snprintf(pchBuffer, sizeof pchBuffer, "%u/%u", s_face.RxInitiatorCounter.Count, s_face.RxTargetCounter.Count);
            CHUARTController::GetInstance().Print("%c: %5s ", CPortController::PortToChar(s_face.Controller.Port), pchBuffer);
            if(s_face.IsInitiator) {
               /* if inactive, transition to target mode */
               if(s_face.RxInitiatorCounter.Count == 0) {
                  s_face.Controller.NFC.SetInitiatorPolicy(CNFCController::EInitiatorPolicy::Disable);
                  s_face.IsInitiator = false;
               }
            }
            else {
               if(s_face.RxTargetCounter.Count == 0) {
                  /* if inactive, transition to initiator mode with probability 0.125 */
                  if(random() < (RANDOM_MAX >> 3)) {
                     s_face.Controller.NFC.SetInitiatorPolicy(CNFCController::EInitiatorPolicy::Continuous);
                     s_face.IsInitiator = true;
                  }
               }
            }
            /* clear the counters */
            s_face.RxTotal += (s_face.RxInitiatorCounter.Count + s_face.RxTargetCounter.Count);
            s_face.RxInitiatorCounter.Count = 0;
            s_face.RxTargetCounter.Count = 0;
         }
         CHUARTController::GetInstance().Print("\r\n");
         /* update diagnostics timestamp */
         LastDiagnosticsTimestamp = un_timestamp;
      }
      /* print extended diagnostics on key press */
      if(CHUARTController::GetInstance().HasData()) {
         while(CHUARTController::GetInstance().HasData()) {
            CHUARTController::GetInstance().Read();
         }
         CHUARTController::GetInstance().Print(FONT_BOLD "[%05lu] ", un_timestamp);
         for(SFace& s_face : Faces) {
            CHUARTController::GetInstance().Print("%c: %5u ",
               CPortController::PortToChar(s_face.Controller.Port),
               s_face.RxTotal);
         }
         CHUARTController::GetInstance().Print(FONT_NORMAL "\r\n");
      }
   }
};

/***********************************************************/
/***********************************************************/

int main() {
   /* Enable interrupts */
   CInterruptController::GetInstance().Enable();
   /* create an instance of the user code */
   SMyUserFunctor sMyUserFunctor;
   /* assign it to the task schedule and start infinite loop */
   CTaskScheduler::GetInstance().SetUserFunctor(sMyUserFunctor);
   CTaskScheduler::GetInstance().Execute();
   return 0;
}

/***********************************************************/
/***********************************************************/

