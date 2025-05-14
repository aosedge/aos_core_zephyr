/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <unistd.h>

#include "logprovider/fslogreader.hpp"

namespace aos::zephyr::logprovider {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error FSLogReader::GetEntry(LogEntry& entry)
{
    LockGuard lock {mMutex};

    if (!mCurrentEntry.HasValue()) {
        return ErrorEnum::eNotFound;
    }

    entry = *mCurrentEntry;

    return ErrorEnum::eNone;
}

bool FSLogReader::Next()
{
    LockGuard lock {mMutex};

    while (HasFilesToRead()) {
        if (auto err = ReadLine(); err.IsNone() && mCurrentEntry.HasValue()) {
            return true;
        }
    }

    mCurrentEntry.Reset();

    return false;
}

Error FSLogReader::Reset()
{
    LockGuard lock {mMutex};

    CloseFile();

    mLogFiles.Clear();

    mCurrentEntry.Reset();

    return ReadLogFiles();
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error FSLogReader::OpenNextFile()
{
    mCurrentPos = 0;
    mFD         = open(mLogFiles.begin()->CStr(), O_RDONLY);
    mLogFiles.Erase(mLogFiles.begin());

    if (mFD < 0) {
        return Error(ErrorEnum::eFailed, "failed to open log file");
    }

    return ErrorEnum::eNone;
}

void FSLogReader::CloseFile()
{
    if (mFD != -1) {
        close(mFD);
        mFD = -1;
    }

    mCurrentPos = 0;
}

Error FSLogReader::ReadLine()
{
    if (mFD == -1) {
        if (auto err = OpenNextFile(); !err.IsNone()) {
            return err;
        }
    }

    mCurrentEntry.EmplaceValue();

    if (auto err = fs::ReadLine(mFD, mCurrentPos, mCurrentEntry->mContent); !err.IsNone()) {
        CloseFile();

        return err;
    }

    mCurrentPos += mCurrentEntry->mContent.Size() + 1;

    if (auto [time, err] = Time::UTC(mCurrentEntry->mContent); err.IsNone()) {
        mCurrentEntry->mTime.SetValue(time);
    }

    return ErrorEnum::eNone;
}

Error FSLogReader::ReadLogFiles()
{
    mLogFiles.Clear();

    fs::DirIterator dirIterator(logger::cLogDir);

    while (dirIterator.Next()) {
        if (dirIterator->mIsDir) {
            continue;
        }

        auto [pos, errFind] = dirIterator->mPath.FindSubstr(0, logger::cLogPrefix);
        if (!errFind.IsNone() || pos != 0) {
            continue;
        }

        if (auto err = mLogFiles.EmplaceBack(); !err.IsNone()) {
            return err;
        }

        fs::AppendPath(mLogFiles.Back(), dirIterator.GetRootPath(), dirIterator->mPath);
    }

    mLogFiles.Sort();

    return ErrorEnum::eNone;
}

bool FSLogReader::HasFilesToRead() const
{
    return mFD != -1 || !mLogFiles.IsEmpty();
}

} // namespace aos::zephyr::logprovider
