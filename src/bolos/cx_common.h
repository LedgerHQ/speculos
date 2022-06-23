
#ifndef CX_COMMON_H
#define CX_COMMON_H

// clang-format off
/**
 * @brief   Cryptography flags
 * @details Some functions take **logical or** of various flags.
 *          The following flags are globally defined:
 *
 * | Bits position  | Values            | Flags                         | Meaning                                    | Algorithms            |
 * |----------------|-------------------|-------------------------------|--------------------------------------------|-----------------------|
 * | 15             | 1000000000000000  | CX_NO_REINIT                  | Do not reinitialize the context on CX_LAST |                       |
 * | 14:12          | 0111000000000000  | CX_ECSCHNORR_Z                | Zilliqa scheme                             | ECSCHNORR             |
 * | 14:12          | 0110000000000000  | CX_ECSCHNORR_LIBSECP          | ECSCHNORR according to libsecp256k1        | ECSCHNORR             |
 * | 14:12          | 0101000000000000  | CX_ECSCHNORR_BSI03111         | ECSCHNORR according to BSI TR-03111        | ECSCHNORR             |
 * | 14:12          | 0100000000000000  | CX_ECSCHNORR_ISO14888_X       | ECSCHNORR according to ISO/IEC 14888-3     | ECSCHNORR             |
 * | 14:12          | 0011000000000000  | CX_ECSCHNORR_ISO14888_XY      | ECSCHNORR according to ISO/IEC 14888-3     | ECSCHNORR             |
 * | 14:12          | 0010000000000000  | CX_ECDH_X                     | ECDH with the x-coordinate of the point    | ECDH                  |
 * | 14:12          | 0001000000000000  | CX_ECDH_POINT                 | ECDH with a point                          | ECDH                  |
 * | 11:9           | 0000100000000000  | CX_RND_PROVIDED               | Provided random                            |                       |
 * | 11:9           | 0000011000000000  | CX_RND_RFC6979                | Random from RFC6979                        |                       |
 * | 11:9           | 0000010000000000  | CX_RND_TRNG                   | Random from a PRNG                         |                       |
 * | 11:9           | 0000001000000000  | CX_RND_PRNG                   | Random from a TRNG                         |                       |
 * | 8:6            | 0000000100000000  | CX_CHAIN_OFB                  | Output feedback mode                       | AES/DES               |
 * | 8:6            | 0000000011000000  | CX_CHAIN_CFB                  | Cipher feedback mode                       | AES/DES               |
 * | 8:6            | 0000000010000000  | CX_CHAIN_CTR                  | Counter mode                               | AES/DES               |
 * | 8:6            | 0000000001000000  | CX_CHAIN_CBC                  | Cipher block chaining mode                 | AES/DES               |
 * | 8:6            | 0000000001000000  | CX_NO_CANONICAL               | Do not compute a canonical signature       | ECDSA/EDDSA/ECSCHNORR |
 * | 8:6            | 0000000000000000  | CX_CHAIN_ECB                  | Electronic codebook mode                   | AES/DES               |
 * | 5:3            | 0000000010100000  | CX_PAD_PKCS1_OAEP             | PKCS1_OAEP padding                         |                       |
 * | 5:3            | 0000000010000000  | CX_PAD_PKCS1_PSS              | PKCS1_PSS padding                          |                       |
 * | 5:3            | 0000000001100000  | CX_PAD_PKCS1_1o5              | PKCS1-v1_5 padding                         |                       |
 * | 5:3            | 0000000001000000  | CX_PAD_ISO9797M2              | ISO9797 padding, method 2                  |                       |
 * | 5:3            | 0000000000100000  | CX_PAD_ISO9797M1              | ISO9797 padding, method 1                  |                       |
 * | 5:3            | 0000000000000000  | CX_PAD_NONE                   | No padding                                 |                       |
 * | 2:1            | 0000000000000110  | CX_SIGN                       | Signature                                  | AES/DES               |
 * | 2:1            | 0000000000000100  | CX_ENCRYPT                    | Encryption                                 | AES/DES               |
 * | 2:1            | 0000000000000010  | CX_VERIFY                     | Signature verification                     | AES/DES               |
 * | 2:1            | 0000000000000000  | CX_DECRYPT                    | Decryption                                 | AES/DES               |
 * | 0              | 0000000000000000  | CX_LAST                       | Last block                                 |                       |
 */
// clang-format on
#define CX_FLAG

/*
 * Bit 0
 */
#define CX_LAST (1 << 0)

/*
 * Bit 1
 */
#define CX_SIG_MODE (1 << 1)

/*
 * Bit 2:1
 */
#define CX_MASK_SIGCRYPT (3 << 1)
#define CX_ENCRYPT       (2 << 1)
#define CX_DECRYPT       (0 << 1)
#define CX_SIGN          (CX_SIG_MODE | CX_ENCRYPT)
#define CX_VERIFY        (CX_SIG_MODE | CX_DECRYPT)

/*
 * Bit 5:3: padding
 */
#define CX_MASK_PAD       (7 << 3)
#define CX_PAD_NONE       (0 << 3)
#define CX_PAD_ISO9797M1  (1 << 3)
#define CX_PAD_ISO9797M2  (2 << 3)
#define CX_PAD_PKCS1_1o5  (3 << 3)
#define CX_PAD_PKCS1_PSS  (4 << 3)
#define CX_PAD_PKCS1_OAEP (5 << 3)

/*
 * Bit 8:6 DES/AES chaining
 */
#define CX_MASK_CHAIN (7 << 6)
#define CX_CHAIN_ECB  (0 << 6)
#define CX_CHAIN_CBC  (1 << 6)
#define CX_CHAIN_CTR  (2 << 6)
#define CX_CHAIN_CFB  (3 << 6)
#define CX_CHAIN_OFB  (4 << 6)

/*
 * Bit 8:6 ECC variant
 */
#define CX_MASK_ECC_VARIANT (7 << 6)
#define CX_NO_CANONICAL     (1 << 6)

/*
 * Bit 11:9
 */
#define CX_MASK_RND     (7 << 9)
#define CX_RND_PRNG     (1 << 9)
#define CX_RND_TRNG     (2 << 9)
#define CX_RND_RFC6979  (3 << 9)
#define CX_RND_PROVIDED (4 << 9)

/*
 * Bit 14:12
 */
#define CX_MASK_EC               (7 << 12)
#define CX_ECDH_POINT            (1 << 12)
#define CX_ECDH_X                (2 << 12)
#define CX_ECSCHNORR_ISO14888_XY (3 << 12)
#define CX_ECSCHNORR_ISO14888_X  (4 << 12)
#define CX_ECSCHNORR_BSI03111    (5 << 12)
#define CX_ECSCHNORR_LIBSECP     (6 << 12)
#define CX_ECSCHNORR_Z           (7 << 12)
/*
 * Bit 15
 */
#define CX_NO_REINIT (1 << 15)

#endif
