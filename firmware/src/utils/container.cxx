#include "container.h"

void* operator new (size_t n, void* ptr) {
   return ptr;
};

