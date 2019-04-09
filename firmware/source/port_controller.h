#ifndef PORT_CONTROLLER_H
#define PORT_CONTROLLER_H

#include <stdint.h>
#include <interrupt.h>

#define EXT_INT0_SENSE_MASK        0x03
#define EXT_INT0_FALLING_EDGE      0x02
#define EXT_INT0_ENABLE            0x01

#define PCA9554_RST_ADDR           0x20
#define PCA9554_IRQ_ADDR           0x21

#define NUM_PORTS 6


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

   void EnablePort(EPort e_enable);
   void DisablePort(EPort e_disable);
   void SelectPort(EPort e_select);

   bool IsPortConnected(EPort e_target);

private:

   CPortController();
   
   enum class EPCA9554Register : uint8_t {
      INPUT          = 0x00, // R
      OUTPUT         = 0x01, // R/W
      CONFIG         = 0x03  // R/W
   };


};

#endif
