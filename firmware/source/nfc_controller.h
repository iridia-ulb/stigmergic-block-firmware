
#include <stdint.h>

#ifndef NFC_CONTROLLER_H
#define NFC_CONTROLLER_H

//////////////////////////////////////

#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)


/*
#define PN532_WAKEUP                        (0x55)

#define PN532_SPI_STATREAD                  (0x02)
#define PN532_SPI_DATAWRITE                 (0x01)
#define PN532_SPI_DATAREAD                  (0x03)
#define PN532_SPI_READY                     (0x01)
*/
#define PN532_I2C_ADDRESS                   (0x48 >> 1)
#define PN532_I2C_READBIT                   (0x01)
#define PN532_I2C_BUSY                      (0x00)
#define PN532_I2C_READY                     (0x01)
#define PN532_I2C_READYTIMEOUT              (20)

//#define PN532_MIFARE_ISO14443A              (0x00)

// Mifare Commands
/*
#define MIFARE_CMD_AUTH_A                   (0x60)
#define MIFARE_CMD_AUTH_B                   (0x61)
#define MIFARE_CMD_READ                     (0x30)
#define MIFARE_CMD_WRITE                    (0xA0)
#define MIFARE_CMD_TRANSFER                 (0xB0)
#define MIFARE_CMD_DECREMENT                (0xC0)
#define MIFARE_CMD_INCREMENT                (0xC1)
#define MIFARE_CMD_RESTORE                  (0xC2)
*/

// Prefixes for NDEF Records (to identify record type)
/*
#define NDEF_URIPREFIX_NONE                 (0x00)
#define NDEF_URIPREFIX_HTTP_WWWDOT          (0x01)
#define NDEF_URIPREFIX_HTTPS_WWWDOT         (0x02)
#define NDEF_URIPREFIX_HTTP                 (0x03)
#define NDEF_URIPREFIX_HTTPS                (0x04)
#define NDEF_URIPREFIX_TEL                  (0x05)
#define NDEF_URIPREFIX_MAILTO               (0x06)
#define NDEF_URIPREFIX_FTP_ANONAT           (0x07)
#define NDEF_URIPREFIX_FTP_FTPDOT           (0x08)
#define NDEF_URIPREFIX_FTPS                 (0x09)
#define NDEF_URIPREFIX_SFTP                 (0x0A)
#define NDEF_URIPREFIX_SMB                  (0x0B)
#define NDEF_URIPREFIX_NFS                  (0x0C)
#define NDEF_URIPREFIX_FTP                  (0x0D)
#define NDEF_URIPREFIX_DAV                  (0x0E)
#define NDEF_URIPREFIX_NEWS                 (0x0F)
#define NDEF_URIPREFIX_TELNET               (0x10)
#define NDEF_URIPREFIX_IMAP                 (0x11)
#define NDEF_URIPREFIX_RTSP                 (0x12)
#define NDEF_URIPREFIX_URN                  (0x13)
#define NDEF_URIPREFIX_POP                  (0x14)
#define NDEF_URIPREFIX_SIP                  (0x15)
#define NDEF_URIPREFIX_SIPS                 (0x16)
#define NDEF_URIPREFIX_TFTP                 (0x17)
#define NDEF_URIPREFIX_BTSPP                (0x18)
#define NDEF_URIPREFIX_BTL2CAP              (0x19)
#define NDEF_URIPREFIX_BTGOEP               (0x1A)
#define NDEF_URIPREFIX_TCPOBEX              (0x1B)
#define NDEF_URIPREFIX_IRDAOBEX             (0x1C)
#define NDEF_URIPREFIX_FILE                 (0x1D)
#define NDEF_URIPREFIX_URN_EPC_ID           (0x1E)
#define NDEF_URIPREFIX_URN_EPC_TAG          (0x1F)
#define NDEF_URIPREFIX_URN_EPC_PAT          (0x20)
#define NDEF_URIPREFIX_URN_EPC_RAW          (0x21)
#define NDEF_URIPREFIX_URN_EPC              (0x22)
#define NDEF_URIPREFIX_URN_NFC              (0x23)
*/

/*
#define PN532_GPIO_VALIDATIONBIT            (0x80)
#define PN532_GPIO_P30                      (0)
#define PN532_GPIO_P31                      (1)
#define PN532_GPIO_P32                      (2)
#define PN532_GPIO_P33                      (3)
#define PN532_GPIO_P34                      (4)
#define PN532_GPIO_P35                      (5)
*/

enum class ESAMMode : uint8_t {
   NORMAL       = 0x01,
   VIRTUAL_CARD = 0x02,
   WIRED_CARD   = 0x03,
   DUAL_CARD    = 0x04
};

/*
#define PN532_BRTY_ISO14443A                0x00
#define PN532_BRTY_ISO14443B                0x03
#define PN532_BRTY_212KBPS                  0x01
#define PN532_BRTY_424KBPS                  0x02
#define PN532_BRTY_JEWEL                    0x04
*/

#define NFC_WAIT_TIME                       30
#define NFC_CMD_BUF_LEN                     64

#define NFC_FRAME_DIRECTION_INDEX           5
#define NFC_FRAME_ID_INDEX                  6
#define NFC_FRAME_STATUS_INDEX              7

#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)

/*
typedef enum{
    NFC_STA_TAG,
    NFC_STA_GETDATA,
    NFC_STA_SETDATA,
}poll_sta_type;
*/

class CNFCController {

public:
   enum class ECommand : uint8_t {
      DIAGNOSE              = 0x00,
      GETFIRMWAREVERSION    = 0x02,
      GETGENERALSTATUS      = 0x04,
      READREGISTER          = 0x06,
      WRITEREGISTER         = 0x08,
      READGPIO              = 0x0C,
      WRITEGPIO             = 0x0E,
      SETSERIALBAUDRATE     = 0x10,
      SETPARAMETERS         = 0x12,
      SAMCONFIGURATION      = 0x14,
      POWERDOWN             = 0x16,
      RFCONFIGURATION       = 0x32,
      RFREGULATIONTEST      = 0x58,
      INJUMPFORDEP          = 0x56,
      INJUMPFORPSL          = 0x46,
      INLISTPASSIVETARGET   = 0x4A,
      INATR                 = 0x50,
      INPSL                 = 0x4E,
      INDATAEXCHANGE        = 0x40,
      INCOMMUNICATETHRU     = 0x42,
      INDESELECT            = 0x44,
      INRELEASE             = 0x52,
      INSELECT              = 0x54,
      INAUTOPOLL            = 0x60,
      TGINITASTARGET        = 0x8C,
      TGSETGENERALBYTES     = 0x92,
      TGGETDATA             = 0x86,
      TGSETDATA             = 0x8E,
      TGSETMETADATA         = 0x94,
      TGGETINITIATORCOMMAND = 0x88,
      TGRESPONSETOINITIATOR = 0x90,
      TGGETTARGETSTATUS     = 0x8A
   };


   CNFCController() {}

   bool ConfigureSAM(ESAMMode e_mode = ESAMMode::NORMAL, uint8_t un_timeout = 20, bool b_use_irq = false);

   uint8_t P2PInitiatorInit();

   uint8_t P2PTargetInit();

   uint8_t P2PInitiatorTxRx(uint8_t* pun_tx_buffer, 
                            uint8_t  un_tx_buffer_len, 
                            uint8_t* pun_rx_buffer,
                            uint8_t  un_rx_buffer_len);

   uint8_t P2PTargetTxRx(uint8_t* pun_tx_buffer, 
                         uint8_t  un_tx_buffer_len, 
                         uint8_t* pun_rx_buffer,
                         uint8_t  un_rx_buffer_len);

   bool SetParameters(uint8_t param);

   bool Probe();

   bool PowerDown();
private:


   //void WriteCommand(ECommand e_cmd, uint8_t* pun_args, uint8_t un_nargs);

   void write_cmd(uint8_t *cmd, uint8_t len);
   uint8_t write_cmd_check_ack(uint8_t *cmd, uint8_t len);
   bool read_dt(uint8_t *buf, uint8_t len);
   uint8_t wait_ready(uint8_t ms=NFC_WAIT_TIME);
   bool read_ack(void);

   void puthex(uint8_t data);
   void puthex(uint8_t *buf, uint32_t len);

   /* data buffer for reading / writing commands */
   uint8_t m_punIOBuffer[NFC_CMD_BUF_LEN];

};

#endif /** __NFC_H */
