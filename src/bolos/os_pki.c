#define _SDK_2_0_
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cx.h"
#include "cx_utils.h"
#include "os_pki.h"
#include "os_signature.h"

#define OS_PKI_DIGEST_MAX_LEN     (64)
#define OS_PKI_TLV_TAG_OFFSET     (0)
#define OS_PKI_TLV_LENGTH_OFFSET  (1)
#define OS_PKI_TLV_VALUE_OFFSET   (2)
#define OS_PKI_SEMVER_VERSION_LEN (6)
#define SWO_OK                    (0x0000)

typedef struct os_pki_s {
  uint8_t key_usage;
  uint8_t signer_sign_algo;
  uint8_t pk_sign_algo;
  uint8_t trusted_name[CERTIFICATE_TRUSTED_NAME_MAXLEN];
  size_t trusted_name_len;
  uint16_t signer_id;
  cx_ecfp_384_public_key_t public_key;
} os_pki_t;

static os_pki_t os_pki;
static uint8_t os_pki_set;

uint32_t os_pki_check_value(uint8_t *certificate_value,
                            cx_ecfp_384_public_key_t *cert_public_key)
{
  size_t domain_len;
  cx_ecpoint_t point;
  uint32_t swo_error = SWO_OK;
  cx_err_t error = CX_OK;
  char cert_validity[OS_PKI_SEMVER_VERSION_LEN] = { 0 };
  uint8_t tag = certificate_value[OS_PKI_TLV_TAG_OFFSET];
  size_t version_nchars = 0;

  switch (tag) {
  case CERTIFICATE_TAG_STRUCTURE_TYPE:
    if ((certificate_value[OS_PKI_TLV_VALUE_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].value) ||
        (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].field_len)) {
      return 0x422F;
    }
    break;
  case CERTIFICATE_TAG_VERSION:
    if ((certificate_value[OS_PKI_TLV_VALUE_OFFSET] >=
         C_os_pki_certificate_tag_info[tag].value) ||
        (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].field_len)) {
      return 0x4230;
    }
    break;
  case CERTIFICATE_TAG_VALIDITY:
    if (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
        C_os_pki_certificate_tag_info[tag].field_len) {
      return 0x4231;
    }
    cert_validity[version_nchars] =
        '0' + certificate_value[OS_PKI_TLV_VALUE_OFFSET];
    cert_validity[++version_nchars] = '.';
    cert_validity[++version_nchars] =
        '0' + certificate_value[OS_PKI_TLV_VALUE_OFFSET + 1];
    cert_validity[++version_nchars] = '.';
    cert_validity[++version_nchars] =
        '0' + certificate_value[OS_PKI_TLV_VALUE_OFFSET + 2];
    // Handle patch version greater than 10
    if (certificate_value[OS_PKI_TLV_VALUE_OFFSET + 2] >= 10) {
      cert_validity[version_nchars] =
          '0' + certificate_value[OS_PKI_TLV_VALUE_OFFSET + 2] / 10;
      cert_validity[++version_nchars] =
          '0' + certificate_value[OS_PKI_TLV_VALUE_OFFSET + 2] % 10;
    }
    if (strncmp(VERSION, cert_validity, version_nchars + 1) > 0) {
      return 0x4231;
    }
    break;
  case CERTIFICATE_TAG_VALIDITY_INDEX:
    if ((U4BE(certificate_value, OS_PKI_TLV_VALUE_OFFSET) <=
         C_os_pki_certificate_tag_info[tag].value) ||
        (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].field_len)) {
      return 0x4232;
    }
    break;
  case CERTIFICATE_TAG_CHALLENGE:
    break;
  case CERTIFICATE_TAG_SIGNER_KEY_ID:
    // Do not restrict Signer key ID value
    // any new key ID added to SDK will be accepted
    if (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
        C_os_pki_certificate_tag_info[tag].field_len) {
      return 0x4233;
    }
    os_pki.signer_id = U2BE(certificate_value, OS_PKI_TLV_VALUE_OFFSET);
    break;
  case CERTIFICATE_TAG_SIGN_ALGO_ID:
    if ((certificate_value[OS_PKI_TLV_VALUE_OFFSET] >=
         C_os_pki_certificate_tag_info[tag].value) ||
        (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].field_len)) {
      return 0x4234;
    }
    os_pki.signer_sign_algo = certificate_value[OS_PKI_TLV_VALUE_OFFSET];
    break;
  case CERTIFICATE_TAG_TIME_VALIDITY:
    if (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
        C_os_pki_certificate_tag_info[tag].field_len) {
      return 0x423B;
    }
    break;
  case CERTIFICATE_TAG_PUBLIC_KEY_ID:
    // Do not restrict public key ID value
    // any new key ID added to SDK will be accepted
    if (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
        C_os_pki_certificate_tag_info[tag].field_len) {
      return 0x4235;
    }
    break;
  case CERTIFICATE_TAG_TRUSTED_NAME:
    if (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] >
        CERTIFICATE_TRUSTED_NAME_MAXLEN) {
      return 0x423A;
    }
    memcpy(os_pki.trusted_name, certificate_value + OS_PKI_TLV_VALUE_OFFSET,
           certificate_value[OS_PKI_TLV_LENGTH_OFFSET]);
    os_pki.trusted_name_len = certificate_value[OS_PKI_TLV_LENGTH_OFFSET];
    break;
  case CERTIFICATE_TAG_PUBLIC_KEY_USAGE:
    // Do not restrict public key usage value
    // any new key usage added to SDK will be accepted
    if (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
        C_os_pki_certificate_tag_info[tag].field_len) {
      return 0x4236;
    }
    os_pki.key_usage = certificate_value[OS_PKI_TLV_VALUE_OFFSET];
    break;
  case CERTIFICATE_TAG_PUBLIC_KEY_CURVE_ID:
    if ((certificate_value[OS_PKI_TLV_VALUE_OFFSET] >=
         C_os_pki_certificate_tag_info[tag].value) ||
        (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].field_len)) {
      return 0x4237;
    }
    cert_public_key->curve = certificate_value[OS_PKI_TLV_VALUE_OFFSET];
    break;
  case CERTIFICATE_TAG_COMPRESSED_PUBLIC_KEY:
    /* Decompress key and initialize os_pki.public_key */
    CX_CHECK(
        sys_cx_ecdomain_parameters_length(cert_public_key->curve, &domain_len));
    CX_CHECK(sys_cx_bn_lock(domain_len, 0));
    CX_CHECK(sys_cx_ecpoint_alloc(&point, cert_public_key->curve));
    /* If the prefix is 02, the y-coordinate is even otherwise if the prefix is
     * 03 the y-coordinate is odd */
    CX_CHECK(sys_cx_ecpoint_decompress(
        &point, certificate_value + OS_PKI_TLV_VALUE_OFFSET + 1,
        certificate_value[OS_PKI_TLV_LENGTH_OFFSET] - 1,
        certificate_value[OS_PKI_TLV_VALUE_OFFSET] & 0x1));
    cert_public_key->W[0] = 0x04;
    CX_CHECK(sys_cx_ecpoint_export(&point, cert_public_key->W + 1, domain_len,
                                   cert_public_key->W + 1 + domain_len,
                                   domain_len));
    cert_public_key->W_len = 2 * domain_len + 1;
    sys_cx_bn_unlock();
    break;
  case CERTIFICATE_TAG_PK_SIGN_ALGO_ID:
    if ((certificate_value[OS_PKI_TLV_VALUE_OFFSET] >=
         C_os_pki_certificate_tag_info[tag].value) ||
        (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].field_len)) {
      return 0x4238;
    }
    os_pki.pk_sign_algo = certificate_value[OS_PKI_TLV_VALUE_OFFSET];
    break;
  case CERTIFICATE_TAG_TARGET_DEVICE:
    if ((certificate_value[OS_PKI_TLV_VALUE_OFFSET] >=
         C_os_pki_certificate_tag_info[tag].value) ||
        (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].field_len)) {
      return 0x4239;
    }
    break;
  case CERTIFICATE_TAG_DEPTH:
    if ((certificate_value[OS_PKI_TLV_VALUE_OFFSET] >=
         C_os_pki_certificate_tag_info[tag].value) ||
        (certificate_value[OS_PKI_TLV_LENGTH_OFFSET] !=
         C_os_pki_certificate_tag_info[tag].field_len)) {
      return 0x423C;
    }
    break;
  default:
    return 0x422D;
  }

end:
  sys_cx_bn_unlock();
  if (error) {
    swo_error = 0x3302;
  }
  return swo_error;
}

static void os_pki_get_next_tag(uint8_t *certificate, uint32_t *offset)
{
  uint8_t tag = certificate[*offset];
  (*offset)++;
  if (C_os_pki_certificate_tag_info[tag].field_len ==
      CERTIFICATE_FIELD_VAR_LEN) {
    *offset += 1 + certificate[*offset];
  } else {
    *offset += 1 + C_os_pki_certificate_tag_info[tag].field_len;
  }
}

static cx_sign_algo_t os_pki_get_signature_algorithm(uint8_t cert_sign_algo_id)
{
  switch (cert_sign_algo_id) {
  case CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA256:
  case CERTIFICATE_SIGN_ALGO_ID_ECDSA_SHA3_256:
  case CERTIFICATE_SIGN_ALGO_ID_ECDSA_KECCAK_256:
  case CERTIFICATE_SIGN_ALGO_ID_ECDSA_RIPEMD160:
    return CX_SIGN_ALGO_ECDSA_RND;
  case CERTIFICATE_SIGN_ALGO_ID_EDDSA_SHA512:
    return CX_SIGN_ALGO_EDDSA_SHA512;
  default:
    return CX_SIGN_ALGO_UNKNOWN;
  }

  return CX_SIGN_ALGO_UNKNOWN;
}

uint32_t sys_os_pki_load_certificate(uint8_t expected_key_usage,
                                     uint8_t *certificate,
                                     size_t certificate_len,
                                     uint8_t *trusted_name,
                                     size_t *trusted_name_len,
                                     cx_ecfp_384_public_key_t *public_key)
{
  uint32_t offset;
  uint8_t cert_hash[OS_PKI_DIGEST_MAX_LEN] = { 0 };
  bool is_verified = false;
  cx_hash_ctx hash_ctx;
  size_t digest_len;
  cx_sign_algo_t sign_algo;
  uint32_t swo_error = SWO_OK;

  os_pki_set = 0;

  for (offset = 0; offset < certificate_len;) {
    if (CERTIFICATE_TAG_SIGNATURE == certificate[offset]) {
      break;
    }
    if ((swo_error = os_pki_check_value(certificate + offset, public_key)) !=
        SWO_OK) {
      explicit_bzero(&os_pki, sizeof(os_pki));
      return swo_error;
    }
    os_pki_get_next_tag(certificate, &offset);
  }

  if (expected_key_usage != os_pki.key_usage) {
    explicit_bzero(&os_pki, sizeof(os_pki));
    return 0x422E;
  }

  if ((CX_SHA3_256 == C_os_sign_algo_hash_info[os_pki.signer_sign_algo]) ||
      (CX_KECCAK == C_os_sign_algo_hash_info[os_pki.signer_sign_algo])) {
    spec_cx_hash_init_ex(&hash_ctx,
                         C_os_sign_algo_hash_info[os_pki.signer_sign_algo],
                         CX_SHA3_256_SIZE);
  } else {
    spec_cx_hash_init(&hash_ctx,
                      C_os_sign_algo_hash_info[os_pki.signer_sign_algo]);
  }

  spec_cx_hash_update(&hash_ctx, certificate, offset);

  spec_cx_hash_final(&hash_ctx, cert_hash);
  digest_len = spec_cx_hash_get_size(&hash_ctx);
  explicit_bzero(&hash_ctx, sizeof(hash_ctx));

  // Skip signature tag
  offset++;
  switch (os_pki.signer_id) {
  case CERTIFICATE_KEY_ID_LEDGER_ROOT_V3:
    is_verified = os_ecdsa_verify_with_root_ca(
        ROOT_CA_V3_KEY_ID, cert_hash, digest_len,
        certificate + offset + OS_PKI_TLV_LENGTH_OFFSET, certificate[offset]);
    break;
  default:
    sign_algo = os_pki_get_signature_algorithm(os_pki.signer_sign_algo);
    is_verified = cx_verify(
        sign_algo, (cx_ecfp_public_key_t *)&os_pki.public_key, cert_hash,
        digest_len, certificate + offset + OS_PKI_TLV_LENGTH_OFFSET,
        certificate[offset]);
    break;
  }

  if (!is_verified) {
    explicit_bzero(&os_pki, sizeof(os_pki));
    return 0x5720;
  }

  cx_ecdsa_internal_init_public_key(public_key->curve, public_key->W,
                                    public_key->W_len,
                                    (cx_ecfp_public_key_t *)&os_pki.public_key);

  if ((trusted_name != NULL) && (trusted_name_len != NULL)) {
    memcpy(trusted_name, os_pki.trusted_name, os_pki.trusted_name_len);
    *trusted_name_len = os_pki.trusted_name_len;
  }

  if (SWO_OK == swo_error) {
    os_pki_set = 1;
  }

  return swo_error;
}

bool sys_os_pki_verify(uint8_t *descriptor_hash, size_t descriptor_hash_len,
                       uint8_t *signature, size_t signature_len)
{
  cx_sign_algo_t sign_algo =
      os_pki_get_signature_algorithm(os_pki.pk_sign_algo);

  if (os_pki_set != 1) {
    return false;
  }

  return cx_verify(sign_algo, (cx_ecfp_public_key_t *)&os_pki.public_key,
                   descriptor_hash, descriptor_hash_len, signature,
                   signature_len);
}

uint32_t sys_os_pki_get_info(uint8_t *key_usage, uint8_t *trusted_name,
                             size_t *trusted_name_len,
                             cx_ecfp_384_public_key_t *public_key)
{
  if (os_pki_set != 1) {
    return 0x590E;
  }
  *key_usage = os_pki.key_usage;
  memcpy(trusted_name, os_pki.trusted_name, os_pki.trusted_name_len);
  *trusted_name_len = os_pki.trusted_name_len;
  if (0 == cx_ecdsa_internal_init_public_key(
               os_pki.public_key.curve, os_pki.public_key.W,
               os_pki.public_key.W_len, (cx_ecfp_public_key_t *)public_key)) {
    return 0x3202;
  }
  return SWO_OK;
}
