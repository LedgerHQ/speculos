#define _SDK_2_0_
#include <err.h>
#include <errno.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bolos/cxlib.h"

//-----------------------------------------------------------------------------
// mpi (Multi Precision Integer) related module
//-----------------------------------------------------------------------------

// Maximum number of entries in cx_mpi_array:
#define MAX_MPI_ARRAY_ENTRIES 64

// Maximum number of bytes available:
#define MAX_MPI_BYTE_SIZE 2048

// MPI numbers are aligned on a 16 bytes boundary:
#define CX_MPI_WORD_BYTE_SIZE 16

//-----------------------------------------------------------------------------
// Typedef & structs
//-----------------------------------------------------------------------------

// This structure will store mpi's information.
struct cx_mpi {
  cx_mpi_t *mpi;
  uint32_t size; // Parameter provided to cx_bn_alloc
};

//-----------------------------------------------------------------------------
// Local variables
//-----------------------------------------------------------------------------

// When the BN_CTX structure is initialized (not NULL), it also means
// 'bn_is_locked'.
static BN_CTX *local_bn_ctx = NULL;

// We'll use an array to store mpi related information:
static struct cx_mpi cx_mpi_array[MAX_MPI_ARRAY_ENTRIES];

// Current number of mpi words allocated:
static uint32_t mpi_words_count;

// Current mpi word size, in bytes:
static uint32_t mpi_word_size;

// Total amount of memory currently allocated:
static uint32_t mpi_total_memory;

//-----------------------------------------------------------------------------

static uint32_t size_to_mpi_bytes(uint32_t size)
{
  // MPI numbers are stored on 'words' that have ?? x 16 bytes.
  // Compute the corresponding value:

  size = (size + CX_MPI_WORD_BYTE_SIZE - 1) & ~(CX_MPI_WORD_BYTE_SIZE - 1);
  if (size == 0) {
    size = CX_MPI_WORD_BYTE_SIZE;
  }

  return size;
}

uint32_t size_to_mpi_word_bytes(uint32_t size)
{
  // Make sure size is a multiple of mpi_word_size:
  if (size == 0) {
    size = mpi_word_size;
  } else {
    size += mpi_word_size - 1;
    size /= mpi_word_size;
    size *= mpi_word_size;
  }
  return size;
}

uint32_t cx_mpi_nbytes(const cx_mpi_t *x)
{
  uint32_t size;

  size = BN_num_bytes(x);

  size = size_to_mpi_bytes(size);

  // Make sure size is a multiple of mpi_word_size:
  size = size_to_mpi_word_bytes(size);

  return size;
}

cx_err_t cx_mpi_bytes(const cx_bn_t bn_x, size_t *nbytes)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));

  if (nbytes == NULL) {
    error = CX_INVALID_PARAMETER;
  } else {
    *nbytes = cx_mpi_nbytes(x);
  }

end:
  return error;
}

//-----------------------------------------------------------------------------

cx_err_t cx_bn_to_mpi(const cx_bn_t bn_x, cx_mpi_t **x)
{
  cx_err_t error;

  CX_CHECK(cx_bn_locked());

  if (bn_x < MAX_MPI_ARRAY_ENTRIES) {
    *x = cx_mpi_array[bn_x].mpi;

    if (*x != NULL) {
      error = CX_OK;
    } else {
      error = CX_MEMORY_FULL;
    }
  } else {
    if (bn_x == (cx_bn_t)-1) {
      error = CX_MEMORY_FULL;
    } else {
      error = CX_INVALID_PARAMETER;
    }
  }
end:
  return error;
}

cx_err_t cx_bn_ab_to_mpi(const cx_bn_t bn_a, cx_mpi_t **a, const cx_bn_t bn_b,
                         cx_mpi_t **b)
{
  cx_err_t error;

  CX_CHECK(cx_bn_locked());

  if (bn_a < MAX_MPI_ARRAY_ENTRIES && bn_b < MAX_MPI_ARRAY_ENTRIES) {
    *a = cx_mpi_array[bn_a].mpi;
    *b = cx_mpi_array[bn_b].mpi;

    if (*a != NULL && *b != NULL) {
      error = CX_OK;
    } else {
      error = CX_MEMORY_FULL;
    }
  } else {
    if (bn_a == (cx_bn_t)-1 || bn_b == (cx_bn_t)-1) {
      error = CX_MEMORY_FULL;
    } else {
      error = CX_INVALID_PARAMETER;
    }
  }
end:
  return error;
}

cx_err_t cx_bn_rab_to_mpi(const cx_bn_t bn_r, cx_mpi_t **r, const cx_bn_t bn_a,
                          cx_mpi_t **a, const cx_bn_t bn_b, cx_mpi_t **b)
{
  cx_err_t error;

  CX_CHECK(cx_bn_locked());

  if (bn_a < MAX_MPI_ARRAY_ENTRIES && bn_b < MAX_MPI_ARRAY_ENTRIES &&
      bn_r < MAX_MPI_ARRAY_ENTRIES) {
    *a = cx_mpi_array[bn_a].mpi;
    *b = cx_mpi_array[bn_b].mpi;
    *r = cx_mpi_array[bn_r].mpi;

    if (*a != NULL && *b != NULL && *r != NULL) {
      error = CX_OK;
    } else {
      error = CX_MEMORY_FULL;
    }
  } else {
    if (bn_a == (cx_bn_t)-1 || bn_b == (cx_bn_t)-1 || bn_r == (cx_bn_t)-1) {
      error = CX_MEMORY_FULL;
    } else {
      error = CX_INVALID_PARAMETER;
    }
  }
end:
  return error;
}

cx_err_t cx_bn_rabm_to_mpi(const cx_bn_t bn_r, cx_mpi_t **r, const cx_bn_t bn_a,
                           cx_mpi_t **a, const cx_bn_t bn_b, cx_mpi_t **b,
                           const cx_bn_t bn_m, cx_mpi_t **m)
{
  cx_err_t error;

  CX_CHECK(cx_bn_locked());

  if (bn_a < MAX_MPI_ARRAY_ENTRIES && bn_b < MAX_MPI_ARRAY_ENTRIES &&
      bn_r < MAX_MPI_ARRAY_ENTRIES && bn_m < MAX_MPI_ARRAY_ENTRIES) {
    *a = cx_mpi_array[bn_a].mpi;
    *b = cx_mpi_array[bn_b].mpi;
    *r = cx_mpi_array[bn_r].mpi;
    *m = cx_mpi_array[bn_m].mpi;

    if (*a != NULL && *b != NULL && *r != NULL && *m != NULL) {
      error = CX_OK;
    } else {
      error = CX_MEMORY_FULL;
    }
  } else {
    if (bn_a == (cx_bn_t)-1 || bn_b == (cx_bn_t)-1 || bn_r == (cx_bn_t)-1 ||
        bn_m == (cx_bn_t)-1) {
      error = CX_MEMORY_FULL;
    } else {
      error = CX_INVALID_PARAMETER;
    }
  }
end:
  return error;
}

//-----------------------------------------------------------------------------

BN_CTX *cx_get_bn_ctx(void)
{
  return local_bn_ctx;
}

uint32_t cx_get_bn_size(void)
{
  return size_to_mpi_word_bytes(mpi_word_size);
}

cx_err_t cx_mpi_destroy(cx_bn_t *bn_x)
{
  uint32_t index;
  cx_err_t error;

  error = CX_OK; // By default, until some error occur.

  // bn_x can be NULL or point to an empty slot: return value will be CX_OK.
  if (bn_x != NULL && ((index = *bn_x) < MAX_MPI_ARRAY_ENTRIES) &&
      (cx_mpi_array[index].mpi != NULL)) {
    // BUT if we update values, context MUST be locked!
    if (local_bn_ctx == NULL) {
      error = CX_NOT_LOCKED;
    } else {
      // No need to check for NULL, OpenSSL does it already.
      BN_clear_free(cx_mpi_array[index].mpi);
      cx_mpi_array[index].mpi = NULL;
      mpi_total_memory -= cx_mpi_array[index].size;
      mpi_words_count -= cx_mpi_array[index].size / mpi_word_size;
      cx_mpi_array[index].size = 0;
    }
  }
  return error;
}

cx_err_t cx_mpi_unlock(void)
{
  unsigned int i;

  // Release & clear all allocated cx_mpi_array entries:
  for (i = 0; i < MAX_MPI_ARRAY_ENTRIES; i++)
    cx_mpi_destroy(&i);

  BN_CTX_free(local_bn_ctx);
  local_bn_ctx = NULL;
  mpi_total_memory = 0;
  mpi_word_size = 0;
  mpi_words_count = 0;

  return CX_OK;
}

cx_err_t cx_mpi_lock(size_t word_size, uint32_t flags __attribute__((unused)))
{
  cx_err_t error;

  if (local_bn_ctx != NULL) {
    error = CX_NOT_UNLOCKED;
  } else {
    local_bn_ctx = BN_CTX_new();

    if (local_bn_ctx == NULL) {
      error = CX_INTERNAL_ERROR;
    } else {
      mpi_total_memory = 0;
      mpi_word_size = size_to_mpi_bytes(word_size);
      mpi_words_count = 0;
      error = CX_OK;
    }
  }
  return error;
}

cx_mpi_t *cx_mpi_alloc(cx_bn_t *bn_x, size_t size)
{
  cx_mpi_t *x;
  size_t i;

  x = NULL;
  *bn_x = -1;

  // Convert size to number of necessary mpi bytes:
  size = size_to_mpi_word_bytes(size);

  // Don't exceed maximum available memory:
  if ((size + mpi_total_memory) <= MAX_MPI_BYTE_SIZE) {
    // Search first available slot (not very optimized but we don't care
    // here):
    for (i = 0; i < MAX_MPI_ARRAY_ENTRIES; i++) {
      if (cx_mpi_array[i].mpi == NULL) {
        x = BN_new();

        if (x != NULL) {
          cx_mpi_array[i].mpi = x;
          *bn_x = i;
          cx_mpi_array[i].size = size;
          mpi_total_memory += size;
          mpi_words_count += size / mpi_word_size;
        }
        break;
      }
    }
  }
  return x;
}

cx_err_t cx_mpi_init(cx_mpi_t *x, const uint8_t *bytes, size_t nbytes)
{
  cx_err_t error;

  // Next function return the cx_mpi_t or NULL on error:
  if (BN_bin2bn(bytes, nbytes, x) == NULL) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_mpi_t *cx_mpi_alloc_init(cx_bn_t *bn_x, size_t size, const uint8_t *bytes,
                            size_t nbytes)
{
  cx_mpi_t *x;

  // Convert size to number of necessary mpi bytes:
  if (nbytes > size) {
    size = nbytes;
  }
  size = size_to_mpi_word_bytes(size);

  x = cx_mpi_alloc(bn_x, size);
  if (x) {
    // Next function return the cx_mpi_t or NULL on error:
    if (BN_bin2bn(bytes, nbytes, x) == NULL) {
      cx_mpi_destroy(bn_x);
      x = NULL;
    }
  }
  return x;
}

cx_err_t cx_mpi_export(const cx_mpi_t *src, uint8_t *dst_ptr, size_t dst_len)
{
  // Instead of just using OpenSSL BN_bn2bin, let's do it like BOLOS do:
  cx_err_t error;
  uint8_t src_ptr[MAX_MPI_BYTE_SIZE];
  uint32_t src_len, offset;

  src_len = cx_mpi_nbytes(src);
  if (src_len > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }

  // Convert src cx_mpi_t into big-endian bytes form:
  if (BN_bn2binpad(src, src_ptr, src_len) == -1) {
    error = CX_INTERNAL_ERROR;
    goto end;
  }
  // Copy src big-endian bytes to dst big-endian bytes:
  if (src_len >= dst_len) {
    offset = src_len - dst_len;
    memcpy(dst_ptr, src_ptr + offset, dst_len);
  } else {
    offset = dst_len - src_len;
    memset(dst_ptr, 0, offset);
    memcpy(dst_ptr + offset, src_ptr, src_len);
  }
  error = CX_OK;
end:

  return error;
}

cx_err_t cx_mpi_copy(cx_mpi_t *dst, cx_mpi_t *src)
{
  // Instead of using OpenSSL BN_copy, let's do it like BOLOS do:
  cx_err_t error;
  uint8_t dst_ptr[MAX_MPI_BYTE_SIZE];
  uint32_t dst_len;

  if (dst == src) {
    return CX_OK;
  }
  dst_len = cx_mpi_nbytes(src);
  if (dst_len > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }
  // Copy src cx_mpi_t into dst big-endian bytes:
  CX_CHECK(cx_mpi_export(src, dst_ptr, dst_len));

  // Now, convert back dst big-endian bytes to cx_mpi_t:
  if (BN_bin2bn(dst_ptr, dst_len, dst) == NULL) {
    error = CX_INTERNAL_ERROR;
  }
end:

  return error;
}

void cx_mpi_set_u32(cx_mpi_t *x, uint32_t n)
{
  uint8_t bytes[4];

  // Convert the 32-bit value into a 4 bytes array, MSB first:
  bytes[0] = (n >> 24) & 0xFF;
  bytes[1] = (n >> 16) & 0xFF;
  bytes[2] = (n >> 8) & 0xFF;
  bytes[3] = (n >> 0) & 0xFF;

  BN_bin2bn(bytes, 4, x);
}

uint32_t cx_mpi_get_u32(const cx_mpi_t *x)
{
  uint8_t bytes[MAX_MPI_BYTE_SIZE];
  int num_bytes, shift;
  uint32_t value;

  value = 0;
  // Convert the bignum into big-endian form and store it in 'bytes':
  // (no need to expand mod 16 and fill with 0, just go as fast as possible)
  num_bytes = BN_num_bytes(x);
  if (num_bytes > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }
  num_bytes = BN_bn2bin(x, bytes);
  // Now, convert those big-endian bytes into a 32-bit value:
  shift = 0;
  for (int i = 0; i < num_bytes && i < 4; i++, shift += 8) {
    value |= (uint32_t)(bytes[num_bytes - i - 1]) << shift;
  }

  return value;
}

int32_t cx_mpi_cmp_u32(const cx_mpi_t *a, uint32_t b)
{
  uint8_t bytes[MAX_MPI_BYTE_SIZE];
  int num_bytes;
  int32_t diff, i;
  uint32_t value, shift;

  diff = 1; // By default values are different (a > b).

  // Convert the bignum into big-endian form and store it in 'bytes':
  // (no need to expand mod 16 and fill with 0, just go as fast as possible)
  num_bytes = BN_num_bytes(a);
  if (num_bytes > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }

  num_bytes = BN_bn2bin(a, bytes);

  // Only last 4 bytes should contain something:
  for (i = 0; i < (num_bytes - 4); i++)
    if (bytes[i])
      break;

  if (i >= (num_bytes - 4)) {
    // Now, convert those big-endian bytes into a 32-bit value:
    shift = 0;
    value = 0;
    if (num_bytes > 4)
      num_bytes = 4;

    for (i = 0; i < num_bytes; i++, shift += 8) {
      value |= (uint32_t)(bytes[num_bytes - i - 1]) << shift;
    }
    // Compare this value to b:
    if (value < b)
      diff = -1;
    else if (value == b)
      diff = 0;
    // No need to check if (value > b), as diff = 1 by default.
  }

  return diff;
}

int cx_mpi_cmp(cx_mpi_t *a, cx_mpi_t *b)
{
  int diff;
  uint8_t a_ptr[MAX_MPI_BYTE_SIZE], b_ptr[MAX_MPI_BYTE_SIZE];
  uint32_t a_len, b_len;

  diff = -1; // By default (a < b)
  // Instead of using OpenSSL BN_cmp, let's do it like BOLOS do:
  a_len = cx_mpi_nbytes(a);
  b_len = cx_mpi_nbytes(b);
  if (a_len > MAX_MPI_BYTE_SIZE || b_len > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }
  // TODO be sure the next test is required (numbers may have different size
  // but still be 'comparables', for example if MSB of a number are at 0)
  if (a_len == b_len) {
    // Convert a cx_mpi_t into big-endian bytes form:
    if (BN_bn2binpad(a, a_ptr, a_len) != -1 &&
        BN_bn2binpad(b, b_ptr, b_len) != -1) {
      diff = memcmp(a_ptr, b_ptr, a_len);
    }
  }

  // In fact, this function does not return -1, 0, or +1, but the diff
  // between first different bytes (and 0 if they are the same).
  return diff;
}

int cx_mpi_is_odd(const cx_mpi_t *a)
{
  return BN_is_odd(a);
}

int cx_mpi_is_one(const cx_mpi_t *a)
{
  return BN_is_one(a);
}

int cx_mpi_is_zero(const cx_mpi_t *a)
{
  return BN_is_zero(a);
}

cx_err_t cx_mpi_tst_bit(const cx_mpi_t *x, uint32_t pos, bool *set)
{
  uint32_t nbytes;

  nbytes = cx_mpi_nbytes(x);
  if (pos > (nbytes * 8 - 1)) {
    return CX_INVALID_PARAMETER;
  }
  *set = BN_is_bit_set(x, pos);

  return CX_OK;
}

cx_err_t cx_mpi_set_bit(cx_mpi_t *x, const uint32_t pos)
{
  cx_err_t error;
  uint32_t nbytes;

  nbytes = cx_mpi_nbytes(x);
  if (pos > (nbytes * 8 - 1)) {
    return CX_INVALID_PARAMETER;
  }

  if (!BN_set_bit(x, pos)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_clr_bit(cx_mpi_t *x, const uint32_t pos)
{
  cx_err_t error;

  if (!BN_clear_bit(x, pos)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

uint32_t cx_mpi_cnt_bits(const cx_mpi_t *x)
{
  uint8_t a[MAX_MPI_BYTE_SIZE];
  uint32_t len, nbits;
  uint8_t b;
  uint8_t *p;

  nbits = 0;

  // Convert the bignum into big-endian form and store it in 'a':
  // (no need to expand mod 16 and fill with 0, just go as fast as possible)
  len = BN_num_bytes(x);
  if (len > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }

  // Convert a cx_mpi_t into big-endian bytes form:
  len = BN_bn2bin(x, a);

  p = a;
  while (*p == 0) {
    p++;
    len--;
    if (len == 0)
      break;
  }
  if (len != 0) {
    len = len * 8;
    b = *p;
    while ((b & 0x80) == 0) {
      b = b << 1;
      len--;
    }
  }

  nbits = len;

  return nbits;
}

cx_err_t cx_mpi_shr(cx_mpi_t *x, const uint32_t n)
{
  cx_err_t error;

  // BOLOS return an error in this case, just do the same:
  if (n > (cx_mpi_nbytes(x) * 8)) {
    return CX_INVALID_PARAMETER;
  }
  if (!BN_rshift(x, x, n)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_shl(cx_mpi_t *x, const uint32_t n)
{
  cx_err_t error;
  uint32_t num_bytes, res_bytes;
  uint8_t bytes[MAX_MPI_BYTE_SIZE];

  num_bytes = cx_mpi_nbytes(x);

  // BOLOS return an error in this case: lets do the same.
  // TODO check if this is really wanted.
  if (n > (num_bytes * 8)) {
    return CX_INVALID_PARAMETER;
  }
  if (!BN_lshift(x, x, n)) {
    return CX_INTERNAL_ERROR;
  }
  error = CX_OK;
  // Currently, BOLOS does not 'extend' the number: lets do the same.
  // Be sure the result remain same size (mod 16) than original one:
  res_bytes = cx_mpi_nbytes(x);
  if (res_bytes > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }
  if (res_bytes > num_bytes) {
    res_bytes = BN_bn2bin(x, bytes);
    if (res_bytes > num_bytes &&
        BN_bin2bn(&bytes[res_bytes - num_bytes], num_bytes, x) == NULL) {
      error = CX_INTERNAL_ERROR;
    }
  }
  return error;
}

cx_err_t cx_mpi_xor(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b)
{
  cx_err_t error;
  uint8_t a_ptr[MAX_MPI_BYTE_SIZE], b_ptr[MAX_MPI_BYTE_SIZE],
      r_ptr[MAX_MPI_BYTE_SIZE];
  uint32_t i, j, a_len, b_len, r_len;

  error = CX_MEMORY_FULL; // By default for the moment.
  a_len = cx_mpi_nbytes(a);
  if (a_len > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }
  // Convert a cx_mpi_t into big-endian bytes form:
  if (BN_bn2binpad(a, a_ptr, a_len) != -1) {
    b_len = cx_mpi_nbytes(b);
    if (b_len > MAX_MPI_BYTE_SIZE) {
      return CX_INVALID_PARAMETER;
    }
    // Convert b cx_mpi_t into big-endian bytes form:
    if (BN_bn2binpad(b, b_ptr, b_len) != -1) {
      if (a_len >= b_len) {
        r_len = a_len;
        j = a_len - b_len;
        for (i = 0; i < j; i++)
          r_ptr[i] = a_ptr[i]; // a XOR 0 = a
        for (i = 0; i < b_len; i++)
          r_ptr[i + j] = a_ptr[i + j] ^ b_ptr[i];
      } else {
        r_len = b_len;
        j = b_len - a_len;
        for (i = 0; i < j; i++)
          r_ptr[i] = b_ptr[i]; // b XOR 0 = b
        for (i = 0; i < a_len; i++)
          r_ptr[i + j] = b_ptr[i + j] ^ a_ptr[i];
      }
      if (BN_bin2bn(r_ptr, r_len, r) != NULL) {
        error = CX_OK;
      } else {
        error = CX_INTERNAL_ERROR;
      }
    }
  }
  return error;
}

cx_err_t cx_mpi_or(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b)
{
  cx_err_t error;
  uint8_t a_ptr[MAX_MPI_BYTE_SIZE], b_ptr[MAX_MPI_BYTE_SIZE],
      r_ptr[MAX_MPI_BYTE_SIZE];
  uint32_t i, j, a_len, b_len, r_len;

  error = CX_MEMORY_FULL; // By default for the moment.
  a_len = cx_mpi_nbytes(a);
  if (a_len > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }
  // Convert a cx_mpi_t into big-endian bytes form:
  if (BN_bn2binpad(a, a_ptr, a_len) != -1) {
    b_len = cx_mpi_nbytes(b);
    if (b_len > MAX_MPI_BYTE_SIZE) {
      return CX_INVALID_PARAMETER;
    }
    // Convert b cx_mpi_t into big-endian bytes form:
    if (BN_bn2binpad(b, b_ptr, b_len) != -1) {
      if (a_len >= b_len) {
        r_len = a_len;
        j = a_len - b_len;
        for (i = 0; i < j; i++)
          r_ptr[i] = a_ptr[i]; // a OR 0 = a
        for (i = 0; i < b_len; i++)
          r_ptr[i + j] = a_ptr[i + j] | b_ptr[i];
      } else {
        r_len = b_len;
        j = b_len - a_len;
        for (i = 0; i < j; i++)
          r_ptr[i] = b_ptr[i]; // b OR 0 = b
        for (i = 0; i < a_len; i++)
          r_ptr[i + j] = b_ptr[i + j] | a_ptr[i];
      }
      if (BN_bin2bn(r_ptr, r_len, r) != NULL) {
        error = CX_OK;
      } else {
        error = CX_INTERNAL_ERROR;
      }
    }
  }
  return error;
}

cx_err_t cx_mpi_and(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b)
{
  cx_err_t error;
  uint8_t a_ptr[MAX_MPI_BYTE_SIZE], b_ptr[MAX_MPI_BYTE_SIZE],
      r_ptr[MAX_MPI_BYTE_SIZE];
  uint32_t i, j, a_len, b_len, r_len;

  error = CX_MEMORY_FULL; // By default for the moment.
  a_len = cx_mpi_nbytes(a);
  // Convert a cx_mpi_t into big-endian bytes form:
  if (BN_bn2binpad(a, a_ptr, a_len) != -1) {
    b_len = cx_mpi_nbytes(b);
    // Convert b cx_mpi_t into big-endian bytes form:
    if (BN_bn2binpad(b, b_ptr, b_len) != -1) {
      if (a_len >= b_len) {
        r_len = a_len;
        j = a_len - b_len;
        memset(r_ptr, 0, j); // a AND 0 = 0
        for (i = 0; i < b_len; i++)
          r_ptr[i + j] = a_ptr[i + j] & b_ptr[i];
      } else {
        r_len = b_len;
        j = b_len - a_len;
        memset(r_ptr, 0, j); // b AND 0 = 0
        for (i = 0; i < a_len; i++)
          r_ptr[i + j] = b_ptr[i + j] & a_ptr[i];
      }
      if (BN_bin2bn(r_ptr, r_len, r) != NULL) {
        error = CX_OK;
      } else {
        error = CX_INTERNAL_ERROR;
      }
    }
  }
  return error;
}

cx_err_t cx_mpi_add(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b)
{
  cx_err_t error;
  uint32_t len, r_len;

  if (!BN_add(r, a, b)) {
    return CX_INTERNAL_ERROR;
  }
  // Check if there is a carry (if more bytes are needed to store r):
  len = cx_mpi_nbytes(a);
  if (len < cx_mpi_nbytes(b))
    len = cx_mpi_nbytes(b);

  r_len = BN_num_bytes(r);
  if (r_len > len) {

    // Only keep len bytes:
    if (!BN_mask_bits(r, 8 * len)) {
      error = CX_INTERNAL_ERROR;
    } else {
      error = CX_CARRY;
    }
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_sub(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b)
{
  cx_err_t error;
  uint32_t len;

  if (!BN_sub(r, a, b)) {
    return CX_INTERNAL_ERROR;
  }
  if (BN_is_negative(r)) {
    // Clear the bit saying the BIGNUM is negative
    BN_set_negative(r, 0);

    // Negate the result (ie XOR FFFF +1) and set the Carry:
    len = cx_mpi_nbytes(a);
    if (len < cx_mpi_nbytes(b))
      len = cx_mpi_nbytes(b);

    // Only keep len bytes:
    if ((uint32_t)BN_num_bytes(r) > len && !BN_mask_bits(r, 8 * len)) {
      error = CX_INTERNAL_ERROR;
    } else if ((error = cx_mpi_neg(r)) == CX_OK) {
      error = CX_CARRY;
    }
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_inc(cx_mpi_t *a)
{
  cx_err_t error;

  if (!BN_add_word(a, 1)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_dec(cx_mpi_t *a)
{
  cx_err_t error;

  if (!BN_sub_word(a, 1)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_not(cx_mpi_t *a)
{
  cx_err_t error;
  uint32_t a_len, i;
  uint8_t a_ptr[MAX_MPI_BYTE_SIZE];

  a_len = cx_mpi_nbytes(a);
  if (a_len > MAX_MPI_BYTE_SIZE) {
    return CX_INVALID_PARAMETER;
  }
  if (BN_bn2binpad(a, a_ptr, a_len) != -1) {
    for (i = 0; i < a_len; i++)
      a_ptr[i] = ~a_ptr[i];

    // Next function return the cx_mpi_t or NULL on error:
    if (BN_bin2bn(a_ptr, a_len, a) == NULL) {
      error = CX_INTERNAL_ERROR;
    } else {
      error = CX_OK;
    }
  } else {
    error = CX_INTERNAL_ERROR;
  }
  return error;
}

cx_err_t cx_mpi_neg(cx_mpi_t *a)
{
  cx_err_t error;

  CX_CHECK(cx_mpi_not(a));
  CX_CHECK(cx_mpi_inc(a));
end:
  return error;
}

cx_err_t cx_mpi_mul(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b)
{
  cx_err_t error;

  if (!BN_mul(r, a, b, local_bn_ctx)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_div(cx_mpi_t *r, cx_mpi_t *d, const cx_mpi_t *n)
{
  cx_err_t error;

  if (!BN_div(r, NULL, d, n, local_bn_ctx)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_rem(cx_mpi_t *r, cx_mpi_t *d, const cx_mpi_t *n)
{
  cx_err_t error;

  if (!BN_div(NULL, r, d, n, local_bn_ctx)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_rand(cx_mpi_t *x)
{
  cx_err_t error;
  uint32_t bytes;

  // The length parameter to BN_rand is in bits while size is in bytes:
  bytes = cx_mpi_nbytes(x);

  if (!BN_rand(x, bytes * 8, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_rng(cx_mpi_t *r, const cx_mpi_t *n)
{
  // Generate a random value r in the range 0 < r < n.

  // Be sure n is >= 2:
  if (BN_is_one(n) || BN_is_zero(n)) {
    return CX_INVALID_PARAMETER;
  }
  // As BN_rand_range return a value r in the range 0 <= r < n
  // we need to check r is not equal to 0
  do {
    if (!BN_rand_range(r, n)) {
      return CX_INTERNAL_ERROR;
    }
    // Loop until r != 0
  } while (BN_is_zero(r));

  return CX_OK;
}

cx_err_t cx_mpi_mod_invert_nprime(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *n)
{
  // This Function will compute r = pow(a, n-2) % n
  cx_err_t error;
  cx_mpi_t *p;
  cx_bn_t bn_p;
  uint32_t size;

  // Allocate p which will hold n-2:
  size = cx_mpi_nbytes(n);
  p = cx_mpi_alloc(&bn_p, size);

  if (p == NULL) {
    error = CX_MEMORY_FULL;
  } else {
    if (BN_copy(p, n) == NULL || BN_sub_word(p, 2) == 0 ||
        BN_mod_exp(r, a, p, n, local_bn_ctx) == 0) {
      error = CX_INTERNAL_ERROR;
    } else {
      error = CX_OK;
    }
  }
  cx_mpi_destroy(&bn_p);
  return error;
}

cx_err_t cx_mpi_mod_u32_invert(cx_mpi_t *r, uint32_t e, const cx_mpi_t *n)
{
  cx_err_t error;
  cx_mpi_set_u32(r, e);

  if (BN_mod_inverse(r, r, n, local_bn_ctx) == NULL) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_mod_add(cx_mpi_t *r, cx_mpi_t *a, cx_mpi_t *b,
                        const cx_mpi_t *n)
{
  cx_err_t error;

  if (!BN_mod_add(r, a, b, n, local_bn_ctx)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_mod_sub(cx_mpi_t *r, cx_mpi_t *a, cx_mpi_t *b,
                        const cx_mpi_t *n)
{
  cx_err_t error;

  if (!BN_mod_sub(r, a, b, n, local_bn_ctx)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_mod_mul(cx_mpi_t *r, cx_mpi_t *a, cx_mpi_t *b,
                        const cx_mpi_t *n)
{
  cx_err_t error;

  if (!BN_is_odd(n)) {
    error = CX_INVALID_PARAMETER_VALUE;
  } else if (!BN_mod_mul(r, a, b, n, local_bn_ctx)) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_mod_sqrt(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *n,
                         uint32_t sign)
{
  bool set;

  if (!BN_mod_sqrt(r, a, n, local_bn_ctx)) {
    return CX_INTERNAL_ERROR;
  }
  // manage sign:
  // - if sign:
  // -   sign = 1
  // - if r&1 != sign:
  // -   r = p-r

  sign = (sign) ? 1 : 0;
  cx_mpi_tst_bit(r, 0, &set);
  if (set != sign) {
    if (!BN_sub(r, n, r)) {
      return CX_INTERNAL_ERROR;
    }
  }
  return CX_OK;
}

cx_err_t cx_mpi_mod_pow(cx_mpi_t *r, const cx_mpi_t *a, const cx_mpi_t *e,
                        const cx_mpi_t *n)
{
  // This Function will compute r = pow(a, e) % n
  cx_err_t error;

  // N must be odd
  if ((cx_mpi_is_odd(n)) == 0 || cx_mpi_is_zero(e)) {
    error = CX_INVALID_PARAMETER;
  } else if (BN_mod_exp(r, a, e, n, local_bn_ctx) == 0) {
    error = CX_INTERNAL_ERROR;
  } else {
    error = CX_OK;
  }
  return error;
}

cx_err_t cx_mpi_is_prime(cx_mpi_t *x, bool *prime)
{
  cx_err_t error;
  int is_prime;

  *prime = false;
  error = CX_OK;

  if ((is_prime = BN_is_prime_ex(x, 64, local_bn_ctx, NULL)) == 1) {
    *prime = true;
  } else if (is_prime < 0) {
    error = CX_INTERNAL_ERROR;
  }
  return error;
}

cx_err_t cx_mpi_next_prime(cx_mpi_t *x)
{
  cx_err_t error;
  int is_prime;
  uint32_t x_size;

  // We'll add 2 each time, so be sure x is odd:
  if ((cx_mpi_is_odd(x) == 0) && (!BN_add_word(x, 1))) {
    return CX_INTERNAL_ERROR;
  }
  // Compute x size, to check we do not increase it too much:
  x_size = cx_mpi_nbytes(x);

  error = CX_OK;
  do {
    if (!BN_add_word(x, 2) || (cx_mpi_nbytes(x) > x_size) ||
        ((is_prime = BN_is_prime_ex(x, 64, local_bn_ctx, NULL)) < 0)) {

      if (cx_mpi_nbytes(x) > x_size)
        error = CX_OVERFLOW;
      else
        error = CX_INTERNAL_ERROR;
      break;
    }
  } while (is_prime == 0);

  return error;
}
