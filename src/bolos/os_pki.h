#ifndef OS_PKI_H
#define OS_PKI_H

#include "cxlib.h"
#include <stddef.h>
#include <stdint.h>

/** Certificate field with a variable length */
#define CERTIFICATE_FIELD_VAR_LEN (0xFF)
/** Certificate field with a non predefined value */
#define CERTIFICATE_FIELD_UNKNOWN_VALUE (0xFFFFFFFF)
/** Certificate validity index minimum value */
#define CERTIFICATE_VALIDITY_INDEX (0x00000001)
/** Certificate structure type */
#define CERTIFICATE_STRUCTURE_TYPE_CERTIFICATE (0x01)
/** Maximum certificate trusted name length */
#define CERTIFICATE_TRUSTED_NAME_MAXLEN (32)

// Fake OS version
#define VERSION "1.0.0"

/** Certificate tags associated to each certificate field */
// clang-format off
typedef enum {
  CERTIFICATE_TAG_STRUCTURE_TYPE = 0x01,        ///< Structure type
  CERTIFICATE_TAG_VERSION = 0x02,               ///< Certificate version
  CERTIFICATE_TAG_VALIDITY = 0x10,              ///< Certificate validity
  CERTIFICATE_TAG_VALIDITY_INDEX = 0x11,        ///< Certificate validity index
  CERTIFICATE_TAG_CHALLENGE = 0x12,             ///< Challenge value
  CERTIFICATE_TAG_SIGNER_KEY_ID = 0x13,         ///< Signer key ID
  CERTIFICATE_TAG_SIGN_ALGO_ID = 0x14,          ///< Signature algorithm with the signer key
  CERTIFICATE_TAG_SIGNATURE = 0x15,             ///< Signature
  CERTIFICATE_TAG_TIME_VALIDITY = 0x16,         ///< Time validity
  CERTIFICATE_TAG_TRUSTED_NAME = 0x20,          ///< Trusted name
  CERTIFICATE_TAG_PUBLIC_KEY_ID = 0x30,         ///< Public key ID
  CERTIFICATE_TAG_PUBLIC_KEY_USAGE = 0x31,      ///< Public key usage
  CERTIFICATE_TAG_PUBLIC_KEY_CURVE_ID = 0x32,   ///< Curve ID on which the public key is defined
  CERTIFICATE_TAG_COMPRESSED_PUBLIC_KEY = 0x33, ///< Public key in compressed form
  CERTIFICATE_TAG_PK_SIGN_ALGO_ID = 0x34,       ///< Signature algorithm with the public key
  CERTIFICATE_TAG_TARGET_DEVICE = 0x35,         ///< Target device
  CERTIFICATE_TAG_DEPTH = 0x36                  ///< Certificate depth
} os_pki_tag_t;
// clang-format on

/** Certificate version possible values */
enum {
  CERTIFICATE_VERSION_02 = 0x02, ///< Certificate version 2
  CERTIFICATE_VERSION_UNKNOWN
};

/** Certificate key ID possible values */
enum {
  CERTIFICATE_KEY_ID_TEST = 0x0000,
  CERTIFICATE_KEY_ID_PERSOV2,
  CERTIFICATE_KEY_ID_LEDGER_ROOT_V3,
  CERTIFICATE_KEY_ID_PLUGIN_SELECTOR,
  CERTIFICATE_KEY_ID_NFT_METADATA,
  CERTIFICATE_KEY_ID_PARTNER_METADATA,
  CERTIFICATE_KEY_ID_ERC20_METADATA,
  CERTIFICATE_KEY_ID_DOMAIN_METADATA,
  CERTIFICATE_KEY_ID_UNKNOWN
};

/** Signature algorithm possible values */
enum {
  CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA256 = 0x01,
  CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA3_256 = 0x02,
  CERTIFICATE_SIGN_ALGO_ID_ECDSA_KECCAK_256 = 0x03,
  CERTIFICATE_SIGN_ALGO_ID_ECDSA_RIPEMD160 = 0x04,
  CERTIFICATE_SIGN_ALGO_ID_EDDSA_SHA512 = 0x10,
  CERTIFICATE_SIGN_ALGO_ID_UNKNOWN
};

/** Public key usages possible values */
enum {
  CERTIFICATE_PUBLIC_KEY_USAGE_GENUINE_CHECK = 0x01,
  CERTIFICATE_PUBLIC_KEY_USAGE_EXCHANGE_PAYLOAD,
  CERTIFICATE_PUBLIC_KEY_USAGE_NFT_METADATA,
  CERTIFICATE_PUBLIC_KEY_USAGE_TRUSTED_NAME,
  CERTIFICATE_PUBLIC_KEY_USAGE_BACKUP_PROVIDER,
  CERTIFICATE_PUBLIC_KEY_USAGE_RECOVER_ORCHESTRATOR,
  CERTIFICATE_PUBLIC_KEY_USAGE_PLUGIN_METADATA,
  CERTIFICATE_PUBLIC_KEY_USAGE_COIN_META,
  CERTIFICATE_PUBLIC_KEY_USAGE_SEED_ID_AUTH,
  CERTIFICATE_PUBLIC_KEY_USAGE_UNKNOWN,
};

/** Target device possible values */
enum {
  CERTIFICATE_TARGET_DEVICE_NANOS = 0x01,
  CERTIFICATE_TARGET_DEVICE_NANOX,
  CERTIFICATE_TARGET_DEVICE_NANOSP,
  CERTIFICATE_TARGET_DEVICE_STAX,
  CERTIFICATE_TARGET_DEVICE_FLEX,
  CERTIFICATE_TARGET_DEVICE_UNKNOWN
};

/** Structure to store field length and field maximum value */
typedef struct {
  uint32_t value;
  uint8_t field_len;
} os_pki_certificate_tag_info_t;

// clang-format off
/** Array of field length and field maximum value corresponding to each tag */
static const os_pki_certificate_tag_info_t C_os_pki_certificate_tag_info[] = {
    [CERTIFICATE_TAG_STRUCTURE_TYPE]        = {CERTIFICATE_STRUCTURE_TYPE_CERTIFICATE, 0x01                     },
    [CERTIFICATE_TAG_VERSION]               = {CERTIFICATE_VERSION_UNKNOWN,            0x01                     },
    [CERTIFICATE_TAG_VALIDITY]              = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        0x04                     },
    [CERTIFICATE_TAG_VALIDITY_INDEX]        = {CERTIFICATE_VALIDITY_INDEX,             0x04                     },
    [CERTIFICATE_TAG_CHALLENGE]             = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_TAG_SIGNER_KEY_ID]         = {CERTIFICATE_KEY_ID_UNKNOWN,             0x02                     },
    [CERTIFICATE_TAG_SIGN_ALGO_ID]          = {CERTIFICATE_SIGN_ALGO_ID_UNKNOWN,       0x01                     },
    [CERTIFICATE_TAG_TIME_VALIDITY]         = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        0x04                     },
    [CERTIFICATE_TAG_TRUSTED_NAME]          = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_TAG_PUBLIC_KEY_ID]         = {CERTIFICATE_KEY_ID_UNKNOWN,             0x02                     },
    [CERTIFICATE_TAG_PUBLIC_KEY_USAGE]      = {CERTIFICATE_PUBLIC_KEY_USAGE_UNKNOWN,   0x01                     },
    [CERTIFICATE_TAG_PUBLIC_KEY_CURVE_ID]   = {CX_CURVE_TWISTED_EDWARDS_END,           0x01                     },
    [CERTIFICATE_TAG_COMPRESSED_PUBLIC_KEY] = {CERTIFICATE_KEY_ID_UNKNOWN,             CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_TAG_PK_SIGN_ALGO_ID]       = {CERTIFICATE_SIGN_ALGO_ID_UNKNOWN,       0x01                     },
    [CERTIFICATE_TAG_TARGET_DEVICE]         = {CERTIFICATE_TARGET_DEVICE_UNKNOWN,      0x01                     },
    [CERTIFICATE_TAG_SIGNATURE]             = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        CERTIFICATE_FIELD_VAR_LEN},
    [CERTIFICATE_TAG_DEPTH]                 = {CERTIFICATE_FIELD_UNKNOWN_VALUE,        0x01                     },
};
// clang-format on

static const cx_md_t C_os_sign_algo_hash_info[] = {
  [CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA256] = CX_SHA256,
  [CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA3_256] = CX_SHA3_256,
  [CERTIFICATE_SIGN_ALGO_ID_ECDSA_KECCAK_256] = CX_KECCAK,
  [CERTIFICATE_SIGN_ALGO_ID_ECDSA_RIPEMD160] = CX_RIPEMD160,
  [CERTIFICATE_SIGN_ALGO_ID_EDDSA_SHA512] = CX_SHA512
};

uint32_t sys_os_pki_load_certificate(uint8_t expected_key_usage,
                                     uint8_t *certificate,
                                     size_t certificate_len,
                                     uint8_t *trusted_name,
                                     size_t *trusted_name_len,
                                     cx_ecfp_384_public_key_t *public_key);

bool sys_os_pki_verify(uint8_t *descriptor_hash, size_t descriptor_hash_len,
                       uint8_t *signature, size_t signature_len);

uint32_t sys_os_pki_get_info(uint8_t *key_usage, uint8_t *trusted_name,
                             size_t *trusted_name_len,
                             cx_ecfp_384_public_key_t *public_key);

#endif /* OS_PKI_H */
