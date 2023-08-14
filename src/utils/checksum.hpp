/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHECKSUM_HPP_
#define CHECKSUM_HPP_

#include <aos/common/types.hpp>

/**
 * Calculates SHA-256.
 *
 * @param data pointer to data.
 * @param size data size.
 * @return aos::RetWithError<SHA256Digest>.
 */
aos::RetWithError<aos::StaticBuffer<aos::cSHA256Size>> CalculateSha256(const void* data, size_t size);

#endif
