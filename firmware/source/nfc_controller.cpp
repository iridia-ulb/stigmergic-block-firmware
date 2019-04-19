
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

const uint8_t CNFCController::m_punAckFrame[] = {
    0x00, /* preamble */
    0x00, /* start code 1 */
    0xFF, /* start code 2 */
    0x00, /* ack code 1 */
    0xFF, /* ack code 2 */
    0x00, /* postamble */
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
         if(!ReadAck()) {
            /* cancel command */
            WriteAck();
            /* try to reinitialize */
            WriteCmd(ECommand::ConfigureSAM, m_punConfigureSAMArguments, sizeof m_punConfigureSAMArguments);
         }
         else {
            m_eState = EState::WaitingForResp;
         }
      }
      break;
   case EState::WaitingForResp:
      if(e_event == EEvent::Interrupt) {
         if(!ReadResp()) {
            /* try cancel command */
            if(WriteAck()) {              
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
               /* hardware failure */
               m_eState = EState::Standby;
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
               m_eState = EState::Standby;
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
   }
   m_unLastStepTimestamp = CClock::GetInstance().GetMilliseconds();
}

/***********************************************************/
/***********************************************************/

bool CNFCController::ReadAck() {
   bool bStatus = CTWController::GetInstance().Read(PN532_I2C_ADDRESS, sizeof m_punAckFrame + 1, m_punTxRxBuffer);
   bStatus = bStatus && (m_punTxRxBuffer[0] == PN532_I2C_READY);
   for(uint8_t un_index = 0; un_index < sizeof m_punAckFrame; un_index++) {
      bStatus = bStatus && (m_punTxRxBuffer[un_index + 1] == m_punAckFrame[un_index]);
   }
   /* reset the watchdog timer */
   m_unWatchdogTimer = CClock::GetInstance().GetMilliseconds();
   return bStatus;
}

/***********************************************************/
/***********************************************************/

bool CNFCController::ReadResp() {
   /* unset watchdog timer */
   m_unWatchdogTimer = 0;
   /* byte for temporarily storing received data */
   uint8_t unReceivedByte = 0;
   /* check if the selected command has a status byte */
   bool bHasStatusByte = 
      (m_eSelectedCommand == ECommand::TgGetData)      ||
      (m_eSelectedCommand == ECommand::TgSetData)      ||
      (m_eSelectedCommand == ECommand::InDataExchange) ||
      (m_eSelectedCommand == ECommand::InJumpForDEP);
   /* start reading */
   bool bStatus = CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS, CTWController::EMode::Receive);
   /* check the header */
   bStatus = bStatus && CTWController::GetInstance().Receive(&unReceivedByte);
   bStatus = bStatus && (unReceivedByte == PN532_I2C_READY);
   bStatus = bStatus && CTWController::GetInstance().Receive(&unReceivedByte);
   bStatus = bStatus && (unReceivedByte == PN532_PREAMBLE);
   bStatus = bStatus && CTWController::GetInstance().Receive(&unReceivedByte);
   bStatus = bStatus && (unReceivedByte == PN532_STARTCODE1);
   bStatus = bStatus && CTWController::GetInstance().Receive(&unReceivedByte);
   bStatus = bStatus && (unReceivedByte == PN532_STARTCODE2);
   if(!bStatus) {
      fprintf(CFirmware::GetInstance().m_psHUART, "R1\r\n");
      CTWController::GetInstance().Receive(&unReceivedByte, false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* check the packet length */
   uint8_t unPacketLength = 0; ;
   uint8_t unPacketLengthChecksum = 0; 
   bStatus = bStatus && CTWController::GetInstance().Receive(&unPacketLength);
   bStatus = bStatus && CTWController::GetInstance().Receive(&unPacketLengthChecksum);
   bStatus = bStatus && ((unPacketLength + unPacketLengthChecksum) & 0xFF) == 0u;
   if(!bStatus) {
      fprintf(CFirmware::GetInstance().m_psHUART, "R2\r\n");
      CTWController::GetInstance().Receive(&unReceivedByte, false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* store the number of returned bytes without the command byte,
      the direction byte, and if applicable the status byte */
   m_unTxRxLength = unPacketLength - (bHasStatusByte ? 3u : 2u);
   /* prevent buffer overflow */
   if(m_unTxRxLength > sizeof m_punTxRxBuffer) {
      fprintf(CFirmware::GetInstance().m_psHUART, "R3\r\n");
      CTWController::GetInstance().Receive(&unReceivedByte, false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* check the frame direction */
   bStatus = bStatus && CTWController::GetInstance().Receive(&unReceivedByte);
   bStatus = bStatus && (unReceivedByte == PN532_PN532TOHOST);
   if(!bStatus) {
      fprintf(CFirmware::GetInstance().m_psHUART, "R4\r\n");
      CTWController::GetInstance().Receive(&unReceivedByte, false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* check if the response is to the correct command */
   bStatus = bStatus && CTWController::GetInstance().Receive(&unReceivedByte);
   bStatus = bStatus && (unReceivedByte == static_cast<uint8_t>(m_eSelectedCommand) + 1u);
   if(!bStatus) {
      fprintf(CFirmware::GetInstance().m_psHUART, "R5\r\n");
      CTWController::GetInstance().Receive(&unReceivedByte, false);
      CTWController::GetInstance().Stop();
      return false;
   }
   /* initialize checksum */
   uint8_t unChecksum = 
      PN532_PN532TOHOST + static_cast<uint8_t>(m_eSelectedCommand) + 1u;
   /* read the status byte if applicable */
   if(bHasStatusByte) {
      uint8_t unStatusByte = 0;
      bStatus = bStatus && CTWController::GetInstance().Receive(&unStatusByte);
      bStatus = bStatus && (unStatusByte == 0x00);
      if(!bStatus) {
         fprintf(CFirmware::GetInstance().m_psHUART, "R6: 0x%02x\r\n", unStatusByte);
         CTWController::GetInstance().Receive(&unReceivedByte, false);
         CTWController::GetInstance().Stop();
         return false;
      }
      unChecksum += unStatusByte;
   }
   /* transfer the data */
   for(uint8_t un_byte_index = 0; un_byte_index < m_unTxRxLength; un_byte_index++) {
      bStatus = bStatus && CTWController::GetInstance().Receive(m_punTxRxBuffer + un_byte_index);
      unChecksum += m_punTxRxBuffer[un_byte_index];
   }
   if(!bStatus) {
      fprintf(CFirmware::GetInstance().m_psHUART, "R7\r\n");
      CTWController::GetInstance().Receive(&unReceivedByte, false);
      CTWController::GetInstance().Stop();
   }
   /* validate the data checksum */
   bStatus = bStatus && CTWController::GetInstance().Receive(&unReceivedByte);
   unChecksum += unReceivedByte;
   bStatus = bStatus && (unChecksum == 0x00);
   if(!bStatus) {
      fprintf(CFirmware::GetInstance().m_psHUART, "R8\r\n");
   }
   CTWController::GetInstance().Receive(&unReceivedByte, false);
   CTWController::GetInstance().Stop();
   return bStatus;
}

/***********************************************************/
/***********************************************************/

bool CNFCController::WriteAck() {
   return CTWController::GetInstance().Write(PN532_I2C_ADDRESS, sizeof m_punAckFrame, m_punAckFrame);
}

/***********************************************************/
/***********************************************************/

bool CNFCController::WriteCmd(ECommand e_command, const uint8_t* pun_arguments, uint8_t un_arguments_length) {
   /* check if the selected command requires the target number */
   bool bRequiresTarget = (e_command == ECommand::InDataExchange);
   /* start the transmission, send header */
   bool bStatus = CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS, CTWController::EMode::Transmit);
   bStatus = bStatus && CTWController::GetInstance().Transmit(PN532_PREAMBLE, PN532_STARTCODE1, PN532_STARTCODE2);
   /* the sent length is the sum of the argument bytes, the command byte, the direction byte,
      and if applicable the target number */
   uint8_t unDataLength = un_arguments_length + (bRequiresTarget ? 3u : 2u);
   /* send the length, direction, and command bytes */
   bStatus = bStatus && CTWController::GetInstance().Transmit(unDataLength, ~unDataLength + 1, PN532_HOSTTOPN532, static_cast<uint8_t>(e_command));
   /* initialize the checksum */
   uint8_t unChecksum = PN532_HOSTTOPN532 + static_cast<uint8_t>(e_command);
   /* send the target number (always 1) if applicable */
   if(bRequiresTarget) {
      bStatus = bStatus && CTWController::GetInstance().Transmit(1u);
      unChecksum += 1u;
   }
   /* send pun_arguments */
   for(uint8_t un_index = 0; un_index < un_arguments_length; un_index++) {
      bStatus = bStatus && CTWController::GetInstance().Transmit(pun_arguments[un_index]);
      unChecksum += pun_arguments[un_index];
   }
   bStatus = bStatus && CTWController::GetInstance().Transmit(~unChecksum + 1, PN532_POSTAMBLE);
   /* end the transmission */
   bStatus = bStatus && CTWController::GetInstance().Stop();
   if(!bStatus) {
      return bStatus;
   }
   /* update the state to waiting for acknowledgement */
   m_eSelectedCommand = e_command;
   m_eState = EState::WaitingForAck;
   /* set watchdog timer */
   m_unWatchdogTimer = CClock::GetInstance().GetMilliseconds();
   return bStatus;
}

