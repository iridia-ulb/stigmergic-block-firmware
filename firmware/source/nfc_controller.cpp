
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

uint8_t CNFCController::m_punTxRxBuffer[] = {0};
uint8_t CNFCController::m_unTxRxLength = 0;

const uint8_t CNFCController::m_punConfigureSAMArguments[] = {
   0x01, /* normal mode */
   20u,  /* timeout: 50ms * 20 = 1s */
   0u,   /* do not use IRQ pin */
};

const uint8_t CNFCController::m_punTgInitAsTargetArguments[] = {
   /* 14443-4A Card only */
   0x00,
   /* SENS_RES */
   0x04, 0x00,
   /* NFCID1 */
   0x12, 0x34, 0x56,
   /* SEL_RES - DEP only mode */
   0x40,
   /* Parameters to build POL_RES (18 bytes including system code) */
   0x01, 0xFE, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xFF, 0xFF,
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
   /* 201 Kbps */
   0x02, 0x01, 0x00, 0xFF, 0xFF, 0x00, 0x00
};

/***********************************************************/
/***********************************************************/

uint8_t CNFCController::STxFunctor::operator()(uint8_t* pun_data, uint8_t un_length) {
//   return 0;
   pun_data[0] = 'h';
   pun_data[1] = 'e';
   pun_data[2] = 'l';
   pun_data[3] = 'l';
   pun_data[4] = 'o';
   return 5;
};

/***********************************************************/
/***********************************************************/

void CNFCController::SRxFunctor::operator()(const uint8_t* pun_data, uint8_t un_length) {
   for(uint8_t i = 0; i < un_length; i++)
      fprintf(CFirmware::GetInstance().m_psHUART, "%c", static_cast<char>(pun_data[i]));
   fprintf(CFirmware::GetInstance().m_psHUART, "\r\n");
   return;
};

/***********************************************************/
/***********************************************************/

CNFCController::CNFCController(STxFunctor& s_target_tx_functor,
                               SRxFunctor& s_target_rx_functor,
                               STxFunctor& s_initiator_tx_functor,
                               SRxFunctor& s_initiator_rx_functor) :
   m_sTargetTxFunctor(s_target_tx_functor),
   m_sTargetRxFunctor(s_target_rx_functor),
   m_sInitiatorTxFunctor(s_initiator_tx_functor),
   m_sInitiatorRxFunctor(s_initiator_rx_functor),
   m_eSelectedCommand(ECommand::GetFirmwareVersion),
   m_eState(EState::Standby) {  
   /* Assumptions: 
      1. I2C is ready and available
      2. the correct face is selected 
   */
   Write(ECommand::GetFirmwareVersion, nullptr, 0);
}

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
      break;   case EState::WaitingForResp:
      if(e_event == EEvent::Interrupt && ReadResp()) {
         switch(m_eSelectedCommand) {
         case ECommand::GetFirmwareVersion:
            if(true) { // TODO check firmware version
               Write(ECommand::ConfigureSAM, m_punConfigureSAMArguments, sizeof m_punConfigureSAMArguments);
            }
            break;
         case ECommand::ConfigureSAM:
            Write(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            break;
         case ECommand::TgInitAsTarget:
            Write(ECommand::TgGetData, nullptr, 0);
            break;
         case ECommand::TgGetData:
            if(m_punTxRxBuffer[0] == 0) {
               m_sTargetRxFunctor(m_punTxRxBuffer + 1, m_unTxRxLength - 1);
               Write(ECommand::TgSetData, m_punTxRxBuffer, m_sTargetTxFunctor(m_punTxRxBuffer, sizeof m_punTxRxBuffer));
            }
            else {
               fprintf(CFirmware::GetInstance().m_psHUART, TEXT_RED "TGD:" TEXT_NORMAL " %02x\r\n", m_punTxRxBuffer[0]);
               /* failure: try return to target mode */
               Write(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
            break;
         case ECommand::TgSetData:
            if(m_punTxRxBuffer[0] == 0) {
               Write(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
            else {
               fprintf(CFirmware::GetInstance().m_psHUART, TEXT_RED "TSD:" TEXT_NORMAL " %02x\r\n", m_punTxRxBuffer[0]);
               /* failure: try return to target mode */
               Write(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
            break;
         case ECommand::InJumpForDEP:
            if(m_punTxRxBuffer[0] == 0) {
               /* set the logical number of the target */
               m_punTxRxBuffer[0] = 1u;
               Write(ECommand::InDataExchange, m_punTxRxBuffer, 
                     m_sInitiatorTxFunctor(m_punTxRxBuffer + 1, (sizeof m_punTxRxBuffer) - 1));
            }
            else {
               fprintf(CFirmware::GetInstance().m_psHUART, TEXT_RED "DEP:" TEXT_NORMAL " %02x\r\n", m_punTxRxBuffer[0]);
               /* failure: try return to target mode */
               Write(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
            break;
         case ECommand::InDataExchange:
            if(m_punTxRxBuffer[0] == 0) {
               m_sInitiatorRxFunctor(m_punTxRxBuffer + 1, m_unTxRxLength - 1);
               /* return to target mode */
               Write(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
            else {
               fprintf(CFirmware::GetInstance().m_psHUART, TEXT_RED "IDX:" TEXT_NORMAL " %02x\r\n", m_punTxRxBuffer[0]);
               /* failure: try return to target mode */
               Write(ECommand::TgInitAsTarget, m_punTgInitAsTargetArguments, sizeof m_punTgInitAsTargetArguments);
            }
            break;
         default:
            m_eState = EState::Failed;
            break;
         }
      }
      else if(e_event == EEvent::Transceive &&
              m_eSelectedCommand == ECommand::TgInitAsTarget) {
         CClock::GetInstance().Delay(10);
         Write(ECommand::InJumpForDEP, m_punInJumpForDEPArguments, sizeof m_punInJumpForDEPArguments);
      }
      break;
   case EState::Failed:
      fprintf(CFirmware::GetInstance().m_psHUART, "Failure!\r\n");
      /* try to reset */
      Write(ECommand::GetFirmwareVersion, nullptr, 0);
      break;
   }
   return true;
}

/***********************************************************/
/***********************************************************/

bool CNFCController::ReadAck() {
   CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS, CTWController::EMode::RECEIVE);
   /*
   bool bAcknowledged =
      (CTWController::GetInstance().Receive() == PN532_I2C_READY) &&
      (CTWController::GetInstance().Receive() == PN532_PREAMBLE) &&
      (CTWController::GetInstance().Receive() == PN532_STARTCODE1) &&
      (CTWController::GetInstance().Receive() == PN532_STARTCODE2) &&
      (CTWController::GetInstance().Receive() == PN532_ACKFRAME1) &&
      (CTWController::GetInstance().Receive() == PN532_ACKFRAME2) &&
      (CTWController::GetInstance().Receive() == PN532_POSTAMBLE);
   */

   uint8_t arr[7];
   for(int i = 0; i < 7; i++) {
      //fprintf(CFirmware::GetInstance().m_psHUART, "<");
      arr[i] = CTWController::GetInstance().Receive();
      //fprintf(CFirmware::GetInstance().m_psHUART, ">");
      //fprintf(CFirmware::GetInstance().m_psHUART, "%02x ", arr[i]);
   }
   
   CTWController::GetInstance().Receive(false);
   CTWController::GetInstance().Stop();

   //fprintf(CFirmware::GetInstance().m_psHUART, "\r\n");

   bool bAcknowledged =
      (arr[0] == PN532_I2C_READY) &&
      (arr[1] == PN532_PREAMBLE) &&
      (arr[2] == PN532_STARTCODE1) &&
      (arr[3] == PN532_STARTCODE2) &&
      (arr[4] == PN532_ACKFRAME1) &&
      (arr[5] == PN532_ACKFRAME2) &&
      (arr[6] == PN532_POSTAMBLE);

   return bAcknowledged;
}

/***********************************************************/
/***********************************************************/

bool CNFCController::ReadResp() {
   CTWController::GetInstance().StartWait(PN532_I2C_ADDRESS,
                                          CTWController::EMode::RECEIVE);
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
   /* pull in the postamble or last byte */
   
   // call functor? only in specific cases
   // return number of bytes?
   return true;
}

/***********************************************************/
/***********************************************************/

void CNFCController::Write(ECommand e_command, const uint8_t* pun_arguments, uint8_t un_arguments_length) {
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
   /* send pun_data */
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
}

