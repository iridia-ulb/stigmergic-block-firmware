
#ifndef PORT_CONTROLLER_H
#define PORT_CONTROLLER_H

#include <stdint.h>

#include "utils/container.h"

class CPortController {

public:

   enum class EPort : uint8_t {
      North      = 0,
      East       = 1,
      South      = 2,
      West       = 3,
      Top        = 4,
      Bottom     = 5,
      Disconnect = 8,
   };

   static EPort Ports[6];

   static char PortToChar(EPort e_port);

public:

   static CPortController& GetInstance() {
      static CPortController cInstance;
      return cInstance;
   }

   uint8_t GetInterrupts();

   bool Enable(EPort e_port);

   bool Disable(EPort e_port);

   void Select(EPort e_port);

   const CContainer<EPort, 6>& GetConnectedPorts() {
      return m_cConnectedPorts;
   }

private:

   CPortController();

   const uint8_t m_unResetDeviceAddress = 0x20;
   const uint8_t m_unInterruptDeviceAddress = 0x21;
   
   enum class ERegister : uint8_t {
      Input         = 0x00,
      Output        = 0x01,
      Configuration = 0x03,
   };

   CContainer<EPort, 6> m_cConnectedPorts;


};

#endif
