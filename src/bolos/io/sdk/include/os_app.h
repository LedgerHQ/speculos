#pragma once

#include <stdint.h>

// Arbitrary max size for application name and version.
#define BOLOS_APPNAME_MAX_SIZE_B    32
#define BOLOS_APPVERSION_MAX_SIZE_B 16

/* ----------------------------------------------------------------------- */
/* -                       APPLICATION FUNCTIONS                         - */
/* ----------------------------------------------------------------------- */

typedef void (*appmain_t)(void);

#define BOLOS_TAG_APPNAME    0x01
#define BOLOS_TAG_APPVERSION 0x02
#define BOLOS_TAG_ICON       0x03
#define BOLOS_TAG_DERIVEPATH 0x04
#define BOLOS_TAG_DATA_SIZE                                                    \
  0x05 // meta tag to retrieve the size of the data section for the application
// Library Dependencies are a tuple of 2 LV, the lib appname and the lib version
// (only exact for now). When lib version is not specified, it is not check,
// only name is asserted The DEPENDENCY tag may have several occurrences, one
// for each dependency (by name). Malformed (multiple dep to the same lib with
// different version is is not ORed but ANDed, and then considered bogus)
#define BOLOS_TAG_DEPENDENCY 0x06
#define BOLOS_TAG_RFU        0x07
// first autorised tag value for user data
#define BOLOS_TAG_USER_TAG 0x20

// application slot description
typedef struct application_s {
  // special flags for this application
  uint64_t flags;

  // nvram start address for this application (to check overlap when loading,
  // and mpu lock)
  unsigned char *nvram_begin;
  // nvram stop address (exclusive) for this application (to check overlap when
  // loading, and mpu lock)
  unsigned char *nvram_end;

  // address of the main address, must be set according to BLX spec ( ORed with
  // 1 when jumping into Thumb code
  appmain_t main;

  // Memory organization: [ code (RX) |alignpage| data (RW) |alignpage| install
  // params (R) ]

  // length of the code section of the application (RX)
  unsigned int code_length;

  // NOTE: code_length+params_length must be a multiple of PAGE_SIZE
  // Length of the DATA section of the application. (RW)
  unsigned int data_length;

  // NOTE: code_length+params_length must be a multiple of PAGE_SIZE
  // length of the parameters sections of the application (R)
  unsigned int params_length;

  // Intermediate hash of the application's loaded code and data segments
  unsigned char sha256_code_data[32];
  // Hash of the application's loaded code, data and instantiation parameters
  unsigned char sha256_full[32];

} application_t;
