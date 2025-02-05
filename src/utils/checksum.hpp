/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHECKSUM_HPP_
#define CHECKSUM_HPP_

#include <aos/common/types.hpp>

namespace aos::zephyr::utils {

/**
 * Calculates SHA-256.
 *
 * @param data[in] data array.
 * @return RetWithError<aos::StaticArray<aos::cSHA256Size>>.
 */
RetWithError<StaticArray<uint8_t, cSHA256Size>> CalculateSha256(const Array<uint8_t>& data);

} // namespace aos::zephyr::utils

#endif
