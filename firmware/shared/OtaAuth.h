#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "mbedtls/md.h"

namespace freddy_ota {

inline bool is_hex(const char *value, size_t length) {
  if (!value || strlen(value) != length) return false;
  for (size_t i = 0; i < length; ++i) {
    const char c = value[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

inline void to_hex(const uint8_t *bytes, size_t size, char *out) {
  constexpr char digits[] = "0123456789abcdef";
  for (size_t i = 0; i < size; ++i) {
    out[2 * i] = digits[bytes[i] >> 4];
    out[2 * i + 1] = digits[bytes[i] & 15];
  }
  out[2 * size] = 0;
}

inline bool constant_time_equal(const char *a, const char *b, size_t size) {
  unsigned difference = 0;
  for (size_t i = 0; i < size; ++i)
    difference |= static_cast<unsigned>(a[i] ^ b[i]);
  return difference == 0;
}

// Authentication binds one upload to its device, one-time nonce, image digest,
// and exact binary length. The key is never sent over the network.
inline bool verify(const char *key, const char *device, const char *nonce,
                   const char *sha256, size_t length, const char *signature) {
  if (!key || strlen(key) < 32 || !is_hex(nonce, 32) ||
      !is_hex(sha256, 64) || !is_hex(signature, 64)) return false;
  char message[160];
  const int n = snprintf(message, sizeof(message), "%s\n%s\n%s\n%zu",
                         device, nonce, sha256, length);
  if (n < 0 || static_cast<size_t>(n) >= sizeof(message)) return false;
  const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!md) return false;
  uint8_t digest[32];
  if (mbedtls_md_hmac(md, reinterpret_cast<const uint8_t *>(key), strlen(key),
                      reinterpret_cast<const uint8_t *>(message), n,
                      digest) != 0) return false;
  char expected[65];
  to_hex(digest, sizeof(digest), expected);
  return constant_time_equal(expected, signature, 64);
}

}  // namespace freddy_ota
