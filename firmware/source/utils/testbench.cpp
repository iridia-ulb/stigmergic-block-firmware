#include "container.h"
#include <iostream>

struct Complex;

std::ostream& operator<<(std::ostream& stream, const Complex& complex);

struct Complex {
   Complex(int nr, int ni) :
      r(nr), i(ni) {
      std::cerr << "c: " << *this << std::endl;
   }

   Complex(const Complex& other) :
      r(other.r),
      i(other.i) {
      std::cerr << "cc: " << *this << std::endl;
   }

   ~Complex() {
      std::cerr << "d: " << *this << std::endl;
   }

   int r, i;
};

std::ostream& operator<<(std::ostream& stream, const Complex& complex) {
   stream << complex.r << " + " << complex.i << "i";
   return stream;
}

int main() {
   container<Complex, 6> my_container;

   std::cerr << sizeof my_container << ", " << sizeof(Complex) << std::endl;

   

   for(int r = 0, i = 0; my_container.insert(r,i) ; r++, i--);
   

   for(Complex& c_complex : my_container) {
      std::cerr << c_complex << std::endl;
   }


   return 0;
}
