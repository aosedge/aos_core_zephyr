/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FSLOGREADER_HPP_
#define FSLOGREADER_HPP_

#include <aos/common/tools/noncopyable.hpp>
#include <aos/common/tools/optional.hpp>
#include <aos/common/tools/thread.hpp>

#include "logger/types.hpp"

#include "logprovider/logprovider.hpp"

namespace aos::zephyr::logprovider {

/**
 * File system log reader.
 */
class FSLogReader : public LogReaderItf, public NonCopyable {
public:
    /**
     * Returns current log entry.
     *
     * @param entry[out] log entry.
     * @return Error.
     */
    Error GetEntry(LogEntry& entry) override;

    /**
     * Moves to the next log entry.
     *
     * @return bool.
     */
    bool Next() override;

    /**
     * Resets reader.
     *
     * @return Error.
     */
    Error Reset() override;

private:
    Error OpenNextFile();
    void  CloseFile();
    Error ReadLine();
    Error ReadLogFiles();
    bool  HasFilesToRead() const;

    Optional<LogEntry>                                            mCurrentEntry = {};
    size_t                                                        mCurrentPos   = 0;
    int                                                           mFD           = -1;
    StaticArray<StaticString<cFilePathLen>, logger::cMaxLogFiles> mLogFiles;
    Mutex                                                         mMutex;
};

} // namespace aos::zephyr::logprovider

#endif
