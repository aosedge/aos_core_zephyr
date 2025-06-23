/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output_custom.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/sys/timeutil.h>

#include <aos/common/tools/fs.hpp>
#include <aos/common/tools/memory.hpp>

#include "fsbackend.hpp"
#include "utils/utils.hpp"

namespace aos::zephyr::logger::backend {

namespace {

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

uint32_t sCurrentLogFormat = 0;

constexpr auto cLogFlags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;

const struct log_backend* log_backend_aos_get(void);

void ActivateBackend()
{
    log_backend_activate(log_backend_aos_get(), nullptr);
}

void DeactivateBackend()
{
    log_backend_deactivate(log_backend_aos_get());
}

int PrintUTCTime(
    const struct log_output* output, const log_timestamp_t timestamp, const log_timestamp_printer_t printer)
{
    (void)timestamp;

    auto now = Time::Now();

    auto [utcTimeStr, err] = now.Time::ToUTCString();
    if (!err.IsNone()) {
        return printer(output, "%s", utils::ErrorToCStr(err));
    }

    return printer(output, "%s", utcTimeStr.CStr());
}

int HandleLog(const uint8_t* data, size_t length, void* ctx)
{
    return FSBackend::Get().HandleLog(data, length);
}

uint8_t __aligned(Z_LOG_MSG_ALIGNMENT) sLogBuffer[cLogEntryLen];
LOG_OUTPUT_DEFINE(sLogOutput, reinterpret_cast<log_output_func_t>(HandleLog), sLogBuffer, cLogEntryLen);

void Panic(struct log_backend const* const backend)
{
    DeactivateBackend();
}

void Dropped(const struct log_backend* const backend, uint32_t cnt)
{
    ARG_UNUSED(backend);

    if (IS_ENABLED(CONFIG_LOG_BACKEND_FS_OUTPUT_DICTIONARY)) {
        log_dict_output_dropped_process(&sLogOutput, cnt);
    } else {
        log_backend_std_dropped(&sLogOutput, cnt);
    }
}

void Process(const struct log_backend* const backend, union log_msg_generic* msg)
{
    log_format_func_t_get(sCurrentLogFormat)(&sLogOutput, &msg->log, cLogFlags);
}

int SetFormat(const struct log_backend* const backend, uint32_t log_type)
{
    sCurrentLogFormat = log_type;

    return 0;
}

const struct log_backend_api log_backend_aos_fs_api = {
    .process    = Process,
    .dropped    = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : Dropped,
    .panic      = Panic,
    .format_set = SetFormat,
};

LOG_BACKEND_DEFINE(log_backend_aos_fs, log_backend_aos_fs_api, false);

const struct log_backend* log_backend_aos_get(void)
{
    return &log_backend_aos_fs;
}

} // namespace

/***********************************************************************************************************************
 * Variables
 **********************************************************************************************************************/

FSBackend FSBackend::sLogBackend = {};

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

FSBackend::~FSBackend()
{
    DeactivateBackend();

    if (mFD != -1) {
        close(mFD);
    }
}

Error FSBackend::Init()
{
    Error err = ErrorEnum::eNone;

    auto activateOnSuccess = DeferRelease(&err, [this](Error* err) {
        if (!err->IsNone()) {
            return;
        }

        ActivateBackend();
    });

    err = fs::MakeDirAll(cLogDir);
    if (!err.IsNone()) {
        return err;
    }

    err = RestoreLogFiles();
    if (!err.IsNone()) {
        fs::ClearDir(cLogDir);
    }

    if (mLogFiles.IsEmpty()) {
        return AllocateNewLogFile();
    }

    ShrinkLogFiles();
    UpdateLogFileNumerals();

    return ReopenLogFile();
}

size_t FSBackend::HandleLog(const uint8_t* data, size_t length)
{
    auto log = String(reinterpret_cast<const char*>(data), length);

    auto addedBytes = FillLogBuffer(log);

    if (mLogBuffer.IsFull() || (!mLogBuffer.IsEmpty() && (mLogBuffer.Back() == '\n' || mLogBuffer.Back() == '\0'))) {
        if (auto err = WriteToFile(); !err.IsNone()) {
            return 0;
        }
    }

    return addedBytes;
}

FSBackend& FSBackend::Get()
{
    return sLogBackend;
}

void FSBackend::SetCustomTimestamp()
{
    if (IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_CUSTOM_TIMESTAMP)) {
        log_custom_timestamp_set(PrintUTCTime);
    }
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error FSBackend::WriteToFile()
{
    if (mFD == -1) {
        return ErrorEnum::eFailed;
    }

    if (mFileSize + mLogBuffer.Size() > cFileSizeLimit) {
        if (auto err = AllocateNewLogFile(); !err.IsNone()) {
            return err;
        }
    }

    ssize_t rc = write(mFD, mLogBuffer.Get(), mLogBuffer.Size());
    if (rc < 0) {
        return ErrorEnum::eFailed;
    }

    mLogBuffer.Clear();

    return ReopenLogFile();
}

Error FSBackend::RestoreLogFiles()
{
    Error err = ErrorEnum::eNone;

    auto onError = DeferRelease(&err, [this](Error* err) {
        if (err->IsNone()) {
            mLogFiles.Sort();

            return;
        }

        mCurrentLogFileNumeral = 0;
        mLogFiles.Clear();
    });

    fs::DirIterator dirIterator(cLogDir);
    while (dirIterator.Next()) {
        if (dirIterator->mIsDir) {
            continue;
        }

        auto fileNumResult = GetFileNumber(dirIterator->mPath);
        if (!fileNumResult.mError.IsNone()) {
            continue;
        }

        mCurrentLogFileNumeral = Max(mCurrentLogFileNumeral, fileNumResult.mValue);

        if (err = mLogFiles.EmplaceBack(); !err.IsNone()) {
            return err;
        }

        fs::AppendPath(mLogFiles.Back(), dirIterator.GetRootPath(), dirIterator->mPath);
    }

    return ErrorEnum::eNone;
}

RetWithError<size_t> FSBackend::GetFileNumber(const String& path)
{
    if (path.Size() < String(cLogPrefix).Size()) {
        return {{}, ErrorEnum::eInvalidArgument};
    }

    String number(path.CStr() + String(cLogPrefix).Size());

    auto fileNum = atoi(number.CStr());
    if (fileNum < 0 || fileNum > cMaxLogFileNumeral) {
        return {{}, ErrorEnum::eInvalidArgument};
    }

    return fileNum;
}

StaticString<cFilePathLen> FSBackend::GetFileName(size_t fileNum)
{
    StaticString<cFilePathLen> path;

    path.Format("%s/%s%ld", cLogDir, cLogPrefix, fileNum);

    return path;
}

size_t FSBackend::GetFileSize(const String& path)
{
    struct stat fileStat;

    if (stat(path.CStr(), &fileStat) == -1) {
        return 0;
    }

    return fileStat.st_size;
}

Error FSBackend::AllocateNewLogFile()
{
    if (mFD != -1) {
        close(mFD);

        mFD = -1;
    }

    if (auto err = ShrinkLogFiles(); !err.IsNone()) {
        return err;
    }

    if (mCurrentLogFileNumeral == cMaxLogFileNumeral) {
        if (auto err = UpdateLogFileNumerals(); !err.IsNone()) {
            return err;
        }
    }

    auto logFilePath = GetFileName(mCurrentLogFileNumeral++);

    if (auto err = mLogFiles.EmplaceBack(logFilePath); !err.IsNone()) {
        return err;
    }

    return ReopenLogFile();
}

Error FSBackend::ShrinkLogFiles()
{
    if (mLogFiles.IsEmpty() || !mLogFiles.IsFull()) {
        return ErrorEnum::eNone;
    }

    if (auto err = fs::Remove(mLogFiles.Front()); !err.IsNone()) {
        return err;
    }

    mLogFiles.Erase(mLogFiles.begin());

    return ErrorEnum::eNone;
}

Error FSBackend::UpdateLogFileNumerals()
{
    mCurrentLogFileNumeral = 0;

    for (auto& logFile : mLogFiles) {
        auto newPath = GetFileName(mCurrentLogFileNumeral);

        if (auto err = fs::Rename(logFile, newPath); !err.IsNone()) {
            return err;
        }

        logFile = newPath;

        ++mCurrentLogFileNumeral;
    }

    return ErrorEnum::eNone;
}

Error FSBackend::ReopenLogFile()
{
    if (mFD >= 0) {
        if (auto ret = close(mFD); ret < 0) {
            return AOS_ERROR_WRAP(errno);
        }

        mFD = -1;
    }

    if (mLogFiles.IsEmpty()) {
        return ErrorEnum::eInvalidArgument;
    }

    mFD = open(mLogFiles.Back().CStr(), O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
    if (mFD < 0) {
        return AOS_ERROR_WRAP(errno);
    }

    mFileSize = GetFileSize(mLogFiles.Back());

    return ErrorEnum::eNone;
}

size_t FSBackend::FillLogBuffer(const String& log)
{
    const auto originalSize  = mLogBuffer.Size();
    const auto availableSize = mLogBuffer.MaxSize() - mLogBuffer.Size();
    const auto copySize      = Min(log.Size(), availableSize);

    if (copySize > 0) {
        mLogBuffer.Insert(mLogBuffer.end(), log.begin(), log.begin() + copySize);
    }

    return mLogBuffer.Size() - originalSize;
}

} // namespace aos::zephyr::logger::backend
