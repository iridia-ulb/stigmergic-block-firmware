#ifndef INTERRUPT_H
#define INTERRUPT_H

//#include <avr/interrupt.h>
//#include <avr/io.h>
#include <stdint.h>
// http://www.mikrocontroller.net/articles/AVR_Interrupt_Routinen_mit_C%2B%2B

class CInterrupt {

private:
   /* Owner Array */
   static CInterrupt* ppcInterruptOwner[25];

   /* Handlers */
   static void Handler01() __asm__("__vector_1") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler02() __asm__("__vector_2") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler03() __asm__("__vector_3") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler04() __asm__("__vector_4") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler05() __asm__("__vector_5") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler06() __asm__("__vector_6") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler07() __asm__("__vector_7") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler08() __asm__("__vector_8") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler09() __asm__("__vector_9") __attribute__((__signal__, __used__, __externally_visible__));
   static void Handler10() __asm__("__vector_10") __attribute__((__signal__, __used__, __externally_visible__));
   static void Handler11() __asm__("__vector_11") __attribute__((__signal__, __used__, __externally_visible__));
   static void Handler12() __asm__("__vector_12") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler13() __asm__("__vector_13") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler14() __asm__("__vector_14") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler15() __asm__("__vector_15") __attribute__((__signal__, __used__, __externally_visible__));
   static void Handler16() __asm__("__vector_16") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler17() __asm__("__vector_17") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler18() __asm__("__vector_18") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler19() __asm__("__vector_19") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler20() __asm__("__vector_20") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler21() __asm__("__vector_21") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler22() __asm__("__vector_22") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler23() __asm__("__vector_23") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler24() __asm__("__vector_24") __attribute__((__signal__, __used__, __externally_visible__));
   // static void Handler25() __asm__("__vector_25") __attribute__((__signal__, __used__, __externally_visible__));
   

private:
   virtual void ServiceRoutine() = 0;

public:
   static void Register(CInterrupt* pc_interrupt, uint8_t un_intr_vect_num);
   
};

#endif
