
#ifndef TW_CONTROLLER_H
#define TW_CONTROLLER_H

#include <stdint.h>

class CTWController {
public:

   /****************************************/
   /****************************************/

   enum class EMode : uint8_t {
      Transmit = 0,
      Receive = 1,
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
      bool bStatus = StartWait(un_device, EMode::Transmit);
      for(uint8_t un_index = 0; un_index < un_length; un_index++) {
         bStatus = bStatus && Transmit(pun_data[un_index]);
      }
      bStatus = bStatus && Stop();
      return bStatus;
   }

   /****************************************/
   /****************************************/

   bool Write(uint8_t un_device, uint8_t un_data) {
      return Write(un_device, 1, &un_data);
   }

   /****************************************/
   /****************************************/

   template<typename TRegister>
   bool WriteRegister(uint8_t un_device, TRegister t_register, uint8_t un_length, const uint8_t* pun_data) {
      bool bStatus = StartWait(un_device, EMode::Transmit);
      bStatus = bStatus && Transmit(static_cast<uint8_t>(t_register));
      for(uint8_t un_index = 0; un_index < un_length; un_index++)
         bStatus = bStatus && Transmit(pun_data[un_index]);
      bStatus = bStatus && Stop();
      return bStatus;
   }

   /****************************************/
   /****************************************/

   template<typename TRegister>
   bool WriteRegister(uint8_t un_device, TRegister t_register, uint8_t un_data) {
      return WriteRegister(un_device, t_register, 1, &un_data);
   }

   /****************************************/
   /****************************************/

   bool Read(uint8_t un_device, uint8_t un_length, uint8_t* pun_data) {
      bool bStatus = StartWait(un_device, EMode::Receive);
      /* read in data while ready */
      for(uint8_t un_index = 0; un_index < (un_length - 1); un_index++) {
         bStatus = bStatus && Receive(pun_data + un_index);
      }
      bStatus = bStatus && Receive(pun_data + un_length - 1, false);
      bStatus = bStatus && Stop();
      return bStatus;
   }

   /****************************************/
   /****************************************/

   template<typename TRegister>
   bool ReadRegister(uint8_t un_device, TRegister t_register, uint8_t un_length, uint8_t* pun_data) {
      bool bStatus = StartWait(un_device, EMode::Transmit);
      bStatus = bStatus && Transmit(static_cast<uint8_t>(t_register));
      /* repeated start */
      bStatus = bStatus && Start(un_device, EMode::Receive);
      for(uint8_t un_index = 0; un_index < (un_length - 1); un_index++)
         bStatus = bStatus && Receive(pun_data + un_index);
      bStatus = bStatus && Receive(pun_data + un_length - 1, false);
      bStatus = bStatus && Stop();
      return bStatus;
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
   bool Stop();

   /**
       @brief Issues a start condition and sends address and transfer direction

       @param    addr address and transfer direction of I2C device
       @returns   0   device accessible
       @returns   1   failed to access device
   */
   bool Start(uint8_t un_device, EMode e_mode);


   /**
      @brief Issues a start condition and sends address and transfer direction

      If device is busy, use ack polling to wait until device ready
      @param    addr address and transfer direction of I2C device
      @return   none
   */
   bool StartWait(uint8_t un_device, EMode e_mode);


   /**
      @brief Send multiple bytes to I2C device
      @param    un_byte byte to be transfered
      @returns   true write successful
      @returns   false write failed
   */
   template<typename... Data>
   bool Transmit(uint8_t un_byte, Data... data) {
      bool bSuccess = Transmit(un_byte);
      if(bSuccess) {
         bSuccess = Transmit(data...);
      }
      return bSuccess;
   }

   /**
      @brief Send one byte to I2C device
      @param    data byte to be transfered
      @returns   true write successful
      @returns   false write failed
   */
   bool Transmit(uint8_t un_byte);

   /**
      @brief    read one byte from the I2C device
      @param    pointer to where the data should be stored
      @param    true if we will request more data
      @return   byte read from I2C device
   */
   bool Receive(uint8_t* pun_byte, bool b_continue = true);

   /**
      @brief waits for the transceiver to become ready
   */
   bool Wait();

private:
   /* constructor */
   CTWController();

};

#endif
