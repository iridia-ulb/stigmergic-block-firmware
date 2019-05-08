#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdint.h>
#include <stddef.h>

void* operator new (size_t n, void* ptr) {
   return ptr;
};

template <typename T, size_t length>
class CContainer {
public:

   ~CContainer() {
      for(T& t : *this) t.~T();
   }

   T* begin() {
      return reinterpret_cast<T*>(data);
   }

   T* end() {
      return begin() + used_count;
   }

   template<typename... Arguments>
   T* Insert(const Arguments&... arguments) {
      if(used_count < length) {
         T* created_instance = new (end()) T(arguments...);
         ++used_count;
         return created_instance;
      }
      else {
         return nullptr;
      }
   }

   size_t Size() {
      return used_count;
   }

private:
   unsigned char data[length * sizeof(T)];
   size_t used_count = 0;
   
};

#endif
