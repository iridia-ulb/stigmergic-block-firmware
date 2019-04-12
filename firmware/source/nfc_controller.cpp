
//TODO remove this
#include <firmware.h>

#include <clock.h>

#include "nfc_controller.h"

#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)
#define PN532_ACKBYTE1                      (0x00)
#define PN532_ACKBYTE2                      (0xFF)

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

#define NFC_DEFAULT_TIMEOUT                 600
#define NFC_INITIATOR_TIMEOUT               200

const uint8_t CNFCController::m_punFirmwareVersion[] = {
    0x32, /* IC:        PN532 */
    0x01, /* Version:   1 */
    0x06, /* Revision:  6 */
    0x07, /* Support:   7 */
};

const uint8_t CNFCController::m_punConfigureSAMArguments[] = {
   0x01, /* normal mode */
   20u,  /* timeout: 50ms * 20 = 1s */
   1u,   /* use IRQ pin */
};

const uint8_t CNFCController::m_punTgInitAsTargetArguments[] = {
   /* Mode */
   0x00,                /* no constraints */
   /* MifareParams */
   0x04, 0x00,          /* SENS_RES */
   0x12, 0x34, 0x56,    /* NFCID1 */
   0x40,                /* SEL_RES - DEP only */
   /* FeliCaParams */
   0x01, 0xFE, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, /* NFCID2t */
   0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, /* PAD */
   0xFF, 0xFF,                                     /* System code */
   /* NFCID3t */
   0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
   /* Length of general bytes */
   0x00,
   /* Length of historical bytes */
   0x00,
};

const uint8_t CNFCController::m_punInJumpForDEPArguments[] = {
   /* active mode */
   0x01,
   /* 424 Kbps */
   0x02,
   /* PassiveInitiatorData, NFCID3i, Gi not set */
   0x00,
};

/***********************************************************/
/***********************************************************/

uint8_t CNFCController::m_punTxRxBuffer[] = {0};
uint8_t CNFCController::m_unTxRxLength = 0;

/***********************************************************/
/***********************************************************/

CNFCController::CNFCController() :
   m_psTargetTxFunctor(nullptr),
   m_psTargetRxFunctor(nullptr),
   m_psInitiatorTxFunctor(nullptr),
   m_psInitiatorRxFunctor(nullptr),
   m_eSelectedCommand(ECommand::ConfigureSAM),
   m_eState(EState::Standby),
   m_unWatchdogTimer(0) {}

/***********************************************************/
/***********************************************************/

void CNFCController::Step(EEvent e_event) {
   switch(m_eState) {
   case EState::Standby:
      if(e_event == EEvent::Init) {
         WriteCmd(ECommand::ConfigureSAM, m_punConfigureSAMArguments, sizeof m_punConfigureSAMArguments);
      }
      break;
   case EState::WaitingForAck:
      if(e_event == EEvent::Interrupt) {
         if(ReadAck()) {
            m_eState = EState::WaitingForResp;
         }
         else {
            /* cancel command */
            WriteAck();
            /* try to reinitialize */
            WriteCmd(ECommand::ConfigureSAM, m_punConfigureSAMArguments, sizeof m_punConfigureSAMArguments);
         }
      }
      break;
   case EState::WaitingForResp:
      if(e_event == EEvent::Interrupt) {
         if(!ReadResp()) {
            /* cancel command */
            WriteAck();
            /* try to return to the previous mode */
            if(m_eSelectedCommand == ECommand::InDataExchange ||
               m_eSelectedCommand == ECommand::InJumpForDEP) {
               WriteCmd(ECommand::InJumpForDEP, m_punInJumpForDEPArguments, sizeof m_punInJumpForDEPArguments);
            }
            else {
               WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
         }
         else {
            switch(m_eSelectedCommand) {
            case ECommand::ConfigureSAM:
               WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               break;
            case ECommand::TgInitAsTarget:
               WriteCmd(ECommand::TgGetData, nullptr, 0);
               break;
            case ECommand::TgGetData:              
               if(m_psTargetRxFunctor) {
                  (*m_psTargetRxFunctor)(m_punTxRxBuffer, m_unTxRxLength);
               }
               WriteCmd(ECommand::TgSetData, m_punTxRxBuffer, 
                        m_psTargetTxFunctor ? (*m_psTargetTxFunctor)(m_punTxRxBuffer, sizeof m_punTxRxBuffer) : 0);
               break;
            case ECommand::TgSetData:
               WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               break;
            case ECommand::InJumpForDEP:
               WriteCmd(ECommand::InDataExchange, m_punTxRxBuffer,
                        m_psInitiatorTxFunctor ? (*m_psInitiatorTxFunctor)(m_punTxRxBuffer, sizeof m_punTxRxBuffer) : 0);
               break;
            case ECommand::InDataExchange:
               if(m_psInitiatorRxFunctor) {
                  (*m_psInitiatorRxFunctor)(m_punTxRxBuffer, m_unTxRxLength);
               }
               /* continue to transmit as initiator again */
               WriteCmd(ECommand::InJumpForDEP, m_punInJumpForDEPArguments, sizeof m_punInJumpForDEPArguments);
               break;
            default: 
               /* unimplemented command selected */
               m_eState = EState::Failed;
               break;
            }
         }
      }
      else if(m_unWatchdogTimer != 0) {
         uint32_t unTimer = CClock::GetInstance().GetMilliseconds() - m_unWatchdogTimer;
         switch(m_eSelectedCommand) {
         case ECommand::InJumpForDEP:
            if(unTimer > NFC_INITIATOR_TIMEOUT) {
               /* cancel command */
               WriteAck();
               /* go to target mode */
               WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
            break;
         case ECommand::TgInitAsTarget:
            if(unTimer > NFC_DEFAULT_TIMEOUT) {
               /* cancel command */
               WriteAck();
               /* go to initiator mode */
               WriteCmd(ECommand::InJumpForDEP, m_punInJumpForDEPArguments, sizeof m_punInJumpForDEPArguments);
            }
            break;
         default:
            if(unTimer > NFC_DEFAULT_TIMEOUT) {
               /* cancel command */
               WriteAck();
               /* go to target mode */
               WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
         }
      }
      break;
   case EState::Failed:
      fprintf(CFirmware::GetInstance().m_psHUART, "Failure\r\n");
      /* try to reset */
      WriteCmd(ECommand::ConfigureSAM, m_punConfigureSAMArguments, sizeof m_punConfigureSAMArguments);
      break;
   }
   m_unLastStepTimestamp = CClock::GetInstance().GetMilliseconds();
}

/***********************************************************/
/***********************************************************/

bool CNFCController::ReadAck() {
   CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS, CTWController::EMode::RECEIVE);
   bool bAcknowledged =
      (CTWController::GetInstance().Receive() == PN532_I2C_READY) &&
      (CTWController::GetInstance().Receive() == PN532_PREAMBLE) &&
      (CTWController::GetInstance().Receive() == PN532_STARTCODE1) &&
      (CTWController::GetInstance().Receive() == PN532_STARTCODE2) &&
      (CTWController::GetInstance().Receive() == PN532_ACKBYTE1) &&
      (CTWController::GetInstance().Receive() == PN532_ACKBYTE2) &&
      (CTWController::GetInstance().Receive() == PN532_POSTAMBLE);
   CTWController::GetInstance().Receive(false);
   CTWController::GetInstance().Stop();
   /* reset the watchdog timer */
   m_unWatchdogTimer = CClock::GetInstance().GetMilliseconds();
   return bAcknowledged;
}

/***********************************************************/
/***********************************************************/

bool CNFCController::ReadResp() {
   CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS,
                                          CTWController::EMode::RECEIVE);
   /* check if the selected command has a status byte */
   bool bHasStatusByte = 
      (m_eSelectedCommand == ECommand::TgGetData)      ||
      (m_eSelectedCommand == ECommand::TgSetData)      ||
      (m_eSelectedCommand == ECommand::InDataExchange) ||
      (m_eSelectedCommand == ECommand::InJumpForDEP);
   /* unset watchdog timer */
   m_unWatchdogTimer = 0;
   /* check the header */    
   if((CTWController::GetInstance().Receive() != PN532_I2C_READY)  ||
      (CTWController::GetInstance().Receive() != PN532_PREAMBLE)   ||
      (CTWController::GetInstance().Receive() != PN532_STARTCODE1) ||
      (CTWController::GetInstance().Receive() != PN532_STARTCODE2)) {
      fprintf(CFirmware::GetInstance().m_psHUART, "E1\r\n");
      CTWController::GetInstance().Receive(false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* check the packet length */
   uint8_t unPacketLength = CTWController::GetInstance().Receive();
   uint8_t unPacketLengthChecksum = CTWController::GetInstance().Receive();
   if(((unPacketLength + unPacketLengthChecksum) & 0xFF) != 0u) {
      fprintf(CFirmware::GetInstance().m_psHUART, "E2\r\n");
      CTWController::GetInstance().Receive(false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* store the number of returned bytes without the command byte,
      the direction byte, and if applicable the status byte */
   m_unTxRxLength = unPacketLength - (bHasStatusByte ? 3u : 2u);
   /* prevent buffer overflow */
   if(m_unTxRxLength > sizeof m_punTxRxBuffer) {
      fprintf(CFirmware::GetInstance().m_psHUART, "E3\r\n");
      CTWController::GetInstance().Receive(false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* check the frame direction */
   if(CTWController::GetInstance().Receive() != PN532_PN532TOHOST) {
      fprintf(CFirmware::GetInstance().m_psHUART, "E4\r\n");
      CTWController::GetInstance().Receive(false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* check if the response is to the correct command */
   if(CTWController::GetInstance().Receive() != static_cast<uint8_t>(m_eSelectedCommand) + 1u) {
      fprintf(CFirmware::GetInstance().m_psHUART, "E5\r\n");
      CTWController::GetInstance().Receive(false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* initialize checksum */
   uint8_t unChecksum = 
      PN532_PN532TOHOST + static_cast<uint8_t>(m_eSelectedCommand) + 1u;
   /* read the status byte if applicable */
   if(bHasStatusByte) {
      uint8_t unStatusByte = CTWController::GetInstance().Receive();
      unChecksum += unStatusByte;
      if(unStatusByte != 0x00) {
         fprintf(CFirmware::GetInstance().m_psHUART, "E6: 0x%02x\r\n", unStatusByte);
         CTWController::GetInstance().Receive(false);
         CTWController::GetInstance().Stop();
         return false;
      }
   }
   /* transfer the data */
   for(uint8_t un_byte_index = 0;
       un_byte_index < m_unTxRxLength;
       un_byte_index++) {
      m_punTxRxBuffer[un_byte_index] = CTWController::GetInstance().Receive();
      unChecksum += m_punTxRxBuffer[un_byte_index];
   }
   /* validate the data checksum */
   unChecksum += CTWController::GetInstance().Receive();
   if(unChecksum != 0x00) {
      fprintf(CFirmware::GetInstance().m_psHUART, "E7\r\n");
      CTWController::GetInstance().Receive(false);
      CTWController::GetInstance().Stop();
      return false;
   }
   return true;
}

/***********************************************************/
/***********************************************************/

void CNFCController::WriteAck() {
   CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS, CTWController::EMode::TRANSMIT);
   CTWController::GetInstance().Transmit(PN532_PREAMBLE);
   CTWController::GetInstance().Transmit(PN532_STARTCODE1);
   CTWController::GetInstance().Transmit(PN532_STARTCODE2);
   CTWController::GetInstance().Transmit(PN532_ACKBYTE1);
   CTWController::GetInstance().Transmit(PN532_ACKBYTE2);
   CTWController::GetInstance().Transmit(PN532_POSTAMBLE);
   CTWController::GetInstance().Stop();
}

/***********************************************************/
/***********************************************************/

void CNFCController::WriteCmd(ECommand e_command, const uint8_t* pun_arguments, uint8_t un_arguments_length) {
   /* check if the selected command requires the target number */
   bool bRequiresTarget = (e_command == ECommand::InDataExchange);
   /* start the transmission, send header */
   CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS, CTWController::EMode::TRANSMIT);
   CTWController::GetInstance().Transmit(PN532_PREAMBLE);
   CTWController::GetInstance().Transmit(PN532_STARTCODE1);
   CTWController::GetInstance().Transmit(PN532_STARTCODE2);
   /* the sent length is the sum of the argument bytes, the command byte, the direction byte,
      and if applicable the target number */
   uint8_t unDataLength = un_arguments_length + (bRequiresTarget ? 3u : 2u);
   /* send the length, direction, and command bytes */
   CTWController::GetInstance().Transmit(unDataLength);
   CTWController::GetInstance().Transmit(~unDataLength + 1);
   CTWController::GetInstance().Transmit(PN532_HOSTTOPN532);
   CTWController::GetInstance().Transmit(static_cast<uint8_t>(e_command));
   /* initialize the checksum */
   uint8_t unChecksum = PN532_HOSTTOPN532 + static_cast<uint8_t>(e_command);
   /* send the target number (always 1) if applicable */
   if(bRequiresTarget) {
      CTWController::GetInstance().Transmit(1u);
      unChecksum += 1u;
   }
   /* send pun_arguments */
   for(uint8_t un_index = 0; un_index < un_arguments_length; un_index++) {
      CTWController::GetInstance().Transmit(pun_arguments[un_index]);
      unChecksum += pun_arguments[un_index];
   }
   CTWController::GetInstance().Transmit(~unChecksum + 1);
   CTWController::GetInstance().Transmit(PN532_POSTAMBLE);
   /* end the transmission */
   CTWController::GetInstance().Stop();
   /* update the state to waiting for acknowledgement */
   m_eSelectedCommand = e_command;
   m_eState = EState::WaitingForAck;
   /* set watchdog timer */
   m_unWatchdogTimer = CClock::GetInstance().GetMilliseconds();
}

