
/***********************************************************/
/***********************************************************/

#include "clock.h"
#include "huart_controller.h"
#include "task_scheduler.h"
#include "nfc_controller.h"

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
      bool IsInitiator = false;
      bool IsActive = true;

      SFace(CTaskScheduler::SController& s_controller) :
         Controller(s_controller) {}
   };

   /* a collection of the faces */
   CContainer<SFace, 6> Faces;

   /* constructor */
   SMyUserFunctor() {
      /* start debug */
      CHUARTController::GetInstance().Print("[%05lu] Connected ports: ", CClock::GetInstance().GetMilliseconds());
      for(CPortController::EPort e_port : CPortController::GetInstance().GetConnectedPorts()) {
         CHUARTController::GetInstance().Print("%c ", CPortController::PortToChar(e_port));
      }
      CHUARTController::GetInstance().Print("\r\n");
      /* end debug */
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
      /* logic for when to initiate communication */
      for(SFace& s_face : Faces) {
         if(!s_face.IsActive) {
            if(s_face.IsInitiator) {
               s_face.Controller.NFC.SetInitiatorPolicy(CNFCController::EInitiatorPolicy::Disable);
               s_face.IsInitiator = false;
               s_face.IsActive = true;
            }
            else {
               /* with low probability */
               s_face.Controller.NFC.SetInitiatorPolicy(CNFCController::EInitiatorPolicy::Continuous);
               s_face.IsInitiator = true;
               s_face.IsActive = true;
            }
         }
      }
      /* print diagnostics information and reset the counters every second */
      if(un_timestamp - LastDiagnosticsTimestamp > 1000) {
         for(SFace& s_face : Faces) {
            CHUARTController::GetInstance().Print("%c: %2u/%-2u ", 
               CPortController::PortToChar(s_face.Controller.Port),
               s_face.RxInitiatorCounter.Count,
               s_face.RxTargetCounter.Count);
            s_face.IsActive = s_face.IsInitiator ?
               (s_face.RxInitiatorCounter.Count > 0) : (s_face.RxTargetCounter.Count > 0);
            s_face.RxInitiatorCounter.Count = 0;
            s_face.RxTargetCounter.Count = 0;
         }
         CHUARTController::GetInstance().Print("\r\n");
         /* update diagnostics timestamp */
         LastDiagnosticsTimestamp = un_timestamp;
      }
   }

   uint32_t LastTimestamp = 0;
   uint32_t LastDiagnosticsTimestamp = 0;
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

void Log(const char* pch_message, bool b_bold) {
   const char* pchSetBold = "\e[1m";
   const char* pchClearBold = "\e[0m";

   uint32_t un_time = CClock::GetInstance().GetMilliseconds();
   CHUARTController::GetInstance().Print("[%05lu] %s%s%s\r\n", un_time, (b_bold ? pchSetBold : ""), pch_message, pchClearBold);
}

/***********************************************************/
/***********************************************************/


