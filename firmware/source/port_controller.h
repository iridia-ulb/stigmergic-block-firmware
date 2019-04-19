#ifndef PORT_CONTROLLER_H
#define PORT_CONTROLLER_H

#include <stdint.h>

class CPortController {

public:

   enum class EPort : uint8_t {
      NORTH  = 0,
      EAST   = 1,
      SOUTH  = 2,
      WEST   = 3,
      TOP    = 4,
      BOTTOM = 5,
      NULLPORT = 8
   };

public:

   static CPortController& GetInstance() {
      static CPortController cInstance;
      return cInstance;
   }

   void Init();
   
   uint8_t GetInterrupts();

   bool EnablePort(EPort e_port);

   bool DisablePort(EPort e_port);

   void SelectPort(EPort e_port);

   bool IsPortConnected(EPort e_port);

private:

   CPortController();
   
   enum class EPCA9554Register : uint8_t {
      INPUT          = 0x00, // R
      OUTPUT         = 0x01, // R/W
      CONFIG         = 0x03  // R/W
   };


};

#endif
