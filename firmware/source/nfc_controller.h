
#ifndef NFC_CONTROLLER_H
#define NFC_CONTROLLER_H

#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)
#define PN532_ACKFRAME1                     (0x00) 
#define PN532_ACKFRAME2                     (0xFF)

#define PN532_I2C_ADDRESS                   (0x48 >> 1)
#define PN532_I2C_READBIT                   (0x01)
#define PN532_I2C_BUSY                      (0x00)
#define PN532_I2C_READY                     (0x01)

#define NFC_CMD_BUF_LEN                     64

#define NFC_FRAME_DIRECTION_INDEX           5
#define NFC_FRAME_ID_INDEX                  6
#define NFC_FRAME_DATA_INDEX                7

#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)

#include <stdint.h>

class CNFCController {

public:

   /* forward declarations */
   enum class ECommand : uint8_t;
   enum class EEvent;
   enum class EState;

   /* interface for tx and rx functors */
   struct STxFunctor {
      virtual uint8_t operator()(uint8_t* pun_data, uint8_t un_length) = 0;
   };

   struct SRxFunctor {
      virtual void operator()(const uint8_t* pun_data, uint8_t un_length) = 0;
   };

public:
   
   /* constructor */
   CNFCController();

   bool AppendEvent(EEvent e_event);

   /* TODO make private: only accepts events if ready */
   bool ProcessEvent(EEvent e_event);

   EState GetState() const {
      return m_eState;
   }

   void SetInitiatorTxFunctor(STxFunctor* ps_tx_functor) {
      m_psInitiatorTxFunctor = ps_tx_functor;
   }

   void SetInitiatorRxFunctor(SRxFunctor* ps_rx_functor) {
      m_psInitiatorRxFunctor = ps_rx_functor;
   }

   void SetTargetTxFunctor(STxFunctor* ps_tx_functor) {
      m_psTargetTxFunctor = ps_tx_functor;
   }

   void SetTargetRxFunctor(SRxFunctor* ps_rx_functor) {
      m_psTargetRxFunctor = ps_rx_functor;
   }

private:

   bool ReadAck();

   bool ReadResp();

   void Write(ECommand e_command, const uint8_t* pun_data, uint8_t un_data_length);

public:

   STxFunctor* m_psTargetTxFunctor;
   SRxFunctor* m_psTargetRxFunctor;
   STxFunctor* m_psInitiatorTxFunctor;
   SRxFunctor* m_psInitiatorRxFunctor;

   ECommand m_eSelectedCommand;
   EState m_eState;

   /* shared data buffer for reading / writing commands */
   static uint8_t m_punTxRxBuffer[64];
   static uint8_t m_unTxRxLength;

   const static uint8_t m_punFirmwareVersion[4];

   const static uint8_t m_punConfigureSAMArguments[3];

   const static uint8_t m_punTgInitAsTargetArguments[37];
   const static uint8_t m_punInJumpForDEPArguments[8];

public:

   enum class ECommand : uint8_t {
      Diagnose              = 0x00,
      GetFirmwareVersion    = 0x02,
      GetGeneralStatus      = 0x04,
      ReadRegister          = 0x06,
      WriteRegister         = 0x08,
      ReadGPIO              = 0x0C,
      WriteGPIO             = 0x0E,
      SetSerialBaudrate     = 0x10,
      SetParameters         = 0x12,
      ConfigureSAM          = 0x14,
      PowerDown             = 0x16,
      ConfigureRf           = 0x32,
      TestRfRegulation      = 0x58,
      InJumpForDEP          = 0x56,
      InJumpForPSL          = 0x46,
      InListPassiveTarget   = 0x4A,
      InATR                 = 0x50,
      InPSL                 = 0x4E,
      InDataExchange        = 0x40,
      InCommunicateThru     = 0x42,
      InDeselect            = 0x44,
      InRelease             = 0x52,
      InSelect              = 0x54,
      InAutoPoll            = 0x60,
      TgInitAsTarget        = 0x8C,
      TgSetGeneralBytes     = 0x92,
      TgGetData             = 0x86,
      TgSetData             = 0x8E,
      TgSetMetaData         = 0x94,
      TgGetInitiatorCommand = 0x88,
      TgResponseToInitiator = 0x90,
      TgGetTargetStatus     = 0x8A,
   };

   enum class EEvent {
      Init = 0, Interrupt, Transceive,
   };

   enum class EState {
      Standby = 0, Ready, WaitingForAck, WaitingForResp, Failed,
   };
};

#endif
