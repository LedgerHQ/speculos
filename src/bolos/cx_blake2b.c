#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "bolos/exception.h"
#include "cx.h"
#include "cx_utils.h"

union cx_u {
  struct {
    cx_blake2b_t blake2b;
    uint64_t m[16];
    uint64_t v[16];
    uint8_t buffer[BLAKE2B_OUTBYTES];
    uint8_t block1[BLAKE2B_BLOCKBYTES];
  } blake;
};

static union cx_u G_cx;

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_blake2b_init(cx_blake2b_t *hash, unsigned int size)
{
  return cx_blake2b_init2(hash, size, NULL, 0, NULL, 0);
}

int cx_blake2b_init2(cx_blake2b_t *hash, unsigned int size, unsigned char *salt,
                     unsigned int salt_len, unsigned char *perso,
                     unsigned int perso_len)
{
  if (salt == NULL && salt_len != 0) {
    goto err;
  }
  if (perso == NULL && perso_len != 0) {
    goto err;
  }
  if (salt_len > 16 || perso_len > 16) {
    goto err;
  }
  if (size % 8 != 0 || size < 8 || size > 512) {
    goto err;
  }
  memset(hash, 0, sizeof(cx_blake2b_t));

  size = size / 8;
  hash->output_size = size;
  hash->header.algo = CX_BLAKE2B;

  if (spec_blake2b_init(&hash->ctx, size, salt, salt_len, perso, perso_len) <
      0) {
    goto err;
  }
  return CX_BLAKE2B;

err:
  THROW(INVALID_PARAMETER);
}

int spec_cx_blake2b_update(cx_blake2b_t *ctx, const uint8_t *data, size_t len)
{
  if (ctx == NULL) {
    return 0;
  }
  if (data == NULL && len != 0) {
    return 0;
  }
  return spec_blake2b_update(&ctx->ctx, data, len) == 0;
}

int spec_cx_blake2b_final(cx_blake2b_t *ctx, uint8_t *digest)
{
  return spec_blake2b_final(&ctx->ctx, digest, ctx->output_size) == 0;
}

size_t spec_cx_blake2b_get_output_size(const cx_blake2b_t *ctx)
{
  return ctx->output_size;
}

int cx_blake2b_validate_context(const cx_blake2b_t *ctx)
{
  size_t output_size = ctx->output_size;
  if (output_size < 1 || output_size > 512 / 8) {
    return 0;
  }

  const struct blake2b_state__ *state = &ctx->ctx;
  if (state->buflen > BLAKE2B_BLOCKBYTES ||
      state->outlen > BLAKE2B_BLOCKBYTES) {
    return 0;
  }
  return 1;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int spec_cx_blake2b(cx_hash_t *hash, int mode, const unsigned char *in,
                    unsigned int len, unsigned char *out, unsigned int out_len)
{

  unsigned int sz = 0;
  if (spec_blake2b_update(&((cx_blake2b_t *)hash)->ctx, in, len) < 0) {
    THROW(INVALID_PARAMETER);
  }
  if (mode & CX_LAST) {
    sz = ((cx_blake2b_t *)hash)->output_size;
    if (out && (out_len < sz)) {
      THROW(INVALID_PARAMETER);
    }
    if (spec_blake2b_final(&((cx_blake2b_t *)hash)->ctx, out, out_len) < 0) {
      THROW(INVALID_PARAMETER);
    }
  }
  return sz;
}

/* clang-format off */

/*
   BLAKE2 reference source code package - reference C implementations

   Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.  You may use this under the
   terms of the CC0, the OpenSSL Licence, or the Apache Public License 2.0, at
   your option.  The terms of these licenses can be found at:

   - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
   - OpenSSL license   : https://www.openssl.org/source/license.html
   - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0

   More information about the BLAKE2 hash function can be found at
   https://blake2.net.
*/

//#include <stdint.h>
//#include <string.h>
//#include <stdio.h>


//#include "cx_blake2.h" //moved on top

#include <stddef.h>
#include <string.h>

/* ***
#include "cx_blake2-impl.h"
//inline here:
*/


#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
  #if   defined(_MSC_VER)
    #define BLAKE2_INLINE __inline
  #elif defined(__GNUC__)
    #define BLAKE2_INLINE __inline__
  #else
    #define BLAKE2_INLINE
  #endif
#else
  #define BLAKE2_INLINE inline
#endif

static BLAKE2_INLINE uint64_t load64( const void *src )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  uint64_t w;
  memcpy(&w, src, sizeof w);
  return w;
#else
  const uint8_t *p = ( const uint8_t * )src;
  return (( uint64_t )( p[0] ) <<  0) |
         (( uint64_t )( p[1] ) <<  8) |
         (( uint64_t )( p[2] ) << 16) |
         (( uint64_t )( p[3] ) << 24) |
         (( uint64_t )( p[4] ) << 32) |
         (( uint64_t )( p[5] ) << 40) |
         (( uint64_t )( p[6] ) << 48) |
         (( uint64_t )( p[7] ) << 56) ;
#endif
}

static BLAKE2_INLINE void store32( void *dst, uint32_t w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  memcpy(dst, &w, sizeof w);
#else
  uint8_t *p = ( uint8_t * )dst;
  p[0] = (uint8_t)(w >>  0);
  p[1] = (uint8_t)(w >>  8);
  p[2] = (uint8_t)(w >> 16);
  p[3] = (uint8_t)(w >> 24);
#endif
}

static BLAKE2_INLINE void store64( void *dst, uint64_t w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  memcpy(dst, &w, sizeof w);
#else
  uint8_t *p = ( uint8_t * )dst;
  p[0] = (uint8_t)(w >>  0);
  p[1] = (uint8_t)(w >>  8);
  p[2] = (uint8_t)(w >> 16);
  p[3] = (uint8_t)(w >> 24);
  p[4] = (uint8_t)(w >> 32);
  p[5] = (uint8_t)(w >> 40);
  p[6] = (uint8_t)(w >> 48);
  p[7] = (uint8_t)(w >> 56);
#endif
}

static BLAKE2_INLINE uint64_t rotr64( const uint64_t w, const unsigned c )
{
  return ( w >> c ) | ( w << ( 64 - c ) );
}

/* prevents compiler optimizing out memset() */
static BLAKE2_INLINE void secure_zero_memory(void *v, size_t n)
{
  memset(v,0,n);
}

/*
endof inline
*** */





static const uint64_t blake2b_IV[8] =
{
  0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
  0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
  0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
  0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

static const uint8_t blake2b_sigma[12][16] =
{
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
  { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
  {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
  {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
  {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
  { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
  { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
  {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
  { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }
};


static void blake2b_set_lastnode( blake2b_state *S )
{
  S->f[1] = (uint64_t)-1;
}

/* Some helper functions, not necessarily useful */
static int blake2b_is_lastblock( const blake2b_state *S )
{
  return S->f[0] != 0;
}

static void blake2b_set_lastblock( blake2b_state *S )
{
  if( S->last_node ) blake2b_set_lastnode( S );

  S->f[0] = (uint64_t)-1;
}

static void blake2b_increment_counter( blake2b_state *S, const uint64_t inc )
{
  S->t[0] += inc;
  S->t[1] += ( S->t[0] < inc );
}

static void spec_blake2b_init0( blake2b_state *S )
{
  size_t i;
  memset( S, 0, sizeof( blake2b_state ) );

  for( i = 0; i < 8; ++i ) S->h[i] = blake2b_IV[i];
}

/* init xors IV with input parameter block */
int spec_blake2b_init_param( blake2b_state *S, const blake2b_param *P )
{
  const uint8_t *p = ( const uint8_t * )( P );
  size_t i;

  spec_blake2b_init0( S );

  /* IV XOR ParamBlock */
  for( i = 0; i < 8; ++i )
    S->h[i] ^= load64( p + sizeof( S->h[i] ) * i );

  S->outlen = P->digest_length;
  return 0;
}



int spec_blake2b_init( blake2b_state *S, size_t outlen,
                  unsigned char *salt, size_t salt_len,
                  unsigned char *personal, size_t personal_len){
  blake2b_param P[1];

  if ( ( !outlen ) || ( outlen > BLAKE2B_OUTBYTES ) ) return -1;

  P->digest_length = (uint8_t)outlen;
  P->key_length    = 0;
  P->fanout        = 1;
  P->depth         = 1;
  store32( &P->leaf_length, 0 );
  store32( &P->node_offset, 0 );
  store32( &P->xof_length, 0 );
  P->node_depth    = 0;
  P->inner_length  = 0;
  memset( P->reserved, 0, sizeof( P->reserved ) );
  memset( P->salt,     0, sizeof( P->salt ) );
  memset( P->personal, 0, sizeof( P->personal ) );
  if (salt) {
    memcpy(P->salt, salt, salt_len);
  }
  if(personal) {
    memcpy(P->personal, personal, personal_len);
  }
  return spec_blake2b_init_param( S, P );
}


int spec_blake2b_init_key( blake2b_state *S, size_t outlen, const void *key, size_t keylen )
{
  blake2b_param P[1];

  if ( ( !outlen ) || ( outlen > BLAKE2B_OUTBYTES ) ) return -1;

  if ( !key || !keylen || keylen > BLAKE2B_KEYBYTES ) return -1;

  P->digest_length = (uint8_t)outlen;
  P->key_length    = (uint8_t)keylen;
  P->fanout        = 1;
  P->depth         = 1;
  store32( &P->leaf_length, 0 );
  store32( &P->node_offset, 0 );
  store32( &P->xof_length, 0 );
  P->node_depth    = 0;
  P->inner_length  = 0;
  memset( P->reserved, 0, sizeof( P->reserved ) );
  memset( P->salt,     0, sizeof( P->salt ) );
  memset( P->personal, 0, sizeof( P->personal ) );

  if( spec_blake2b_init_param( S, P ) < 0 ) return -1;

  {
    //uint8_t block[BLAKE2B_BLOCKBYTES];
    #define block G_cx.blake.block1
    memset( block, 0, BLAKE2B_BLOCKBYTES );
    memcpy( block, key, keylen );
    spec_blake2b_update( S, block, BLAKE2B_BLOCKBYTES );
    secure_zero_memory( block, BLAKE2B_BLOCKBYTES ); /* Burn the key from stack */
    #undef block
  }
  return 0;
}

#define G(r,i,a,b,c,d)                      \
  do {                                      \
    a = a + b + m[blake2b_sigma[r][2*(i)+0]]; \
    d = rotr64(d ^ a, 32);                  \
    c = c + d;                              \
    b = rotr64(b ^ c, 24);                  \
    a = a + b + m[blake2b_sigma[r][2*(i)+1]]; \
    d = rotr64(d ^ a, 16);                  \
    c = c + d;                              \
    b = rotr64(b ^ c, 63);                  \
  } while(0)

static void blake2b_compress( blake2b_state *S, const uint8_t block[BLAKE2B_BLOCKBYTES] )
{
  /*
  uint64_t m[16];
  uint64_t v[16];
  */
  #define m G_cx.blake.m
  #define v G_cx.blake.v

  size_t i;

  for( i = 0; i < 16; ++i ) {
    m[i] = load64( block + i * sizeof( m[i] ) );
  }

  for( i = 0; i < 8; ++i ) {
    v[i] = S->h[i];
  }

  v[ 8] = blake2b_IV[0];
  v[ 9] = blake2b_IV[1];
  v[10] = blake2b_IV[2];
  v[11] = blake2b_IV[3];
  v[12] = blake2b_IV[4] ^ S->t[0];
  v[13] = blake2b_IV[5] ^ S->t[1];
  v[14] = blake2b_IV[6] ^ S->f[0];
  v[15] = blake2b_IV[7] ^ S->f[1];

  for (int r = 0; r < 12; r++) {
    for (i = 0; i < 4; i++) {
      G(r, i, v[i], v[4 + i], v[8 + i], v[12 + i]);
    }

    for (i = 0; i < 4; i++) {
      G(r, 4 + i, v[i], v[4 + ((i + 1) & 3)], v[8 + ((i + 2) & 3)], v[12 + ((i + 3) & 3)]);
    }
  }

  for( i = 0; i < 8; ++i ) {
    S->h[i] = S->h[i] ^ v[i] ^ v[i + 8];
  }
  #undef m
  #undef v
}

#undef G
#undef ROUND

int spec_blake2b_update( blake2b_state *S, const void *pin, size_t inlen )
{
  const unsigned char * in = (const unsigned char *)pin;
  if( inlen > 0 )
  {
    size_t left = S->buflen;
    size_t fill = BLAKE2B_BLOCKBYTES - left;
    if( inlen > fill )
    {
      S->buflen = 0;
      memcpy( S->buf + left, in, fill ); /* Fill buffer */
      blake2b_increment_counter( S, BLAKE2B_BLOCKBYTES );
      blake2b_compress( S, S->buf ); /* Compress */
      in += fill; inlen -= fill;
      while(inlen > BLAKE2B_BLOCKBYTES) {
        blake2b_increment_counter(S, BLAKE2B_BLOCKBYTES);
        blake2b_compress( S, in );
        in += BLAKE2B_BLOCKBYTES;
        inlen -= BLAKE2B_BLOCKBYTES;
      }
    }
    memcpy( S->buf + S->buflen, in, inlen );
    S->buflen += inlen;
  }
  return 0;
}

int spec_blake2b_final( blake2b_state *S, void *out, size_t outlen )
{
  size_t i;
  //uint8_t buffer[BLAKE2B_OUTBYTES] = {0};
  #define buffer G_cx.blake.buffer
  memset(buffer, 0, BLAKE2B_OUTBYTES);

  if((out != NULL) && (outlen < S->outlen))
    return -1;

  if( blake2b_is_lastblock( S ) )
    return -1;

  blake2b_increment_counter( S, S->buflen );
  blake2b_set_lastblock( S );
  memset( S->buf + S->buflen, 0, BLAKE2B_BLOCKBYTES - S->buflen ); /* Padding */
  blake2b_compress( S, S->buf );

  for( i = 0; i < 8; ++i ) /* Output full hash to temp buffer */
    store64( buffer + sizeof( S->h[i] ) * i, S->h[i] );
  memcpy(S->buf, buffer, S->outlen); //.acc destination
  if (out) {
    memcpy(out, buffer, S->outlen );
  }
  secure_zero_memory(buffer, sizeof(buffer));
  return 0;
  #undef buffer
}


/* =============================== UNUSED =============================== */
//Following code is not used
#if defined(BLAKE2_UNUSED)
/* inlen, at least, should be uint64_t. Others can be size_t. */
int blake2b( void *out, size_t outlen, const void *in, size_t inlen, const void *key, size_t keylen )
{
  blake2b_state S[1];

  /* Verify parameters */
  if ( NULL == in && inlen > 0 ) return -1;

  if ( NULL == out ) return -1;

  if( NULL == key && keylen > 0 ) return -1;

  if( !outlen || outlen > BLAKE2B_OUTBYTES ) return -1;

  if( keylen > BLAKE2B_KEYBYTES ) return -1;

  if( keylen > 0 )
  {
    if( spec_blake2b_init_key( S, outlen, key, keylen ) < 0 ) return -1;
  }
  else
  {
    if( spec_blake2b_init( S, outlen ) < 0 ) return -1;
  }

  spec_blake2b_update( S, ( const uint8_t * )in, inlen );
  spec_blake2b_final( S, out, outlen );
  return 0;
}

int blake2( void *out, size_t outlen, const void *in, size_t inlen, const void *key, size_t keylen ) {
  return blake2b(out, outlen, in, inlen, key, keylen);
}

#if defined(SUPERCOP)
int crypto_hash( unsigned char *out, unsigned char *in, unsigned long long inlen )
{
  return blake2b( out, BLAKE2B_OUTBYTES, in, inlen, NULL, 0 );
}
#endif

#if defined(BLAKE2B_SELFTEST)
#include <string.h>
#include "blake2-kat.h"
int main( void )
{
  uint8_t key[BLAKE2B_KEYBYTES];
  uint8_t buf[BLAKE2_KAT_LENGTH];
  size_t i, step;

  for( i = 0; i < BLAKE2B_KEYBYTES; ++i )
    key[i] = ( uint8_t )i;

  for( i = 0; i < BLAKE2_KAT_LENGTH; ++i )
    buf[i] = ( uint8_t )i;

  /* Test simple API */
  for( i = 0; i < BLAKE2_KAT_LENGTH; ++i )
  {
    uint8_t hash[BLAKE2B_OUTBYTES];
    blake2b( hash, BLAKE2B_OUTBYTES, buf, i, key, BLAKE2B_KEYBYTES );

    if( 0 != memcmp( hash, blake2b_keyed_kat[i], BLAKE2B_OUTBYTES ) )
    {
      goto fail;
    }
  }

  /* Test streaming API */
  for(step = 1; step < BLAKE2B_BLOCKBYTES; ++step) {
    for (i = 0; i < BLAKE2_KAT_LENGTH; ++i) {
      uint8_t hash[BLAKE2B_OUTBYTES];
      blake2b_state S;
      uint8_t * p = buf;
      size_t mlen = i;
      int err = 0;

      if( (err = spec_blake2b_init_key(&S, BLAKE2B_OUTBYTES, key, BLAKE2B_KEYBYTES)) < 0 ) {
        goto fail;
      }

      while (mlen >= step) {
        if ( (err = spec_blake2b_update(&S, p, step)) < 0 ) {
          goto fail;
        }
        mlen -= step;
        p += step;
      }
      if ( (err = spec_blake2b_update(&S, p, mlen)) < 0) {
        goto fail;
      }
      if ( (err = spec_blake2b_final(&S, hash, BLAKE2B_OUTBYTES)) < 0) {
        goto fail;
      }

      if (0 != memcmp(hash, blake2b_keyed_kat[i], BLAKE2B_OUTBYTES)) {
        goto fail;
      }
    }
  }

  puts( "ok" );
  return 0;
fail:
  puts("error");
  return -1;
}
#endif

#endif //BLAKE2_UNUSED

/* clang-format on */
