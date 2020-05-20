#include <err.h>
#include <stdio.h>

#include "cx.h"
#include "cx_aes.h"
#include "cx_ec.h"
#include "cx_hash.h"
#include "cx_hmac.h"
#include "cx_math.h"
#include "cx_utils.h"
#include "emulate.h"
#include "emu_endorsement.h"

// This header needs to point to the oldest available SDK
#include "bolos_syscalls_1.5.h"

int emulate(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose, sdk_version_t sdk_version)
{
  int retid;
  switch (sdk_version) {
    case SDK_1_5:
      retid = emulate_1_5(syscall, parameters, ret, verbose);
      break;
    case SDK_1_6:
      retid = emulate_1_6(syscall, parameters, ret, verbose);
      break;
    case SDK_BLUE_2_2_5:
      retid = emulate_blue_2_2_5(syscall, parameters, ret, verbose);
      break;
    default:
      errx(1, "Unsupported SDK version %i", sdk_version);
      break;
  }
  return retid;
}


int emulate_common(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose)
{
  int retid;

  switch(syscall) {
  SYSCALL0(check_api_level);

  SYSCALL6(cx_aes, "(%p, 0x%x, %p, %u, %p, %u)",
           const cx_aes_key_t *, key,
           int,                  mode,
           const uint8_t *,      in,
           unsigned int,         len,
           uint8_t *,            out,
           unsigned int,         out_len);

  SYSCALL8(cx_aes_iv, "(%p, 0x%x, %p, %u, %p, %u, %p, %u)",
           const cx_aes_key_t *, key,
           int,                  mode,
           const uint8_t *,      iv,
           unsigned int,         iv_len,
           const uint8_t *,      in,
           unsigned int,         len,
           uint8_t *,            out,
           unsigned int,         out_len);

  SYSCALL3(cx_aes_init_key, "(%p, %u, %p)",
           const uint8_t *, raw_key,
           unsigned int,    key_len,
           cx_aes_key_t *,  key);

  SYSCALL2(cx_blake2b_init, "(%p, %u)",
           cx_blake2b_t *, hash,
           unsigned int,   size);

  SYSCALL6(cx_blake2b_init2, "(%p, %u, %p, %u, %p, %u)",
           cx_blake2b_t *, hash,
           unsigned int,   size,
           uint8_t *,      salt,
           unsigned int,   salt_len,
           uint8_t *,      perso,
           unsigned int,   perso_len);

  SYSCALL3(cx_crc16_update, "(%u, %p, %u)",
           unsigned short, crc, const void *, b, size_t, len);

  SYSCALL6(cx_ecdh, "(%p, %d, %p, %u, %p, %u)",
          const cx_ecfp_private_key_t *, key,
          int,                           mode,
          const uint8_t *,               public_point,
          size_t,                        P_len,
          uint8_t *,                     secret,
          size_t,                        secret_len);

  SYSCALL8(cx_ecdsa_sign, "(%p, 0x%x, %d, %p, %u, %p, %u, %p)",
           const cx_ecfp_private_key_t *, key,
           int,                           mode,
           cx_md_t,                       hashID,
           const uint8_t *,               hash,
           unsigned int,                  hash_len,
           uint8_t *,                     sig,
           unsigned int,                  sig_len,
           unsigned int *,                info);

  SYSCALL7(cx_ecdsa_verify, "(%p, 0x%x, %d, %p, %u, %p, %u)",
           const cx_ecfp_public_key_t *, key,
           int,                          mode,
           cx_md_t,                      hashID,
           const uint8_t *,              hash,
           unsigned int,                 hash_len,
           const uint8_t *,              sig,
           unsigned int,                 sig_len);

  SYSCALL4(cx_ecfp_generate_pair, "(0x%x, %p, %p, %d)",
           cx_curve_t,              curve,
           cx_ecfp_public_key_t *,  public_key,
           cx_ecfp_private_key_t *, private_key,
           int,                     keep_private);

  SYSCALL4(cx_ecfp_init_private_key, "(0x%x, %p, %u, %p)",
           cx_curve_t,              curve,
           const uint8_t *,         raw_key,
           unsigned int,            key_len,
           cx_ecfp_private_key_t *, key);

  SYSCALL4(cx_ecfp_init_public_key, "(0x%x, %p, %u, %p)",
           cx_curve_t,              curve,
           const uint8_t *,         raw_key,
           unsigned int,            key_len,
           cx_ecfp_public_key_t *,  key);

  SYSCALL5(cx_ecfp_scalar_mult, "(0x%x, %p, %u, %p, %u)",
           cx_curve_t,            curve,
           unsigned char *,       P,
           unsigned int,          P_len,
           const unsigned char *, k,
           unsigned int,          k_len);

  SYSCALL10(cx_eddsa_sign, "(%p, 0x%x, 0x%x, %p, %u, %p, %u, %p, %u, %p)",
            const cx_ecfp_private_key_t *, pvkey,
            int,                           mode,
            cx_md_t,                       hashID,
            const unsigned char *,         hash,
            unsigned int,                  hash_len,
            const unsigned char *,         ctx,
            unsigned int,                  ctx_len,
            unsigned char *,               sig,
            unsigned int,                  sig_len,
            unsigned int *,                info);

  SYSCALL9(cx_eddsa_verify, "(%p, 0x%x, 0x%x, %p, %u, %p, %u, %p, %u)",
    const cx_ecfp_public_key_t *, pu_key,
    int,                          mode,
    cx_md_t,                      hashID,
    const unsigned char *,        hash,
    unsigned int,                 hash_len,
    const unsigned char *,        ctx,
    unsigned int,                 ctx_len,
    const unsigned char *,        sig,
    unsigned int,                 sig_len);

  SYSCALL6(cx_hash, "(%p, 0x%x, %p, %u, %p, %u)",
           cx_hash_t *,     hash,
           int,             mode,
           const uint8_t *, in,
           size_t,          len,
           uint8_t *,       out,
           size_t,          out_len);

  SYSCALL4(cx_hash_sha256, "(%p, %u, %p, %u)",
           const uint8_t *, in,
           size_t,          len,
           uint8_t *,       out,
           size_t,          out_len);

  SYSCALL4(cx_hash_sha512, "(%p, %u, %p, %u)",
           const uint8_t *, in,
           size_t,          len,
           uint8_t *,       out,
           size_t,          out_len);

  SYSCALL6(cx_hmac, "(%p, 0x%x, %p, %u, %p, %u)",
           cx_hmac_t *,     hmac,
           int,             mode,
           const uint8_t *, in,
           unsigned int,    len,
           uint8_t *,       out,
           unsigned int,    out_len);

  SYSCALL6(cx_hmac_sha256, "(%p, %u, %p, %u, %p, %u)",
           const uint8_t *, key,
           unsigned int,    key_len,
           const uint8_t *, in,
           unsigned int,    len,
           uint8_t *,       out,
           unsigned int,    out_len);

  SYSCALL2(cx_keccak_init, "(%p, %u)",
          cx_sha3_t *,      hash,
          unsigned int,     size);

  SYSCALL3(cx_hmac_sha256_init, "(%p, %p, %u)",
           cx_hmac_sha256_t *, hmac,
           const uint8_t *,    key,
           unsigned int,       key_len);

  SYSCALL5(cx_math_addm, "(%p, %p, %p, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           const uint8_t *, m,
           unsigned int,    len);

  SYSCALL3(cx_math_cmp, "(%p, %p, %u)",
           const uint8_t *, a,
           const uint8_t *, b,
           unsigned int,    len);

  SYSCALL4(cx_math_invintm, "(%p, %u, %p, %u)",
           uint8_t *,       r,
           uint32_t,        a,
           const uint8_t *, m,
           size_t,          len);

  SYSCALL4(cx_math_invprimem, "(%p, %p, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, m,
           unsigned int,    len);

  SYSCALL2(cx_math_is_prime, "(%p, %u)",
           const uint8_t *, r,
           unsigned int,    len);

  SYSCALL2(cx_math_is_zero, "(%p, %u)",
           const uint8_t *, a,
           unsigned int,    len);

  SYSCALL4(cx_math_modm, "(%p, %u, %p, %u)",
           uint8_t *,       v,
           unsigned int,    len_v,
           const uint8_t *, m,
           unsigned int,    len_m);

  SYSCALL4(cx_math_mult, "(%p, %p, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           unsigned int,    len);

  SYSCALL5(cx_math_multm, "(%p, %p, %p, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           const uint8_t *, m,
           unsigned int,    len);

  SYSCALL2(cx_math_next_prime, "(%p, %u)",
           uint8_t *,          buf,
           unsigned int,       len);

  SYSCALL1(cx_ripemd160_init, "(%p)", cx_ripemd160_t *, hash);

  SYSCALL2(cx_rng, "(%p, %u)",
           uint8_t *,    buffer,
           unsigned int, length);

  SYSCALL0(cx_rng_u8);

  SYSCALL1(cx_sha256_init, "(%p)", cx_sha256_t *, hash);

  SYSCALL2(cx_sha3_init, "(%p, %u)",
          cx_sha3_t *, hash,
          unsigned int, size);

  SYSCALL3(cx_sha3_xof_init, "(%p, %u, %u)",
          cx_sha3_t *, hash,
          unsigned int, size,
          unsigned int, out_length);

  SYSCALL6(cx_math_powm,    "(%p, %p, %p, %u, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, e,
           size_t,          len_e,
           const uint8_t *, m,
           size_t,          len);

  SYSCALL4(cx_math_sub,     "(%p, %p, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           size_t,          len);

  SYSCALL5(cx_math_subm,    "(%p, %p, %p, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           const uint8_t *, m,
           size_t,          len);

  SYSCALL3(nvm_write,  "(%p, %p, %u)",
           void *, dst_addr,
           void *, src_addr,
           size_t, src_len);

  SYSCALL2(os_endorsement_get_public_key, "(%d, %p)",
           uint8_t,   index,
           uint8_t *, buffer);

  SYSCALL3(os_endorsement_key1_sign_data, "(%p, %u, %p)",
           uint8_t *, data,
           size_t,    dataLength,
           uint8_t *, signature);

  SYSCALL0(os_flags);

  SYSCALL0(os_global_pin_invalidate);

  SYSCALL1(os_lib_throw, "(0x%x)",
           unsigned int, exception);

  SYSCALL0(os_perso_isonboarded);

  SYSCALL5(os_perso_derive_node_bip32, "(0x%x, %p, %u, %p, %p)",
           cx_curve_t,       curve,
           const uint32_t *, path,
           size_t,           length,
           uint8_t*,         private_key,
           uint8_t *,        chain
           );

  SYSCALL3(os_registry_get_current_app_tag, "(0x%x, %p, %u)",
           unsigned int, tag,
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL1(os_ux, "(%p)", bolos_ux_params_t *, params);

  SYSCALL0(try_context_get);

  SYSCALL1(try_context_set, "(%p)",
           try_context_t *, context);

  SYSCALL1(os_endorsement_get_code_hash, "(%p)", uint8_t *, buffer);

  SYSCALL2(os_endorsement_get_public_key_certificate, "(%d, %p)",
    unsigned char, index,
    unsigned char *, buffer);

  default:

    errx(1, "failed to emulate syscall 0x%08lx", syscall);
    break;
  }

  return retid;
}
