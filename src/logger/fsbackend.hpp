/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FSBACKEND_HPP_
#define FSBACKEND_HPP_

#include <aos/common/cloudprotocol/log.hpp>
#include <aos/common/tools/error.hpp>
#include <aos/common/tools/memory.hpp>
#include <aos/common/tools/noncopyable.hpp>

#include "logger/types.hpp"

namespace aos::zephyr::logger::backend {

/**
 * File system backend logger.
 *
 * This logger writes logs to the file system.
 */
class FSBackend : public NonCopyable {
public:
    /**
     * Destructor.
     */
    ~FSBackend();

    /**
     * Initializes logger backend.
     *
     * @return Error
     */
    Error Init();

    /**
     * Handles log message.
     *
     * @param data log message.
     * @param length log message length.
     * @return Error
     */
    size_t HandleLog(const uint8_t* data, size_t length);

    /**
     * Returns fs backend logger instance.
     * @return FS&
     */
    static FSBackend& Get();

    /**
     * Sets custom timestamp format for log output.
     */
    static void SetCustomTimestamp();

private:
    static constexpr auto cMaxLogFileNumeral = 2 * cMaxLogFiles;

    static FSBackend sLogBackend;

    FSBackend() = default;

    Error                      ReopenLogFile();
    Error                      WriteToFile();
    Error                      RestoreLogFiles();
    RetWithError<size_t>       GetFileNumber(const String& path);
    StaticString<cFilePathLen> GetFileName(size_t fileNum);
    size_t                     GetFileSize(const String& path);
    Error                      AllocateNewLogFile();
    Error                      ShrinkLogFiles();
    Error                      UpdateLogFileNumerals();
    size_t                     FillLogBuffer(const String& log);

    StaticString<cLogEntryLen>                            mLogBuffer;
    int                                                   mFD                    = -1;
    size_t                                                mFileSize              = 0;
    size_t                                                mCurrentLogFileNumeral = 0;
    StaticArray<StaticString<cFilePathLen>, cMaxLogFiles> mLogFiles;
};

} // namespace aos::zephyr::logger::backend

#endif // FSBACKEND_HPP_
