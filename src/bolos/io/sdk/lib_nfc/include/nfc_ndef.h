#ifndef NFC_H
#define NFC_H

#include "errors.h"
#include "stdint.h"

/*********************
 *      DEFINES
 *********************/
// From ST lib_NDEF.h
#define URI_ID_0x00 0x00
#define URI_ID_0x01 0x01
#define URI_ID_0x02 0x02
#define URI_ID_0x03 0x03
#define URI_ID_0x04 0x04
#define URI_ID_0x05 0x05
#define URI_ID_0x06 0x06
#define URI_ID_0x07 0x07
#define URI_ID_0x08 0x08
#define URI_ID_0x09 0x09
#define URI_ID_0x0A 0x0A
#define URI_ID_0x0B 0x0B
#define URI_ID_0x0C 0x0C
#define URI_ID_0x0D 0x0D
#define URI_ID_0x0E 0x0E
#define URI_ID_0x0F 0x0F
#define URI_ID_0x10 0x10
#define URI_ID_0x11 0x11
#define URI_ID_0x12 0x12
#define URI_ID_0x13 0x13
#define URI_ID_0x14 0x14
#define URI_ID_0x15 0x15
#define URI_ID_0x16 0x16
#define URI_ID_0x17 0x17
#define URI_ID_0x18 0x18
#define URI_ID_0x19 0x19
#define URI_ID_0x1A 0x1A
#define URI_ID_0x1B 0x1B
#define URI_ID_0x1C 0x1C
#define URI_ID_0x1D 0x1D
#define URI_ID_0x1E 0x1E
#define URI_ID_0x1F 0x1F
#define URI_ID_0x20 0x20
#define URI_ID_0x21 0x21
#define URI_ID_0x22 0x22
#define URI_ID_0x23 0x23
#define URI_RFU     0x24

#define URI_ID_0x01_STRING "http://www.\0"
#define URI_ID_0x02_STRING "https://www.\0"
#define URI_ID_0x03_STRING "http://\0"
#define URI_ID_0x04_STRING "https://\0"
#define URI_ID_0x05_STRING "tel:\0"
#define URI_ID_0x06_STRING "mailto:\0"
#define URI_ID_0x07_STRING "ftp://anonymous:anonymous@\0"
#define URI_ID_0x08_STRING "ftp://ftp.\0"
#define URI_ID_0x09_STRING "ftps://\0"
#define URI_ID_0x0A_STRING "sftp://\0"
#define URI_ID_0x0B_STRING "smb://\0"
#define URI_ID_0x0C_STRING "nfs://\0"
#define URI_ID_0x0D_STRING "ftp://\0"
#define URI_ID_0x0E_STRING "dav://\0"
#define URI_ID_0x0F_STRING "news:\0"
#define URI_ID_0x10_STRING "telnet://\0"
#define URI_ID_0x11_STRING "imap:\0"
#define URI_ID_0x12_STRING "rtsp://\0"
#define URI_ID_0x13_STRING "urn:\0"
#define URI_ID_0x14_STRING "pop:\0"
#define URI_ID_0x15_STRING "sip:\0"
#define URI_ID_0x16_STRING "sips:\0"
#define URI_ID_0x17_STRING "tftp:\0"
#define URI_ID_0x18_STRING "btspp://\0"
#define URI_ID_0x19_STRING "btl2cap://\0"
#define URI_ID_0x1A_STRING "btgoep://\0"
#define URI_ID_0x1B_STRING "tcpobex://\0"
#define URI_ID_0x1C_STRING "irdaobex://\0"
#define URI_ID_0x1D_STRING "file://\0"
#define URI_ID_0x1E_STRING "urn:epc:id:\0"
#define URI_ID_0x1F_STRING "urn:epc:tag\0"
#define URI_ID_0x20_STRING "urn:epc:pat:\0"
#define URI_ID_0x21_STRING "urn:epc:raw:\0"
#define URI_ID_0x22_STRING "urn:epc:\0"
#define URI_ID_0x23_STRING "urn:nfc:\0"

#define URI_ID_STRING_MAX_LEN 27 // strlen(URI_ID_0x07_STRING)+1

#define NFC_TEXT_MAX_LEN 215
#define NFC_INFO_MAX_LEN 30

#define NFC_NDEF_MAX_SIZE                                                      \
  (URI_ID_STRING_MAX_LEN + NFC_TEXT_MAX_LEN + NFC_INFO_MAX_LEN + 1)

#define NFC_NDEF_TYPE_TEXT 0x01
#define NFC_NDEF_TYPE_URI  0x02

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief Structure to store an NDEF message
 */
typedef struct __attribute__((packed)) ndef_struct_s {
  uint8_t ndef_type; // NDEF message type NFC_NDEF_TYPE_TEXT / NFC_NDEF_TYPE_URI
  uint8_t uri_id;    // URI string id (only applicable for NFC_NDEF_TYPE_URI)
  char text[NFC_TEXT_MAX_LEN + 1]; // String to store NDEF text/uri +1 for \0
  char info[NFC_INFO_MAX_LEN +
            1]; // String to store NDEF uri information +1 for \0
} ndef_struct_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
uint16_t os_get_uri_header(uint8_t uri_id, char *uri_header);
uint8_t os_parse_ndef(uint8_t *in_buffer, ndef_struct_t *parsed);
uint16_t os_ndef_to_string(ndef_struct_t *ndef_message, char *out_string);

#endif
