/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <zephyr/logging/log.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/fs.hpp>
#include <aos/common/tools/log.hpp>

#include "logger/fsbackend.hpp"
#include "utils/utils.hpp"

namespace aos::zephyr::logger {

namespace {

LOG_MODULE_REGISTER(logger_test, LOG_LEVEL_INF);

/***********************************************************************************************************************
 * Constants
 **********************************************************************************************************************/

/**
 * Duration difference approximate time loggers is called and when the log is written.
 */
constexpr auto cLogTimeDiffEpsilon = 10 * Time::cMilliseconds;

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

bool FileContainsLog(const std::string& filePath, const std::string& logEntry, const Time& logTime)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(logEntry) != std::string::npos) {
            auto [fileLogTime, err] = Time::UTC(line.c_str());

            zassert_true(err.IsNone(), "Failed to parse log time: %s", utils::ErrorToCStr(err));

            if (fileLogTime.Sub(logTime) > cLogTimeDiffEpsilon) {
                return false;
            }

            return true;
        }
    }

    return false;
}

std::string GetLogPath(const std::string& suffix)
{
    auto path = fs::JoinPath(cLogDir, cLogPrefix);
    path.Append(suffix.c_str());

    return path.CStr();
}

std::vector<std::string> GetLogFils()
{
    std::vector<std::string> result;

    fs::DirIterator dirIterator(cLogDir);
    while (dirIterator.Next()) {
        if (dirIterator->mIsDir) {
            continue;
        }

        result.push_back(fs::JoinPath(dirIterator.GetRootPath(), dirIterator->mPath).CStr());
    }

    std::sort(result.begin(), result.end());

    return result;
}

std::vector<std::string> CreateLogEntries(size_t count, size_t len)
{
    std::vector<std::string> logEntries;
    logEntries.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        logEntries.emplace_back(std::string(len, 'A' + static_cast<char>(i)));
    }

    return logEntries;
}

void* Setup(void)
{
    backend::FSBackend::SetCustomTimestamp();

    fs::RemoveAll(cLogDir);

    return NULL;
}

} // namespace

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(logger, nullptr, Setup, nullptr, nullptr, nullptr);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(logger, test_fsbackend)
{
    auto err = backend::FSBackend::Get().Init();
    zassert_true(err.IsNone(), "Failed to initialize fs log backend: %s", utils::ErrorToCStr(err));

    std::vector<std::string> logEntries = CreateLogEntries(cMaxLogFiles, Log::cMaxLineLen);

    auto logTime = Time::Now();

    for (const auto& entry : logEntries) {
        LOG_INF("%s", entry.c_str());
    }

    auto logFiles = GetLogFils();
    zassert_equal(logFiles.size(), logEntries.size());

    for (size_t i = 0; i < logEntries.size(); ++i) {
        zassert_equal(logFiles[i], GetLogPath(std::to_string(i)));
        zassert_true(FileContainsLog(logFiles[i], logEntries[i], logTime));
    }

    // Logger backend shrinks old log files if the new log file is created
    // and the number of log files exceeds the limit.

    logTime = Time::Now();

    for (const auto& entry : logEntries) {
        LOG_INF("%s", entry.c_str());
    }

    logFiles = GetLogFils();
    zassert_equal(logFiles.size(), logEntries.size());

    for (size_t i = 0; i < logEntries.size(); ++i) {
        zassert_equal(logFiles[i], GetLogPath(std::to_string(logEntries.size() + i)));
        zassert_true(FileContainsLog(logFiles[i], logEntries[i], logTime));
    }
}

} // namespace aos::zephyr::logger
