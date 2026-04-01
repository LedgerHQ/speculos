#define _SDK_2_0_

#include <string.h>
#include "cxlib.h"
#include "hdkey.h"
#include "os_hdkey.h"

bolos_err_t sys_hdkey_derive(HDKEY_derive_mode_t derivation_mode,
                             cx_curve_t curve, const uint32_t *path,
                             size_t path_len, uint8_t *private_key,
                             size_t private_key_len, uint8_t *chain_code,
                             size_t chain_code_len, uint8_t *seed,
                             size_t seed_len)
{
  // private_key is a mandatory output buffer
  if ((private_key == NULL) || (private_key_len == 0)) {
    return 0x420E;
  }

  // path is required for all modes except when explicitly NULL-safe
  if ((path == NULL) && (path_len != 0)) {
    return 0x420E;
  }

  // chain_code and chain_code_len must be consistent
  if ((chain_code == NULL) && (chain_code_len != 0)) {
    return 0x420E;
  }

  // seed and seed_len must be consistent
  if ((seed == NULL) && (seed_len != 0)) {
    return 0x420E;
  }

  HDKEY_params_t params = { 0 };
  params.mode = derivation_mode;
  params.from_app = BOLOS_TRUE;
  params.curve = curve;
  params.path = path;
  params.path_len = path_len;
  params.private_key = private_key;
  params.private_key_len = private_key_len;
  params.chain = chain_code;
  params.chain_len = chain_code_len;
  params.seed_key = seed;
  params.seed_key_len = seed_len;

  os_result_t result = HDKEY_derive(&params);

  explicit_bzero(&params, sizeof(params));

  switch (result) {
  case OS_SUCCESS:
    return 0x0000;
  case OS_ERR_ACCESS_DENIED:
    return 0x550B;
  case OS_ERR_NOT_FOUND:
    return 0x4214;
  case OS_ERR_INTEGRITY:
    return 0x521D;
  case OS_ERR_OVERFLOW:
    return 0x420E;
  case OS_ERR_BAD_FORMAT:
    return 0x4210;
  case OS_ERR_NOT_IMPLEMENTED:
    return 0x4213;
  case OS_ERR_FORBIDDEN:
    return 0x4215;
  case OS_ERR_CRYPTO_INTERNAL:
    return 0x3308;
  case OS_ERR_SHORT_BUFFER:
    return 0x4212;
  case OS_ERR_BAD_PARAM:
  default:
    return 0x420E;
  }
}
