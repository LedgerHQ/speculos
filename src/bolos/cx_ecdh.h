
#ifndef SRC_BOLOS_CX_ECDH_H_
#define SRC_BOLOS_CX_ECDH_H_

int sys_cx_ecdh(const cx_ecfp_private_key_t *key, int mode,
                const uint8_t *public_point, size_t P_len, uint8_t *secret,
                size_t secret_len);

#endif /* SRC_BOLOS_CX_ECDH_H_ */
