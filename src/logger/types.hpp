/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGGER_TYPES_HPP
#define LOGGER_TYPES_HPP

#include <aos/common/tools/time.hpp>

namespace aos::zephyr::logger {

/**
 * Max log entry length.
 */
static constexpr auto cLogEntryLen = cTimeStrLen + Log::cMaxLineLen;

/**
 * Log directory.
 */
static constexpr auto cLogDir = CONFIG_AOS_LOG_BACKEND_FS_DIR;

/**
 * Log file prefix.
 */
static constexpr auto cLogPrefix = CONFIG_AOS_LOG_BACKEND_FS_FILE_PREFIX;

/**
 * Log file size limit.
 */
static constexpr auto cFileSizeLimit = CONFIG_AOS_LOG_BACKEND_FS_FILE_SIZE;

/**
 *  Max log files.
 */
static constexpr auto cMaxLogFiles = CONFIG_AOS_LOG_BACKEND_FS_FILES_LIMIT;

} // namespace aos::zephyr::logger

#endif
