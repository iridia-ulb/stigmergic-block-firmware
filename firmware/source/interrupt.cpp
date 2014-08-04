#include "interrupt.h"

// http://www.mikrocontroller.net/articles/AVR_Interrupt_Routinen_mit_C%2B%2B
//#include <avr/interrupt.h>

/* Initialise the owner array */
CInterrupt* CInterrupt::ppcInterruptOwner[] = {0};

/* Register Method */
void CInterrupt::Register(CInterrupt* pc_interrupt, uint8_t un_interrupt_num) {
   ppcInterruptOwner[un_interrupt_num - 1] = pc_interrupt;
}

/* Handlers */
   void CInterrupt::Handler01() {
      if(ppcInterruptOwner[0])
         ppcInterruptOwner[0]->ServiceRoutine();
   }

// void CInterrupt::Handler02() {
//    if(ppcInterruptOwner[1])
//       ppcInterruptOwner[1]->ServiceRoutine();
// }

// void CInterrupt::Handler03() {
//    if(ppcInterruptOwner[2])
//       ppcInterruptOwner[2]->ServiceRoutine();
// }

// void CInterrupt::Handler04() {
//    if(ppcInterruptOwner[3])
//       ppcInterruptOwner[3]->ServiceRoutine();
// }

// void CInterrupt::Handler05() {
//    if(ppcInterruptOwner[4])
//       ppcInterruptOwner[4]->ServiceRoutine();
// }

// void CInterrupt::Handler06() {
//    if(ppcInterruptOwner[5])
//       ppcInterruptOwner[5]->ServiceRoutine();
// }

// void CInterrupt::Handler07() {
//    if(ppcInterruptOwner[6])
//       ppcInterruptOwner[6]->ServiceRoutine();
// }

// void CInterrupt::Handler08() {
//    if(ppcInterruptOwner[7])
//       ppcInterruptOwner[7]->ServiceRoutine();
// }

// void CInterrupt::Handler09() {
//    if(ppcInterruptOwner[8])
//       ppcInterruptOwner[8]->ServiceRoutine();
// }

void CInterrupt::Handler10() {
   if(ppcInterruptOwner[9])
      ppcInterruptOwner[9]->ServiceRoutine();
}

void CInterrupt::Handler11() {
   if(ppcInterruptOwner[10])
      ppcInterruptOwner[10]->ServiceRoutine();
}

void CInterrupt::Handler12() {
   if(ppcInterruptOwner[11])
      ppcInterruptOwner[11]->ServiceRoutine();
}

// void CInterrupt::Handler13() {
//    if(ppcInterruptOwner[12])
//       ppcInterruptOwner[12]->ServiceRoutine();
// }

// void CInterrupt::Handler14() {
//    if(ppcInterruptOwner[13])
//       ppcInterruptOwner[13]->ServiceRoutine();
// }

// void CInterrupt::Handler15() {
//    if(ppcInterruptOwner[14])
//       ppcInterruptOwner[14]->ServiceRoutine();
// }

void CInterrupt::Handler16() {
   if(ppcInterruptOwner[15])
      ppcInterruptOwner[15]->ServiceRoutine();
}

// void CInterrupt::Handler17() {
//    if(ppcInterruptOwner[16])
//       ppcInterruptOwner[16]->ServiceRoutine();
// }

// void CInterrupt::Handler18() {
//    if(ppcInterruptOwner[17])
//       ppcInterruptOwner[17]->ServiceRoutine();
// }

// void CInterrupt::Handler19() {
//    if(ppcInterruptOwner[18])
//       ppcInterruptOwner[18]->ServiceRoutine();
// }

// void CInterrupt::Handler20() {
//    if(ppcInterruptOwner[19])
//       ppcInterruptOwner[19]->ServiceRoutine();
// }

// void CInterrupt::Handler21() {
//    if(ppcInterruptOwner[20])
//       ppcInterruptOwner[20]->ServiceRoutine();
// }

// void CInterrupt::Handler22() {
//    if(ppcInterruptOwner[21])
//       ppcInterruptOwner[21]->ServiceRoutine();
// }

// void CInterrupt::Handler23() {
//    if(ppcInterruptOwner[22])
//       ppcInterruptOwner[22]->ServiceRoutine();
// }

// void CInterrupt::Handler24() {
//    if(ppcInterruptOwner[23])
//       ppcInterruptOwner[23]->ServiceRoutine();
// }

// void CInterrupt::Handler25() {
//    if(ppcInterruptOwner[24])
//       ppcInterruptOwner[24]->ServiceRoutine();
// }

