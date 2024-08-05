/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTMBEDTLSCONFIG_H_
#define TESTMBEDTLSCONFIG_H_

#define MBEDTLS_PEM_WRITE_C
#define MBEDTLS_PK_HAVE_ECC_KEYS
#define MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS
#define MBEDTLS_TEST_SW_INET_PTON
#define MBEDTLS_X509_CREATE_C
#define MBEDTLS_X509_CRT_WRITE_C
#define MBEDTLS_X509_CSR_PARSE_C
#define MBEDTLS_X509_CSR_WRITE_C
#define PSA_CRYPTO_DRIVER_AOS
#define MBEDTLS_NET_C

#endif
