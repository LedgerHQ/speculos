#ifndef _CX_25519_H
#define _CX_25519_H

int scalarmult_ed25519(BIGNUM *Qx, BIGNUM *Qy, BIGNUM *Px, BIGNUM *Py, BIGNUM *e);
void cx_decode_int(uint8_t *v, size_t len);
void cx_encode_int(uint8_t *v, size_t len);

#endif
