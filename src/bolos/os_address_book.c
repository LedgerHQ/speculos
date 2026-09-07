#define _SDK_2_0_
#include "os_address_book.h"
#include "address_book.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool sys_ADDRESS_BOOK_HMAC(ADDRESS_BOOK_salt_id_t salt_id,
                           const uint8_t *message, size_t message_len,
                           uint8_t *hmac_out)
{
  // Check params
  // hmac_out must not be NULL
  if (hmac_out == NULL) {
    return false;
  }

  if ((message == NULL) && (message_len != 0)) {
    return false;
  }

  // Call kernel
  return ADDRESS_BOOK_hmac(salt_id, message, message_len, hmac_out);
}

bool sys_ADDRESS_BOOK_HMAC_VERIFY(ADDRESS_BOOK_salt_id_t salt_id,
                                  const uint8_t *message, size_t message_len,
                                  const uint8_t *hmac_expected)
{
  // Check params
  // hmac_expected must not be NULL
  if (hmac_expected == NULL) {
    return false;
  }

  if ((message == NULL) && (message_len != 0)) {
    return false;
  }

  // Call kernel
  return ADDRESS_BOOK_hmac_verify(salt_id, message, message_len, hmac_expected);
}
