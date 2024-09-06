/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PARTITION_HPP_
#define PARTITION_HPP_

#include <aos/common/types.hpp>

namespace aos::zephyr::utils {

/**
 * Calculates partition size.
 *
 * @param name partition name.
 * @return RetWithError<size_t>.
 */
RetWithError<size_t> CalculatePartitionSize(const String& name);

/**
 * Calculates partition used size.
 *
 * @param path partition path.
 * @return RetWithError<uint64_t>.
 */
RetWithError<uint64_t> CalculatePartitionUsedSize(const String& path);

} // namespace aos::zephyr::utils

#endif
