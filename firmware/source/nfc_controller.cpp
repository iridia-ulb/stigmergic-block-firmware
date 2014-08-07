#include <firmware.h>

#include "nfc_controller.h"

uint8_t ack[6]={
    0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00
};
uint8_t nfc_version[6]={
    0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03
};

/***********************************************************/
/***********************************************************/

bool CNFCController::PowerDown() {
   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::POWERDOWN);
   m_punIOBuffer[1] = 0x88; // Wake up on RF field & I2C
   m_punIOBuffer[2] = 0x01; // Generate an IRQ on wake up
   /* write command and check ack frame */
   if(!write_cmd_check_ack(m_punIOBuffer, 3)) {
#ifdef DEBUG
      fprintf(Firmware::GetInstance().m_psTUART, 
              "PN532 didn't send acknowledgement frame\r\n");
#endif
      return false;
   }

   /* read the rest of the reply */
   read_dt(m_punIOBuffer, 8);
   /* verify that the recieved data was a reply frame to given command */
   if(m_punIOBuffer[NFC_FRAME_DIRECTION_INDEX] != PN532_PN532TOHOST ||
      m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::POWERDOWN)) {
#ifdef DEBUG
      fprintf(Firmware::GetInstance().m_psTUART, 
              "Reply frame format error\r\n");
#endif
      return false;
   }
   /* check the lower 6 bits of the status byte, error code 0x00 means success */
   if((m_punIOBuffer[NFC_FRAME_STATUS_INDEX] & 0x3F) != 0x00) {
#ifdef DEBUG
      fprintf(Firmware::GetInstance().m_psTUART, 
              "Status byte contained error: 0x%02x\r\n",
              m_punIOBuffer[NFC_FRAME_STATUS_INDEX] & 0x3F);
#endif
      return false;
   }
   return true;
}

/***********************************************************/
/***********************************************************/

bool CNFCController::Probe() {
   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::GETFIRMWAREVERSION);
   /* write command and check ack frame */
   if(!write_cmd_check_ack(m_punIOBuffer, 1)) {
      return false;
   }
   /* read the rest of the reply */
   read_dt(m_punIOBuffer, 12);
   /* verify that the recieved data was a reply frame to given command */
   if(m_punIOBuffer[NFC_FRAME_DIRECTION_INDEX] != PN532_PN532TOHOST ||
      m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::GETFIRMWAREVERSION)) {
      return false;
   }
   /* check the firmware version */
   return (strncmp((char *)m_punIOBuffer, (char *)nfc_version, 6) == 0);
}

/*****************************************************************************/
/*!
	@brief  Configures the SAM (Secure Access Module)
	@param  mode - set mode, normal mode default
	@param  timeout - Details in NXP's PN532UM.pdf
	@param  irq - 0 unused (default), 1 used
	@return 0 - failed
            1 - successfully
*/
/*****************************************************************************/
bool CNFCController::ConfigureSAM(ESAMMode e_mode, uint8_t un_timeout, bool b_use_irq) {
    m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::SAMCONFIGURATION);
    m_punIOBuffer[1] = static_cast<uint8_t>(e_mode);
    m_punIOBuffer[2] = un_timeout; // timeout = 50ms * un_timeout
    m_punIOBuffer[3] = b_use_irq?1u:0u; // use IRQ pin
    /* write command */
    if(!write_cmd_check_ack(m_punIOBuffer, 4)){
       return false;
    }
    /* read response */
    read_dt(m_punIOBuffer, 8);
    /* return whether the reply was as expected */
    return  (m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 == static_cast<uint8_t>(ECommand::SAMCONFIGURATION));
}


/*****************************************************************************/
/*!
	@brief  Configure PN532 as Initiator
	@param  NONE
	@return 0 - failed
            1 - successfully
*/
/*****************************************************************************/
bool CNFCController::P2PInitiatorInit() {
    m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::INJUMPFORDEP);
    m_punIOBuffer[1] = 0x01; // avtive mode
    m_punIOBuffer[2] = 0x02; // 201Kbps
    m_punIOBuffer[3] = 0x01;

    m_punIOBuffer[4] = 0x00;
    m_punIOBuffer[5] = 0xFF;
    m_punIOBuffer[6] = 0xFF;
    m_punIOBuffer[7] = 0x00;
    m_punIOBuffer[8] = 0x00;

    if(!write_cmd_check_ack(m_punIOBuffer, 9)) {
#ifdef DEBUG
       fprintf(Firmware::GetInstance().m_psTUART, "InJumpForDEP send failed\n");
#endif
       return false;
    }

#ifdef DEBUG
    fprintf(Firmware::GetInstance().m_psTUART, "InJumpForDEP sent\n");
#endif

    read_dt(m_punIOBuffer, 25);

    if(m_punIOBuffer[5] != PN532_PN532TOHOST) {
       //        Serial.println("InJumpForDEP sent read failed");
       return false;
    }

    if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::INJUMPFORDEP)) {
#ifdef DEBUG
       puthex(m_punIOBuffer, m_punIOBuffer[3] + 7);
       fprintf(Firmware::GetInstance().m_psTUART, "Initiator init failed");
#endif
       return false;
    }
    if(m_punIOBuffer[NFC_FRAME_ID_INDEX + 1]) {
       return false;
    }

#ifdef DEBUG
    fprintf(Firmware::GetInstance().m_psTUART, "InJumpForDEP read success");
#endif
    return true;
}

/*****************************************************************************/
/*!
	@brief  Configure PN532 as Target.
	@param  NONE.
	@return 0 - failed
            1 - successfully
*/
/*****************************************************************************/
bool CNFCController::P2PTargetInit() {
    m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::TGINITASTARGET);
    /** 14443-4A Card only */
    m_punIOBuffer[1] = 0x00;
    /** SENS_RES */
    m_punIOBuffer[2] = 0x04;
    m_punIOBuffer[3] = 0x00;
    /** NFCID1 */
    m_punIOBuffer[4] = 0x12;
    m_punIOBuffer[5] = 0x34;
    m_punIOBuffer[6] = 0x56;
    /** SEL_RES - DEP only mode */
    m_punIOBuffer[7] = 0x40; 
    /**Parameters to build POL_RES (18 bytes including system code) */
    m_punIOBuffer[8] = 0x01;
    m_punIOBuffer[9] = 0xFE;
    m_punIOBuffer[10] = 0xA2;
    m_punIOBuffer[11] = 0xA3;
    m_punIOBuffer[12] = 0xA4;
    m_punIOBuffer[13] = 0xA5;
    m_punIOBuffer[14] = 0xA6;
    m_punIOBuffer[15] = 0xA7;
    m_punIOBuffer[16] = 0xC0;
    m_punIOBuffer[17] = 0xC1;
    m_punIOBuffer[18] = 0xC2;
    m_punIOBuffer[19] = 0xC3;
    m_punIOBuffer[20] = 0xC4;
    m_punIOBuffer[21] = 0xC5;
    m_punIOBuffer[22] = 0xC6;
    m_punIOBuffer[23] = 0xC7;
    m_punIOBuffer[24] = 0xFF;
    m_punIOBuffer[25] = 0xFF;
    /** NFCID3t */
    m_punIOBuffer[26] = 0xAA;
    m_punIOBuffer[27] = 0x99;
    m_punIOBuffer[28] = 0x88;
    m_punIOBuffer[29] = 0x77;
    m_punIOBuffer[30] = 0x66;
    m_punIOBuffer[31] = 0x55;
    m_punIOBuffer[32] = 0x44;
    m_punIOBuffer[33] = 0x33;
    m_punIOBuffer[34] = 0x22;
    m_punIOBuffer[35] = 0x11;
    /** Length of general bytes  */
    m_punIOBuffer[36] = 0x00;
    /** Length of historical bytes  */
    m_punIOBuffer[37] = 0x00;

    if(!write_cmd_check_ack(m_punIOBuffer, 38)) {
       return false;
    }
#ifdef DEBUG
        fprintf(Firmware::GetInstance().m_psTUART, "Target init sent\r\n");
#endif
    read_dt(m_punIOBuffer, 24);

    if(m_punIOBuffer[5] != PN532_PN532TOHOST){
        return false;
    }

    if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::TGINITASTARGET)) {
#ifdef DEBUG
        puthex(m_punIOBuffer, m_punIOBuffer[3] + 7);
        fprintf(Firmware::GetInstance().m_psTUART, "Target init fail\r\n");
#endif
        return false;
    }

#ifdef DEBUG
    fprintf(Firmware::GetInstance().m_psTUART, "TgInitAsTarget read success\r\n");
#endif
    return true;
}

/*****************************************************************************/
/*!
	@brief  Initiator send and reciev data.
    @param  tx_buf --- data send buffer, user sets
            tx_len --- data send legth, user sets.
            rx_buf --- data recieve buffer, returned by P2PInitiatorTxRx
            rx_len --- data receive length, returned by P2PInitiatorTxRx
	@return 0 - send failed
            1 - send successfully
*/
/*****************************************************************************/
uint8_t CNFCController::P2PInitiatorTxRx(uint8_t* pun_tx_buffer,
                                         uint8_t  un_tx_buffer_len,
                                         uint8_t* pun_rx_buffer,
                                         uint8_t  un_rx_buffer_len) {
   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::INDATAEXCHANGE);
   m_punIOBuffer[1] = 0x01; // logical number of the relevant target

   /* transfer the tx data into the IO buffer as the command parameter */
   memcpy(m_punIOBuffer + 2, pun_tx_buffer, un_tx_buffer_len);

   if(!write_cmd_check_ack(m_punIOBuffer, un_tx_buffer_len + 2)){
      return 0;
   }
#ifdef DEBUG
   fprintf(Firmware::GetInstance().m_psTUART, "Initiator DataExchange sent\r\n");
#endif

   read_dt(m_punIOBuffer, 60);
   if(m_punIOBuffer[5] != PN532_PN532TOHOST){
      return 0;
   }

#ifdef DEBUG
   fprintf(Firmware::GetInstance().m_psTUART, "Initiator DataExchange Get\r\n");
#endif

   if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::INDATAEXCHANGE)){
#ifdef DEBUG
      puthex(m_punIOBuffer, m_punIOBuffer[3] + 7);
      fprintf(Firmware::GetInstance().m_psTUART, "Send data failed\r\n");
#endif
      return 0;
   }

   if(m_punIOBuffer[NFC_FRAME_ID_INDEX + 1]) {
#ifdef DEBUG
      fprintf(Firmware::GetInstance().m_psTUART,"InExchangeData Error: ");
      puthex(m_punIOBuffer, m_punIOBuffer[3] + 7);
      fprintf(Firmware::GetInstance().m_psTUART, "\r\n");
#endif
      return 0;
   }

#ifdef DEBUG
   puthex(m_punIOBuffer, m_punIOBuffer[3] + 7);
   fprintf(Firmware::GetInstance().m_psTUART, "\r\n");
#endif
   /* return number of read bytes */
   uint8_t unRxDataLength = m_punIOBuffer[3] - 3;
   memcpy(pun_rx_buffer, m_punIOBuffer + 8, (unRxDataLength > un_rx_buffer_len) ? un_rx_buffer_len : unRxDataLength);
   return (unRxDataLength > un_rx_buffer_len) ? un_rx_buffer_len : unRxDataLength;
}

/*****************************************************************************/
/*!
	@brief  Target sends and recievs data.
    @param  tx_buf --- data send buffer, user sets
            tx_len --- data send legth, user sets.
            rx_buf --- data recieve buffer, returned by P2PInitiatorTxRx
            rx_len --- data receive length, returned by P2PInitiatorTxRx
	@return 0 - send failed
            1 - send successfully
*/
/*****************************************************************************/

uint8_t CNFCController::P2PTargetTxRx(uint8_t* pun_tx_buffer, 
                                      uint8_t  un_tx_buffer_len,
                                      uint8_t* pun_rx_buffer,
                                      uint8_t  un_rx_buffer_len) {

   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::TGGETDATA);
   if(!write_cmd_check_ack(m_punIOBuffer, 1)){
      return 0;
   }
   read_dt(m_punIOBuffer, 60);
   if(m_punIOBuffer[5] != PN532_PN532TOHOST){
      return 0;
   }

   if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::TGGETDATA)) {
#ifdef DEBUG
      puthex(m_punIOBuffer, 20);
      fprintf(Firmware::GetInstance().m_psTUART, "Target GetData failed\r\n");
#endif
      return 0;
   }
   if(m_punIOBuffer[NFC_FRAME_ID_INDEX + 1]) {
#ifdef DEBUG
      fprintf(Firmware::GetInstance().m_psTUART,"TgGetData Error:");
      puthex(m_punIOBuffer, m_punIOBuffer[3]+7);
      fprintf(Firmware::GetInstance().m_psTUART, "\r\n");
#endif
      return 0;
   }

#ifdef DEBUG
   fprintf(Firmware::GetInstance().m_psTUART, "TgGetData: ");
   puthex(m_punIOBuffer, m_punIOBuffer[3]+7);
   fprintf(Firmware::GetInstance().m_psTUART, "\r\n");
#endif

   /* read data */
   uint8_t unRxDataLength = m_punIOBuffer[3] - 3;
   memcpy(pun_rx_buffer, m_punIOBuffer + 8, (unRxDataLength > un_rx_buffer_len) ? un_rx_buffer_len : unRxDataLength);

   /* send reply */
   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::TGSETDATA);
   memcpy(m_punIOBuffer + 1, pun_tx_buffer, un_tx_buffer_len);

   if(!write_cmd_check_ack(m_punIOBuffer, un_tx_buffer_len + 1)) {
      return 0;
   }
   read_dt(m_punIOBuffer, 26);
   if(m_punIOBuffer[5] != PN532_PN532TOHOST) {
      return 0;
   }
   if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::TGSETDATA)) {
#ifdef DEBUG
      puthex(m_punIOBuffer, 20);
      fprintf(Firmware::GetInstance().m_psTUART, "Send data failed\r\n");
#endif
      return 0;
   }
   if(m_punIOBuffer[NFC_FRAME_ID_INDEX + 1]) {
      return 0;
   }
   /* return the number of bytes in the rx buffer (or maximum buffer capacity) */
   return (unRxDataLength > un_rx_buffer_len) ? un_rx_buffer_len : unRxDataLength;
}

/*****************************************************************************/
/*!
	@brief  send frame to PN532 and wait for ack
	@param  cmd - pointer to frame buffer
	@param  len - frame length
	@return 0 - send failed
            1 - send successfully
*/
/*****************************************************************************/


uint8_t CNFCController::write_cmd_check_ack(uint8_t *cmd, uint8_t len) {
    write_cmd(cmd, len);
#ifdef DEBUG
    fprintf(Firmware::GetInstance().m_psTUART,"Checking for ACK frame\r\n");
#endif

    // read acknowledgement
    if (!read_ack()) {
#ifdef DEBUG
       fprintf(Firmware::GetInstance().m_psTUART, "No ACK frame received!\r\n");
#endif
       return false;
    }
    return true; // ack'd command
}


/*****************************************************************************/
/*!
	@brief  Send hexadecimal data through Serial with specified length.
	@param  buf - pointer of data buffer.
	@param  len - length need to send.
	@return NONE
*/
/*****************************************************************************/
void CNFCController::puthex(uint8_t data) {
   fprintf(Firmware::GetInstance().m_psTUART, "0x%02x ", data);
}


void CNFCController::puthex(uint8_t *buf, uint32_t len) {
    for(uint32_t i = 0; i < len; i++)
    {
       fprintf(Firmware::GetInstance().m_psTUART, "0x%02x ", buf[i]);
    }
}

/*****************************************************************************/
/*!
	@brief  Write data frame to PN532.
	@param  cmd - Pointer of the data frame.
	@param  len - length need to write
	@return NONE
*/
/*****************************************************************************/
void CNFCController::write_cmd(uint8_t *cmd, uint8_t len)
{
    uint8_t checksum;

    len++;

#ifdef DEBUG
    fprintf(Firmware::GetInstance().m_psTUART, "Sending: ");
#endif

    Firmware::GetInstance().GetTimer().Delay(2);     // or whatever the delay is for waking up the board

    // I2C START
    Firmware::GetInstance().GetTWController().BeginTransmission(PN532_I2C_ADDRESS);
    checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
    Firmware::GetInstance().GetTWController().Write(PN532_PREAMBLE);
    Firmware::GetInstance().GetTWController().Write(PN532_PREAMBLE);
    Firmware::GetInstance().GetTWController().Write(PN532_STARTCODE2);

    Firmware::GetInstance().GetTWController().Write(len);
    Firmware::GetInstance().GetTWController().Write(~len + 1);

    Firmware::GetInstance().GetTWController().Write(PN532_HOSTTOPN532);
    checksum += PN532_HOSTTOPN532;

#ifdef DEBUG
    puthex(PN532_PREAMBLE);
    puthex(PN532_PREAMBLE);
    puthex(PN532_STARTCODE2);
    puthex((unsigned char)len);
    puthex((unsigned char)(~len + 1));
    puthex(PN532_HOSTTOPN532);
#endif

    for (uint8_t i=0; i<len-1; i++)
    {
        if(Firmware::GetInstance().GetTWController().Write(cmd[i])){
            checksum += cmd[i];
#ifdef DEBUG
            puthex(cmd[i]);
#endif
        } else {
            i--;
            Firmware::GetInstance().GetTimer().Delay(1);
        }
    }

    Firmware::GetInstance().GetTWController().Write(~checksum);
    Firmware::GetInstance().GetTWController().Write(PN532_POSTAMBLE);

    // I2C STOP
    uint8_t err = Firmware::GetInstance().GetTWController().EndTransmission();
  
#ifdef DEBUG
    puthex(~checksum);
    puthex(PN532_POSTAMBLE);
    fprintf(Firmware::GetInstance().m_psTUART, "\r\n");
    fprintf(Firmware::GetInstance().m_psTUART, "EndTran returned %i\r\n", err);
#endif

}

/*****************************************************************************/
/*!
	@brief  Read data frame from PN532.
	@param  buf - pointer of data buffer
	@param  len - length need to read
	@return true if read was successful.
*/
/*****************************************************************************/
bool CNFCController::read_dt(uint8_t *buf, uint8_t len) {
   uint8_t unStatus = PN532_I2C_BUSY;
   // attempt to read response twenty times
   for(uint8_t i = 0; i < 25; i++) {
      Firmware::GetInstance().GetTimer().Delay(10);
      // Start read (n+1 to take into account leading 0x01 with I2C)
      Firmware::GetInstance().GetTWController().Read(PN532_I2C_ADDRESS, len + 2, true);
      // Read the status byte
      unStatus = Firmware::GetInstance().GetTWController().Read();

#ifdef DEBUG
      fprintf(Firmware::GetInstance().m_psTUART,"rdt: status = 0x%02x\r\n", unStatus);
#endif
    
      if(unStatus == PN532_I2C_READY) {
         break;
      }
      else {
         // flush the buffer
         for(uint8_t i=0; i<len; i++) {
            Firmware::GetInstance().GetTWController().Read();
         }
      }
   }

   if(unStatus == PN532_I2C_READY) {

#ifdef DEBUG
      fprintf(Firmware::GetInstance().m_psTUART,"Reading: ");
#endif
      for(uint8_t i=0; i<len; i++) {
         buf[i] = Firmware::GetInstance().GetTWController().Read();
#ifdef DEBUG
         puthex(buf[i]);
#endif
      }
#ifdef DEBUG
      fprintf(Firmware::GetInstance().m_psTUART,"\r\n");
#endif
   }
   // Discard trailing 0x00 0x00
   // receive();
   return (unStatus == PN532_I2C_READY);
}

/*****************************************************************************/
/*!
	@brief  read ack frame from PN532
	@param  NONE
	@return 0 - ack failed
            1 - Ack OK
*/
/*****************************************************************************/
bool CNFCController::read_ack(void)
{
   uint8_t ack_buf[6];

   read_dt(ack_buf, 6);

   //    puthex(ack_buf, 6);
   //    Serial.println();
   return (memcmp(ack_buf, ack, 6) == 0);
}
