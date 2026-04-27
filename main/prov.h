/**
 * Copyright (c) 2025 김태희
 *
 * SPDX-Licence-Identifier: MIT
 */

#pragma once

#include <esp_err.h>

void provision_init(void);
esp_err_t update_config(void);
esp_err_t update_firmware(void);
