/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGREADERSTUB_HPP_
#define LOGREADERSTUB_HPP_

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "logprovider/logprovider.hpp"

namespace aos::zephyr::logprovider {

/**
 * Log reader stub.
 */
class LogReaderStub : public LogReaderItf {
public:
    /**
     * Returns current log entry.
     *
     * @param entry[out] log entry.
     * @return Error.
     */
    Error GetEntry(LogEntry& entry) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mLogEntries.empty()) {
            return ErrorEnum::eNotFound;
        }

        entry = mLogEntries.front();
        mLogEntries.erase(mLogEntries.begin());

        return ErrorEnum::eNone;
    }

    /**
     * Moves to the next log entry.
     *
     * @return bool.
     */
    bool Next() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        return !mLogEntries.empty();
    }

    /**
     * Resets reader.
     *
     * @return Error.
     */
    Error Reset() override { return ErrorEnum::eNone; }

    /**
     * Sets log entries.
     *
     * @param logEntries log entries.
     */
    void SetLogEntries(const std::vector<LogEntry>& logEntries)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mLogEntries = logEntries;
    }

private:
    std::mutex            mMutex;
    bool                  mValid = false;
    std::vector<LogEntry> mLogEntries;
};

} // namespace aos::zephyr::logprovider

#endif
