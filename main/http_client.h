/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#pragma once

#include <esp_err.h>
#include <stdint.h>

esp_err_t https_get(const char* host_name, const char* path, const char* token,
                    uint8_t* resp_buff, size_t resp_buff_size,
                    uint32_t* recv_size);
