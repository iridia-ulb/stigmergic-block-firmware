#include "firmware.h"

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
      EPort Port;
      CNFCController& NFC;
      SRxCounter RxInitiatorCounter;
      SRxCounter RxTargetCounter;
      bool IsInitiator = false;
      bool IsActive = true;

      SFace(EPort e_port,
            CNFCController& c_nfc_controller) : 
         Port(e_port),
         NFC(c_nfc_controller) {}
   };

   /* a collection of the faces */
   CContainer<SFace, 6> Faces;

   /* constructor */
   SMyUserFunctor() {
      /* create faces on the connected ports */
      for(CTaskScheduler::SController& s_controller : CTaskScheduler::GetInstance().GetControllers()) {
         /* create a face instance */
         SFace* psFace = Faces.Insert(s_controller.Port, s_controller.NFC);
         if(psFace != nullptr) {
            /* set up the functors */
            psFace->NFC.SetInitiatorRxFunctor(psFace->RxInitiatorCounter);
            psFace->NFC.SetTargetRxFunctor(psFace->RxTargetCounter);
         }
      }
   }

   virtual void operator(uint32_t un_timestamp) override {
      /* logic for when to initiate communication */
      for(SFace& s_face : Faces) {
         if(!s_face.IsActive) {
            if(s_face.IsInitiator) {
               s_face.NFC.SetInitiatorPolicy(CNFCController::EInitiatorPolicy::Disable);
               s_face.IsInitiator = false;
            }
            else {
               /* with low probability */
               s_face.NFC.SetInitiatorPolicy(CNFCController::EInitiatorPolicy::Continuous);
               s_face.IsInitiator = true;
            }
         }
      }
      /* print diagnostics information and reset the counters every second */
      if(un_timestamp - LastDiagnosticsTimestamp > 1000) {
         for(SFace& s_face : Faces) {
            CHUARTController::GetInstance().Print("%s: %2u/%-2u ", 
               ToString(s_face.Port), s_face.RxInitiatorCounter.Count, s_face.RxTargetCounter.Count);
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

void CFirmware::Execute() {
   /* Enable interrupts */
   CInterruptController::GetInstance().Enable();
   /* create an instance of the user code */
   SMyUserFunctor sMyUserFunctor;
   /* assign it to the task schedule and start infinite loop */
   CTaskScheduler::GetInstance().SetUserFunctor(sMyUserFunctor);
   CTaskScheduler::GetInstance().Execute();
}

/***********************************************************/
/***********************************************************/

int main() {
   // TODO remove CFirmware and just start the task scheduler directly
   CFirmware::GetInstance().Execute();
   return 0;
}

/***********************************************************/
/***********************************************************/

void CFirmware::Log(const char* pch_message, bool b_bold) {
   const char* pchSetBold = "\e[1m";
   const char* pchClearBold = "\e[0m";

   uint32_t un_time = CClock::GetInstance().GetMilliseconds();
   CHUARTController::GetInstance().Print("[%05lu] %s%s%s\r\n", un_time, (b_bold ? pchSetBold : ""), pch_message, pchClearBold);
}

/***********************************************************/
/***********************************************************/

/*
CHUARTController::GetInstance().Print("[%05lu] Connected ports: ", CClock::GetInstance().GetMilliseconds());
for(EPort e_port : cConnectedPorts) {
   CHUARTController::GetInstance().Print("%c ", CPortController::PortToChar(e_port));
}
CHUARTController::GetInstance().Print("\r\n", CPortController::PortToChar(e_port));
*/


