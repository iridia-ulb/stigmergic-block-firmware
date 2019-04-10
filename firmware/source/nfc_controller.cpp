
//TODO remove this
#include <firmware.h>

#include <clock.h>

#include "nfc_controller.h"

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
   m_eSelectedCommand(ECommand::GetFirmwareVersion),
   m_eState(EState::Standby),
   m_unWatchdogTimer(0) {}

/***********************************************************/
/***********************************************************/

bool CNFCController::AppendEvent(EEvent e_event) {
   // TODO: create a small circular buffer for events?
   // TODO: interrupts get passed through immediately (rx have priority over tx, i.e. EEvent::Transcieve)
   return ProcessEvent(e_event);
}

/***********************************************************/
/***********************************************************/

#define TEXT_RED "\e[1;31m"
#define TEXT_NORMAL "\e[0m"

/* private: only accepts events if ready */
bool CNFCController::ProcessEvent(EEvent e_event) {
   switch(m_eState) {
   case EState::Standby:
      WriteCmd(ECommand::GetFirmwareVersion, nullptr, 0);
      break;
   case EState::Ready:
      break;
   case EState::WaitingForAck:
      if(e_event == EEvent::Interrupt) {
         if(ReadAck()) {
            m_eState = EState::WaitingForResp;
         }
         else {
            fprintf(CFirmware::GetInstance().m_psHUART, "NAck\r\n");
            m_eState = EState::Failed;
         }
      }
      break;
   case EState::WaitingForResp:
      if(e_event == EEvent::Interrupt) {
         if(!ReadResp()) {
            // resend packet
            //WriteNack();
         }
         else {
            switch(m_eSelectedCommand) {
            case ECommand::GetFirmwareVersion:
               if(true) { // TODO check firmware version
                  WriteCmd(ECommand::ConfigureSAM, m_punConfigureSAMArguments, sizeof m_punConfigureSAMArguments);
               }
               break;
            case ECommand::ConfigureSAM:
               WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               break;
            case ECommand::TgInitAsTarget:
               WriteCmd(ECommand::TgGetData, nullptr, 0);
               break;
            case ECommand::TgGetData:
               if(m_punTxRxBuffer[0] == 0) {
                  if(m_psTargetRxFunctor) {
                     (*m_psTargetRxFunctor)(m_punTxRxBuffer + 1, m_unTxRxLength - 1);
                  }
                  WriteCmd(ECommand::TgSetData, m_punTxRxBuffer, 
                        m_psTargetTxFunctor ? (*m_psTargetTxFunctor)(m_punTxRxBuffer, sizeof m_punTxRxBuffer) : 0);
               }
               else {
                  fprintf(CFirmware::GetInstance().m_psHUART, TEXT_RED "TGD:" TEXT_NORMAL " %02x\r\n", m_punTxRxBuffer[0]);
                  /* failure: try return to target mode */
                  WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               }
               break;
            case ECommand::TgSetData:
               if(m_punTxRxBuffer[0] == 0) {
                  WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               }
               else {
                  fprintf(CFirmware::GetInstance().m_psHUART, TEXT_RED "TSD:" TEXT_NORMAL " %02x\r\n", m_punTxRxBuffer[0]);
                  /* failure: try return to target mode */
                  WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               }
               break;
            case ECommand::InJumpForDEP:
               if(m_punTxRxBuffer[0] == 0) {
                  /* set the logical number of the target */
                  m_punTxRxBuffer[0] = 1u;
                  WriteCmd(ECommand::InDataExchange, m_punTxRxBuffer,
                        m_psInitiatorTxFunctor ? (*m_psInitiatorTxFunctor)(m_punTxRxBuffer + 1, (sizeof m_punTxRxBuffer) - 1) + 1 : 1);
               }
               else {
                  fprintf(CFirmware::GetInstance().m_psHUART, TEXT_RED "DEP:" TEXT_NORMAL " %02x\r\n", m_punTxRxBuffer[0]);
                  /* failure: try return to target mode */
                  WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               }
               break;
            case ECommand::InDataExchange:
               if(m_punTxRxBuffer[0] == 0) {
                  if(m_psInitiatorRxFunctor) {
                     (*m_psInitiatorRxFunctor)(m_punTxRxBuffer + 1, m_unTxRxLength - 1);
                  }
                  /* return to target mode */
                  WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               }
               else {
                  fprintf(CFirmware::GetInstance().m_psHUART, TEXT_RED "IDX:" TEXT_NORMAL " %02x\r\n", m_punTxRxBuffer[0]);
                  /* failure: try return to target mode */
                  WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
               }
               break;
            default:
               m_eState = EState::Failed;
               break;
            } /* switch(m_eSelectedCommand) */
         } /* if response was valid */
      } /* if interrupt */
      else if((m_unWatchdogTimer != 0) && (CClock::GetInstance().GetMilliseconds() - m_unWatchdogTimer > NFC_WATCHDOG_THRES)) {
         /* cancel last command */
         WriteAck();
         /* try return to target mode or to execute InJumpForDEP */
         if(m_eSelectedCommand == ECommand::TgInitAsTarget) {
            WriteCmd(ECommand::InJumpForDEP, m_punInJumpForDEPArguments, sizeof m_punInJumpForDEPArguments);
         }
         else {
            WriteCmd(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
         }
      }
      break;
   case EState::Failed:
      fprintf(CFirmware::GetInstance().m_psHUART, "Failure!\r\n");
      /* try to reset */
      WriteCmd(ECommand::GetFirmwareVersion, nullptr, 0);
      break;
   }
   return true;
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
   return bAcknowledged;
}

/***********************************************************/
/***********************************************************/

bool CNFCController::ReadResp() {
   CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS,
                                          CTWController::EMode::RECEIVE);
   /* disable watchdog timer */
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
   /* store the number of returned bytes without the command byte and
      the direction byte */
   m_unTxRxLength = unPacketLength - 2u;
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
      fprintf(CFirmware::GetInstance().m_psHUART, "E6\r\n");
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
   /* start the transmission, send header */
   CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS, CTWController::EMode::TRANSMIT);
   CTWController::GetInstance().Transmit(PN532_PREAMBLE);
   CTWController::GetInstance().Transmit(PN532_STARTCODE1);
   CTWController::GetInstance().Transmit(PN532_STARTCODE2);
   /* send the length, direction, and command bytes */
   /* the sent length is the sum of the direction byte, the command byte, and the argument bytes */
   uint8_t unDataLength = 2u + un_arguments_length;
   CTWController::GetInstance().Transmit(unDataLength);
   CTWController::GetInstance().Transmit(~unDataLength + 1);
   CTWController::GetInstance().Transmit(PN532_HOSTTOPN532);
   CTWController::GetInstance().Transmit(static_cast<uint8_t>(e_command));
   /* initialize the checksum */
   uint8_t unChecksum = PN532_HOSTTOPN532 + static_cast<uint8_t>(e_command);
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
   m_unWatchdogTimer = CClock::GetInstance().GetMilliseconds();
}

