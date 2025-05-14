/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <zephyr/sys/timeutil.h>

#include <aos/common/tools/memory.hpp>

#include "log.hpp"
#include "logprovider.hpp"

namespace aos::zephyr::logprovider {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error LogProvider::Init(LogReaderItf& logReader)
{
    LOG_DBG() << "Initialize log provider";

    mLogReader = &logReader;

    return ErrorEnum::eNone;
}

Error LogProvider::Start()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Start log provider";

    if (!mStopped) {
        return ErrorEnum::eWrongState;
    }

    mStopped = false;

    return mThread.Run([this](void*) { ProcessLogRequests(); });
}

Error LogProvider::Stop()
{
    {
        LockGuard lock {mMutex};

        LOG_DBG() << "Stop log provider";

        if (mStopped) {
            return ErrorEnum::eWrongState;
        }

        mStopped = true;
        mCondVar.NotifyAll();
    }

    return mThread.Join();
}

Error LogProvider::GetInstanceLog(const cloudprotocol::RequestLog& request)
{
    LOG_DBG() << "Get instance log: logID=" << request.mLogID;

    return ErrorEnum::eNotSupported;
}

Error LogProvider::GetInstanceCrashLog(const cloudprotocol::RequestLog& request)
{
    LOG_DBG() << "Get instance crash log: logID=" << request.mLogID;

    return ErrorEnum::eNotSupported;
}

Error LogProvider::GetSystemLog(const cloudprotocol::RequestLog& request)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Get system log: logID=" << request.mLogID;

    if (auto err = mLogRequests.PushBack(request); !err.IsNone()) {
        return err;
    }

    mCondVar.NotifyAll();

    return ErrorEnum::eNone;
}

Error LogProvider::Subscribe(sm::logprovider::LogObserverItf& observer)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Subscribe log observer";

    if (mLogObserver != nullptr) {
        return ErrorEnum::eAlreadyExist;
    }

    mLogObserver = &observer;

    return ErrorEnum::eNone;
}

Error LogProvider::Unsubscribe(sm::logprovider::LogObserverItf& observer)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Unsubscribe log observer";

    mLogObserver = nullptr;

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error LogProvider::SendLogChunk(cloudprotocol::PushLog& log)
{
    ++log.mPartsCount;

    if (!mLogObserver) {
        return Error(ErrorEnum::eNotFound, "log observer not set");
    }

    if (auto err = mLogObserver->OnLogReceived(log); !err.IsNone()) {
        return err;
    }

    log.mContent.Clear();

    return ErrorEnum::eNone;
}

Error LogProvider::SendFinalChunk(cloudprotocol::PushLog& log)
{
    log.mStatus = cloudprotocol::LogStatusEnum::eEmpty;

    return SendLogChunk(log);
}

Error LogProvider::SendErrorLog(const String& logID, const Error& err)
{
    auto log = MakeUnique<cloudprotocol::PushLog>(&mAllocator);

    log->mLogID       = logID;
    log->mMessageType = cloudprotocol::LogMessageTypeEnum::ePushLog;
    log->mStatus      = cloudprotocol::LogStatusEnum::eError;
    log->mErrorInfo   = err;
    log->mPartsCount  = 0;
    log->mPart        = 1;

    return SendLogChunk(*log);
}

Error LogProvider::HandleLogRequest(const cloudprotocol::RequestLog& request)
{
    LOG_DBG() << "Handle log request: logID=" << request.mLogID;

    if (auto err = mLogReader->Reset(); !err.IsNone()) {
        return err;
    }

    auto logEntry = MakeUnique<LogEntry>(&mAllocator);

    auto log          = MakeUnique<cloudprotocol::PushLog>(&mAllocator);
    log->mLogID       = request.mLogID;
    log->mMessageType = cloudprotocol::LogMessageTypeEnum::ePushLog;
    log->mStatus      = cloudprotocol::LogStatusEnum::eOk;

    while (mLogReader->Next()) {
        logEntry->Reset();

        if (auto err = mLogReader->GetEntry(*logEntry); !err.IsNone()) {
            LOG_WRN() << "Failed to read log entry: err=" << err;

            continue;
        }

        if (SkipLogEntry(*logEntry, request)) {
            continue;
        }

        if (log->mContent.Size() + logEntry->mContent.Size() > log->mContent.MaxSize()) {
            if (auto err = SendLogChunk(*log); !err.IsNone()) {
                return err;
            }
        }

        log->mContent.Append(logEntry->mContent);
    }

    if (!log->mContent.IsEmpty()) {
        if (auto err = SendLogChunk(*log); !err.IsNone()) {
            return err;
        }
    }

    return SendFinalChunk(*log);
}

void LogProvider::ProcessLogRequests()
{
    while (true) {
        UniqueLock lock {mMutex};

        mCondVar.Wait(lock, [this]() { return !mLogRequests.IsEmpty() || mStopped; });

        if (mStopped) {
            return;
        }

        if (auto err = HandleLogRequest(mLogRequests.Front()); !err.IsNone()) {
            if (auto sendErr = SendErrorLog(mLogRequests.Front().mLogID, err); !sendErr.IsNone()) {
                LOG_ERR() << "Failed to send error log: err=" << sendErr;
            }
        }

        mLogRequests.Erase(mLogRequests.begin());

#if AOS_CONFIG_THREAD_STACK_USAGE
        LOG_DBG() << "Stack usage: size=" << mThread.GetStackUsage();
#endif
    }
}

bool LogProvider::SkipLogEntry(const LogEntry& logEntry, const cloudprotocol::RequestLog& request)
{
    if (logEntry.mContent.IsEmpty()) {
        return true;
    }

    if (!request.mFilter.mFrom.HasValue() && !request.mFilter.mTill.HasValue()) {
        return false;
    }

    if (!logEntry.mTime.HasValue()) {
        return true;
    }

    if (request.mFilter.mFrom.HasValue()) {
        if (logEntry.mTime.GetValue() < request.mFilter.mFrom.GetValue()) {
            return true;
        }
    }

    if (request.mFilter.mTill.HasValue()) {
        auto till = request.mFilter.mTill.GetValue();
        if (logEntry.mTime.GetValue() > till || logEntry.mTime.GetValue() == till) {
            return true;
        }
    }

    return false;
}

} // namespace aos::zephyr::logprovider
