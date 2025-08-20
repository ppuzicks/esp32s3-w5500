/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#include "utils.h"

#include <errno.h>

int hex2char(uint8_t x, char* c) {
  if (x <= 9) {
    *c = x + (char) '0';
  } else if (x <= 15) {
    *c = x - 10 + (char) 'a';
  } else {
    return -EINVAL;
  }

  return 0;
}

size_t bin2hex(const uint8_t* buf, size_t buflen, char* hex, size_t hexlen) {
  if (hexlen < ((buflen * 2U) + 1U)) {
    return 0;
  }

  for (size_t i = 0; i < buflen; i++) {
    if (hex2char(buf[i] >> 4, &hex[2U * i]) < 0) {
      return 0;
    }
    if (hex2char(buf[i] & 0xf, &hex[2U * i + 1U]) < 0) {
      return 0;
    }
  }

  hex[2U * buflen] = '\0';
  return 2U * buflen;
}
