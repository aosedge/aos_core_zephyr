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
 * @param data[in] data array.
 * @return aos::RetWithError<aos::StaticArray<aos::cSHA256Size>>.
 */
aos::RetWithError<aos::StaticArray<uint8_t, aos::cSHA256Size>> CalculateSha256(const aos::Array<uint8_t>& data);

#endif
