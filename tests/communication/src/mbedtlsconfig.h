/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOSMBEDTLSCONFIG_H_
#define AOSMBEDTLSCONFIG_H_

#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_NET_C

#include "../../../aos_core_lib_cpp/include/aos/common/crypto/mbedtls/mbedtls_config.h"

#endif
