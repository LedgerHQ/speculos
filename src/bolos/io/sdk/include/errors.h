
/*******************************************************************************
 *   Ledger Nano S - Secure firmware
 *   (c) 2022 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#if !defined(ERRORS_H)
#define ERRORS_H

/**
 * Applications-reserved error codes ranges.
 * The Operating System do not use any error code within these ranges.
 */
#define ERR_APP_RANGE_01 0xB000
#define ERR_APP_RANGE_02 0xC000
#define ERR_APP_RANGE_03 0xD000
#define ERR_APP_RANGE_04 0xE000

/**
 * I/O error codes range.
 *
 */
#define ERR_IOL_RANGE 0x1000

// Generic subcategories.
#define ERR_GEN_SUB_01 0x0100
#define ERR_GEN_SUB_02 0x0200
#define ERR_GEN_SUB_03 0x0300
#define ERR_GEN_SUB_04 0x0400
#define ERR_GEN_SUB_05 0x0500
#define ERR_GEN_SUB_06 0x0600
#define ERR_GEN_SUB_07 0x0700
#define ERR_GEN_SUB_08 0x0800
#define ERR_GEN_SUB_09 0x0900
#define ERR_GEN_SUB_0D 0x0D00
#define ERR_GEN_SUB_0E 0x0E00

// Generic identifiers.
enum sdk_generic_identifiers {
  ERR_GEN_ID_01 = 0x01,
  ERR_GEN_ID_02,
  ERR_GEN_ID_03,
  ERR_GEN_ID_04,
  ERR_GEN_ID_05,
  ERR_GEN_ID_06,
  ERR_GEN_ID_07,
  ERR_GEN_ID_08,
  ERR_GEN_ID_09,
  ERR_GEN_ID_0A,
  ERR_GEN_ID_0B,
  ERR_GEN_ID_0C,
  ERR_GEN_ID_0D,
  ERR_GEN_ID_0E,
  ERR_GEN_ID_0F
};

/**
 * I/O-related issues are categorized into:
 * Overflow issues (OFW),
 * Header issues (HDR),
 * State issues (STA),
 * Reset issues (RST),
 * BLE issues (BLE).
 */
#define ERR_IOL_OFW (ERR_IOL_RANGE + ERR_GEN_SUB_01)
#define ERR_IOL_HDR (ERR_IOL_RANGE + ERR_GEN_SUB_02)
#define ERR_IOL_STA (ERR_IOL_RANGE + ERR_GEN_SUB_03)
#define ERR_IOL_RST (ERR_IOL_RANGE + ERR_GEN_SUB_04)

#if defined(HAVE_BLE)
#define ERR_IOL_BLE (ERR_IOL_RANGE + ERR_GEN_SUB_05)
#endif // HAVE_BLE

#define SWO_IOL_OFW_01 (ERR_IOL_OFW + ERR_GEN_ID_01) // 0x1101
#define SWO_IOL_OFW_02 (ERR_IOL_OFW + ERR_GEN_ID_02) // 0x1102
#define SWO_IOL_OFW_03 (ERR_IOL_OFW + ERR_GEN_ID_03) // 0x1103
#define SWO_IOL_OFW_04 (ERR_IOL_OFW + ERR_GEN_ID_04) // 0x1104
#define SWO_IOL_OFW_05 (ERR_IOL_OFW + ERR_GEN_ID_05) // 0x1105

#define SWO_IOL_HDR_01 (ERR_IOL_HDR + ERR_GEN_ID_01) // 0x1201
#define SWO_IOL_HDR_02 (ERR_IOL_HDR + ERR_GEN_ID_02) // 0x1202
#define SWO_IOL_HDR_03 (ERR_IOL_HDR + ERR_GEN_ID_03) // 0x1203
#define SWO_IOL_HDR_04 (ERR_IOL_HDR + ERR_GEN_ID_04) // 0x1204
#define SWO_IOL_HDR_05 (ERR_IOL_HDR + ERR_GEN_ID_05) // 0x1205
#define SWO_IOL_HDR_06 (ERR_IOL_HDR + ERR_GEN_ID_06) // 0x1206
#define SWO_IOL_HDR_07 (ERR_IOL_HDR + ERR_GEN_ID_07) // 0x1207
#define SWO_IOL_HDR_08 (ERR_IOL_HDR + ERR_GEN_ID_08) // 0x1208

#define SWO_IOL_STA_01 (ERR_IOL_STA + ERR_GEN_ID_01) // 0x1301
#define SWO_IOL_STA_02 (ERR_IOL_STA + ERR_GEN_ID_02) // 0x1302
#define SWO_IOL_STA_03 (ERR_IOL_STA + ERR_GEN_ID_03) // 0x1303
#define SWO_IOL_STA_04 (ERR_IOL_STA + ERR_GEN_ID_04) // 0x1304

#define SWO_IOL_RST_01 (ERR_IOL_RST + ERR_GEN_ID_01) // 0x1401
#define SWO_IOL_RST_02 (ERR_IOL_RST + ERR_GEN_ID_02) // 0x1402
#define SWO_IOL_RST_03 (ERR_IOL_RST + ERR_GEN_ID_03) // 0x1403
#define SWO_IOL_RST_05                                                         \
  (ERR_IOL_RST + ERR_GEN_ID_05) // 0x1405 - Do not move this value.

#if defined(HAVE_BLE)
#define SWO_IOL_BLE_01 (ERR_IOL_BLE + ERR_GEN_ID_01) // 0x1501
#define SWO_IOL_BLE_02 (ERR_IOL_BLE + ERR_GEN_ID_02) // 0x1502
#define SWO_IOL_BLE_03 (ERR_IOL_BLE + ERR_GEN_ID_03) // 0x1503
#define SWO_IOL_BLE_04 (ERR_IOL_BLE + ERR_GEN_ID_04) // 0x1504
#define SWO_IOL_BLE_05 (ERR_IOL_BLE + ERR_GEN_ID_05) // 0x1505
#define SWO_IOL_BLE_06 (ERR_IOL_BLE + ERR_GEN_ID_06) // 0x1506
#define SWO_IOL_BLE_07 (ERR_IOL_BLE + ERR_GEN_ID_07) // 0x1507
#define SWO_IOL_BLE_08 (ERR_IOL_BLE + ERR_GEN_ID_08) // 0x1508
#define SWO_IOL_BLE_09 (ERR_IOL_BLE + ERR_GEN_ID_09) // 0x1509
#define SWO_IOL_BLE_0A (ERR_IOL_BLE + ERR_GEN_ID_0A) // 0x150A
#define SWO_IOL_BLE_0B (ERR_IOL_BLE + ERR_GEN_ID_0B) // 0x150B
#define SWO_IOL_BLE_0C (ERR_IOL_BLE + ERR_GEN_ID_0C) // 0x150C
#endif                                               // HAVE_BLE

#define SWO_SEC_PIN_15 0x5515

/**
 * The process is successful.
 */
#define SWO_SUCCESS 0x9000

// Legacy
#define EXCEPTION             0x1  // keep original value // SWO_MUI_UNK_01
#define INVALID_PARAMETER     0x2  // keep original value // SWO_CRY_LEN_01
#define EXCEPTION_SECURITY    0x3  // keep original value // SWO_MUI_UNK_02
#define INVALID_STATE         0x4  // keep original value // SWO_CRY_VAL_01
#define EXCEPTION_IO_RESET    0x5  // keep original value // SWO_IOL_RST_01
#define NOT_ENOUGH_SPACE      0x6  // keep original value // SWO_CRY_VAL_01
#define EXCEPTION_OVERFLOW    0x7  // keep original value // SWO_MUI_UNK_03
#define INVALID_CRC           0x8  // keep original value // SWO_MUI_UNK_0C
#define INVALID_CHECKSUM      0x9  // keep original value // SWO_MUI_UNK_0D
#define INVALID_COUNTER       0xA  // keep original value // SWO_MUI_UNK_0E
#define NOT_SUPPORTED         0xB  // keep original value // SWO_MUI_UNK_0F
#define TIMEOUT               0xC  // keep original value // SWO_MUI_UNK_10
#define EXCEPTION_PIC         0xD  // keep original value // SWO_MUI_UNK_11
#define EXCEPTION_APPEXIT     0xE  // keep original value // SWO_MUI_UNK_12
#define EXCEPTION_IO_OVERFLOW 0xF  // keep original value // SWO_MUI_UNK_13
#define EXCEPTION_IO_HEADER   0x10 // keep original value // SWO_MUI_UNK_14
#define EXCEPTION_IO_STATE    0x11 // keep original value // SWO_MUI_UNK_15
#define EXCEPTION_CXPORT      0x12 // keep original value // SWO_MUI_UNK_16
#define EXCEPTION_SYSTEM      0x13 // keep original value // SWO_MUI_UNK_17
#define EXCEPTION_CORRUPT     0x14

#endif // ERRORS_H
