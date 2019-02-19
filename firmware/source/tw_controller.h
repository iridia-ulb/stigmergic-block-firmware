#ifndef TW_CONTROLLER_H
#define TW_CONTROLLER_H

#include <stdint.h>

class CTWController {
public:

   /****************************************/
   /****************************************/

   enum class EMode : uint8_t {
      TRANSMIT = 0,
      RECEIVE = 1,
   };

   /****************************************/
   /****************************************/

   static CTWController& GetInstance() {
      static CTWController cInstance;
      return cInstance;
   }

   /****************************************/
   /****************************************/

   bool Write(uint8_t un_device, uint8_t un_length, const uint8_t* pun_data) {
      StartWait(un_device, EMode::TRANSMIT);
      for(uint8_t un_index = 0; un_index < un_length; un_index++)
         Transmit(pun_data[un_index]);
      Stop();
      return true;
   }

   /****************************************/
   /****************************************/

   bool Write(uint8_t un_device, uint8_t un_data) {
      return Write(un_device, 1, &un_data);
   }

   /****************************************/
   /****************************************/

   template<typename REGISTER_ID>
   bool Write(uint8_t un_device, REGISTER_ID t_register, uint8_t un_length, const uint8_t* pun_data) {
      StartWait(un_device, EMode::TRANSMIT);
      Transmit(static_cast<uint8_t>(t_register));
      for(uint8_t un_index = 0; un_index < un_length; un_index++)
         Transmit(pun_data[un_index]);
      Stop();
      return true;
   }

   /****************************************/
   /****************************************/

   template<typename REGISTER_ID>
   bool Write(uint8_t un_device, REGISTER_ID t_register, uint8_t un_data) {
      return Write(un_device, t_register, 1, &un_data);
   }

   /****************************************/
   /****************************************/

   void Read(uint8_t un_device, uint8_t un_length, uint8_t* pun_data) {
      StartWait(un_device, EMode::RECEIVE);
      for(uint8_t un_index = 0; un_index < (un_length - 1); un_index++)
         pun_data[un_index] = Receive();
      pun_data[un_length - 1] = Receive(false);
      Stop();
   }

   /****************************************/
   /****************************************/

   uint8_t Read(uint8_t un_device) {
      uint8_t unData;
      Read(un_device, 1, &unData);
      return unData;
   }

   /****************************************/
   /****************************************/

   template<typename REGISTER_ID>
   void Read(uint8_t un_device, REGISTER_ID t_register, uint8_t un_length, uint8_t* pun_data) {
      StartWait(un_device, EMode::TRANSMIT);
      Transmit(static_cast<uint8_t>(t_register));
      /* repeated start */
      Start(un_device, EMode::RECEIVE);
      for(uint8_t un_index = 0; un_index < (un_length - 1); un_index++)
         pun_data[un_index] = Receive();
      pun_data[un_length - 1] = Receive(false);
      Stop();
   }

   /****************************************/
   /****************************************/

   template<typename REGISTER_ID>
   uint8_t Read(uint8_t un_device, REGISTER_ID t_register) {
      uint8_t unData;
      Read(un_device, t_register, 1, &unData);
      return unData;
   }

   /****************************************/
   /****************************************/

   void Enable();

   /****************************************/
   /****************************************/

   void Disable();

   /****************************************/
   /****************************************/

   /**
       @brief Terminates the data transfer and releases the I2C bus
       @return none
   */
   void Stop();

   /**
       @brief Issues a start condition and sends address and transfer direction

       @param    addr address and transfer direction of I2C device
       @retval   0   device accessible
       @retval   1   failed to access device
   */
   bool Start(uint8_t un_device, EMode e_mode);


   /**
      @brief Issues a start condition and sends address and transfer direction

      If device is busy, use ack polling to wait until device ready
      @param    addr address and transfer direction of I2C device
      @return   none
   */
   void StartWait(uint8_t un_device, EMode e_mode);


   /**
      @brief Send one byte to I2C device
      @param    data byte to be transfered
      @retval   0 write successful
      @retval   1 write failed
   */
   bool Transmit(uint8_t un_data);


   /**
      @brief    read one byte from the I2C device
      @param    true if we will request more data
      @return   byte read from I2C device
   */
   uint8_t Receive(bool b_continue = true);

   /**
      @brief waits for the transceiver to become ready
   */
   void Wait();

private:
   /* constructor */
   CTWController();

};

#endif
