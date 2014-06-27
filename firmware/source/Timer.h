#ifndef TIMER_H
#define TIMER_H

class Timer {
public:

   static Timer& instance() {
      return _timer;
   }
   unsigned long millis();
   unsigned long micros();
   void delay(unsigned long ms);
   void delayMicroseconds(unsigned int us);
   
private:   
   /* constructor */
   Timer();

   /* singleton instance */
   static Timer _timer;
   
};

#endif
