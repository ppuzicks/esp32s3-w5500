/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#pragma once

#include <stddef.h>
#include <stdint.h>


int hex2char(uint8_t x, char *c);
size_t bin2hex(const uint8_t *buf, size_t buflen, char *hex, size_t hexlen);
