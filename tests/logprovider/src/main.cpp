/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/fs.hpp>
#include <aos/common/tools/log.hpp>

#include "logprovider/fslogreader.hpp"
#include "logprovider/logprovider.hpp"
#include "stubs/launcherstub.hpp"
#include "stubs/logobserverstub.hpp"
#include "stubs/logproviderstub.hpp"
#include "utils/log.hpp"
#include "utils/utils.hpp"

namespace aos::zephyr::logprovider {

namespace {

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

const auto cLogTime        = Time::Unix(1706702400);
const auto cFromTimeFilter = Time::Unix(1706702400); // "2024-01-31T12:00:00Z"
const auto cTillTimeFilter = cFromTimeFilter.Add(Time::cHours); // "2024-01-31T13:00:00Z"

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct logprovider_fixture {
    std::unique_ptr<LogObserverStub>           mLogObserver;
    std::unique_ptr<LogReaderStub>             mLogReader;
    std::unique_ptr<LogProvider>               mLogProvider;
    std::unique_ptr<sm::launcher::StorageStub> mLauncherStorage;
};

struct TestFSLogEntry {
    LogEntry                   mLogEntry;
    StaticString<cFilePathLen> mLogFile;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

LogEntry CreateLogEntry(const String& content, Optional<Time> time = {})
{
    LogEntry logEntry;

    logEntry.mTime = time;

    if (time.HasValue()) {
        Error err;

        Tie(logEntry.mContent, err) = time.GetValue().ToUTCString();
        zassert_true(err.IsNone(), "Failed to create log entry: %s", utils::ErrorToCStr(err));
    }

    logEntry.mContent.Append(content);

    return logEntry;
}

TestFSLogEntry CreateFSLogEntry(const String& logFileSuffix, const String& content, Optional<Time> time = {})
{
    TestFSLogEntry fsLogEntry;

    fsLogEntry.mLogEntry = CreateLogEntry(content, time);
    fsLogEntry.mLogFile  = fs::JoinPath(logger::cLogDir, logger::cLogPrefix).Append(logFileSuffix);

    return fsLogEntry;
}

Error WriteLogToFile(const TestFSLogEntry& logEntry)
{
    auto f = std::ofstream(logEntry.mLogFile.CStr(), std::ios::out | std::ios::app);
    if (!f.is_open()) {
        return ErrorEnum::eFailed;
    }

    f << logEntry.mLogEntry.mContent.CStr() << std::endl;

    return ErrorEnum::eNone;
}

void* Setup(void)
{
    Log::SetCallback(
        [](const aos::String&, aos::LogLevel, const aos::String& message) { printk("%s\n", message.CStr()); });

    auto fixture = new logprovider_fixture;

    return fixture;
}

} // namespace

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    logprovider, nullptr, Setup,
    [](void* fixture) {
        auto logproviderFixture = static_cast<logprovider_fixture*>(fixture);

        logproviderFixture->mLogObserver     = std::make_unique<LogObserverStub>();
        logproviderFixture->mLogReader       = std::make_unique<LogReaderStub>();
        logproviderFixture->mLogProvider     = std::make_unique<LogProvider>();
        logproviderFixture->mLauncherStorage = std::make_unique<sm::launcher::StorageStub>();

        auto err = logproviderFixture->mLogProvider->Init(
            *logproviderFixture->mLogReader, *logproviderFixture->mLauncherStorage);
        zassert_true(err.IsNone(), "Failed to initialize log provider: %s", utils::ErrorToCStr(err));

        err = logproviderFixture->mLogProvider->Subscribe(*logproviderFixture->mLogObserver);
        zassert_true(err.IsNone(), "Failed to subscribe log observer: %s", utils::ErrorToCStr(err));

        err = logproviderFixture->mLogProvider->Start();
        zassert_true(err.IsNone(), "Failed to start log provider: %s", utils::ErrorToCStr(err));

        err = aos::fs::ClearDir(logger::cLogDir);
        zassert_true(err.IsNone(), "Failed to remove disk mount point: %s", utils::ErrorToCStr(err));
    },
    [](void* fixture) {
        auto logproviderFixture = static_cast<logprovider_fixture*>(fixture);

        auto err = logproviderFixture->mLogProvider->Stop();
        zassert_true(err.IsNone(), "Can't stop SM client: %s", utils::ErrorToCStr(err));
    },
    nullptr);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(logprovider, test_start_stop)
{
    auto err = fixture->mLogProvider->Start();
    zassert_true(err.Is(ErrorEnum::eWrongState), "Unexpected error: %s", utils::ErrorToCStr(err));

    err = fixture->mLogProvider->Stop();
    zassert_true(err.IsNone(), "Failed to stop log provider: %s", utils::ErrorToCStr(err));

    err = fixture->mLogProvider->Stop();
    zassert_true(err.Is(ErrorEnum::eWrongState), "Unexpected error: %s", utils::ErrorToCStr(err));

    err = fixture->mLogProvider->Start();
    zassert_true(err.IsNone(), "Failed to start log provider: %s", utils::ErrorToCStr(err));
}

sm::launcher::InstanceData CreateInstanceData(const String& instanceID)
{
    auto data = sm::launcher::InstanceData {};

    data.mInstanceID.Assign(instanceID);

    data.mInstanceInfo.mInstanceIdent.mSubjectID.Assign(instanceID);

    return data;
}

ZTEST_F(logprovider, test_get_instance_log)
{
    const std::vector<LogEntry> logEntries = {
        CreateLogEntry("[instance-id-1] should not be filtered out 1", cFromTimeFilter),
        CreateLogEntry("[instance-id-1] should not be filtered out 2", cFromTimeFilter),
        CreateLogEntry("instance-id-1] should be filtered out", cFromTimeFilter),
        CreateLogEntry("no-instance-id should be filtered out", cFromTimeFilter),
    };

    fixture->mLogReader->SetLogEntries(logEntries);

    const auto cInstanceData = CreateInstanceData("instance-id-1");

    auto err = fixture->mLauncherStorage->AddInstance(cInstanceData);
    zassert_true(err.IsNone(), "Failed to add instance data: %s", utils::ErrorToCStr(err));

    auto logRequest                                = std::make_unique<cloudprotocol::RequestLog>();
    logRequest->mLogID                             = "log_id";
    logRequest->mFilter.mInstanceFilter.mSubjectID = cInstanceData.mInstanceInfo.mInstanceIdent.mSubjectID;

    err = fixture->mLogProvider->GetInstanceLog(*logRequest);
    zassert_true(err.IsNone(), "No error expected: %s", utils::ErrorToCStr(err));

    auto response = std::make_unique<cloudprotocol::PushLog>();

    err = fixture->mLogObserver->WaitLogReceived(*response);
    zassert_true(err.IsNone(), "Failed to wait log received: %s", utils::ErrorToCStr(err));

    zassert_equal(response->mLogID, logRequest->mLogID, "Log ID mismatch");
    zassert_equal(response->mStatus, cloudprotocol::LogStatusEnum::eOk, "Log status mismatch");
    zassert_equal(response->mPart, 1, "Log part mismatch");
    zassert_equal(response->mPartsCount, 1, "Log parts count mismatch");

    for (size_t i = 0; i < 2; ++i) {
        auto trimmedContent = logEntries[i].mContent;
        trimmedContent.Trim("\n");

        auto [pos, errFind] = response->mContent.FindSubstr(0, trimmedContent);
        zassert_true(errFind.IsNone(), "Failed to find log entry: %s", utils::ErrorToCStr(errFind));
    }

    err = fixture->mLogObserver->WaitLogReceived(*response);
    zassert_true(err.IsNone(), "Failed to wait log received: %s", utils::ErrorToCStr(err));

    zassert_equal(response->mLogID, logRequest->mLogID, "Log ID mismatch");
    zassert_equal(response->mStatus, cloudprotocol::LogStatusEnum::eEmpty, "Log status mismatch");
    zassert_equal(response->mPart, 2, "Log part mismatch");
    zassert_equal(response->mPartsCount, 2, "Log parts count mismatch");
    zassert_equal(response->mContent.Size(), 0, "Log content size mismatch");
}

ZTEST_F(logprovider, test_get_instance_crash_log)
{
    auto logRequest = std::make_unique<cloudprotocol::RequestLog>();

    auto err = fixture->mLogProvider->GetInstanceCrashLog(*logRequest);
    zassert_true(err.Is(ErrorEnum::eNotSupported), "Not supported error expected: %s", utils::ErrorToCStr(err));
}

ZTEST_F(logprovider, test_get_empty_system_logs)
{
    const std::vector<LogEntry> logEntries = {
        CreateLogEntry("log_entry_1", cTillTimeFilter),
        CreateLogEntry("log_entry_2", cTillTimeFilter),
        CreateLogEntry("log_entry_3", cTillTimeFilter),
        CreateLogEntry("log_entry_4", cTillTimeFilter.Add(Time::cHours)),
        CreateLogEntry("log_entry_5", cFromTimeFilter.Add(-Time::cHours)),
        CreateLogEntry("time not set", {}),
    };

    fixture->mLogReader->SetLogEntries(logEntries);

    auto logRequest           = std::make_unique<cloudprotocol::RequestLog>();
    logRequest->mLogID        = "log_id";
    logRequest->mFilter.mFrom = cFromTimeFilter;
    logRequest->mFilter.mTill = cTillTimeFilter;

    auto err = fixture->mLogProvider->GetSystemLog(*logRequest);
    zassert_true(err.IsNone(), "Failed to get system log: %s", utils::ErrorToCStr(err));

    auto response = std::make_unique<cloudprotocol::PushLog>();

    err = fixture->mLogObserver->WaitLogReceived(*response);
    zassert_true(err.IsNone(), "Failed to wait log received: %s", utils::ErrorToCStr(err));

    zassert_equal(response->mLogID, logRequest->mLogID, "Log ID mismatch");
    zassert_equal(response->mPartsCount, 1, "Log parts count mismatch");
    zassert_equal(response->mStatus, cloudprotocol::LogStatusEnum::eEmpty, "Log status mismatch");
    zassert_equal(response->mContent.Size(), 0, "Log content size mismatch");
}

ZTEST_F(logprovider, test_get_filtered_system_logs)
{
    const std::vector<LogEntry> logEntries = {
        CreateLogEntry("log_entry_1", cFromTimeFilter),
        CreateLogEntry("log_entry_2", cFromTimeFilter),
        CreateLogEntry("log_entry_3", cFromTimeFilter),
        CreateLogEntry("log_entry_4", cFromTimeFilter.Add(10 * Time::cMinutes)),
        CreateLogEntry("log_entry_5", cFromTimeFilter.Add(11 * Time::cMinutes)),
        CreateLogEntry("time not set", {}),
    };

    fixture->mLogReader->SetLogEntries(logEntries);

    auto logRequest           = std::make_unique<cloudprotocol::RequestLog>();
    logRequest->mLogID        = "log_id";
    logRequest->mFilter.mFrom = cFromTimeFilter;
    logRequest->mFilter.mTill = cTillTimeFilter;

    auto err = fixture->mLogProvider->GetSystemLog(*logRequest);
    zassert_true(err.IsNone(), "Failed to get system log: %s", utils::ErrorToCStr(err));

    auto response = std::make_unique<cloudprotocol::PushLog>();

    err = fixture->mLogObserver->WaitLogReceived(*response);
    zassert_true(err.IsNone(), "Failed to wait log received: %s", utils::ErrorToCStr(err));

    zassert_equal(response->mLogID, logRequest->mLogID, "Log ID mismatch");
    zassert_equal(response->mPartsCount, 1, "Log parts count mismatch");
    zassert_equal(response->mStatus, cloudprotocol::LogStatusEnum::eOk, "Log status mismatch");

    err = fixture->mLogObserver->WaitLogReceived(*response);
    zassert_true(err.IsNone(), "Failed to wait log received: %s", utils::ErrorToCStr(err));

    zassert_equal(response->mLogID, logRequest->mLogID, "Log ID mismatch");
    zassert_equal(response->mPartsCount, 2, "Log parts count mismatch");
    zassert_equal(response->mStatus, cloudprotocol::LogStatusEnum::eEmpty, "Log status mismatch");
    zassert_equal(response->mContent.Size(), 0, "Log content size mismatch");
}

ZTEST(logprovider, test_fslogreader)
{
    auto fsLogReader = std::make_unique<FSLogReader>();
    auto logEntry    = std::make_unique<LogEntry>();

    auto err = fsLogReader->Reset();
    zassert_true(err.IsNone(), "Failed to reset log reader: %s", utils::ErrorToCStr(err));

    // No log entries should be available

    zassert_false(fsLogReader->Next(), "Log entry should not be available");

    err = fsLogReader->GetEntry(*logEntry);
    zassert_true(err.Is(ErrorEnum::eNotFound), "Failed to get log entry: %s", utils::ErrorToCStr(err));
    zassert_true(logEntry->mContent.IsEmpty(), "Log entry should be empty");
    zassert_false(logEntry->mTime.HasValue(), "Log time should be empty");

    const std::vector<TestFSLogEntry> logs = {
        CreateFSLogEntry("t1", "F1 Test message 1", cLogTime),
        CreateFSLogEntry("t1", "F1 Test message 2", cLogTime),
        CreateFSLogEntry("t1", "F1 Test message 3", cLogTime),
        CreateFSLogEntry("t1", "F1 Test message 4", cLogTime),
        CreateFSLogEntry("t2", "F1 Test message 5", cLogTime),
        CreateFSLogEntry("t2", "F1 Test message 6", cLogTime),
    };

    for (const auto& log : logs) {
        err = WriteLogToFile(log);
        zassert_true(err.IsNone(), "Failed to write log: %s", utils::ErrorToCStr(err));
    }

    err = fsLogReader->Reset();
    zassert_true(err.IsNone(), "Failed to reset log reader: %s", utils::ErrorToCStr(err));

    std::vector<LogEntry> readLogEntries;

    while (fsLogReader->Next()) {
        logEntry = std::make_unique<LogEntry>();

        err = fsLogReader->GetEntry(*logEntry);
        if (!err.IsNone()) {
            continue;
        }

        readLogEntries.push_back(*logEntry);
    }

    zassert_equal(readLogEntries.size(), logs.size(), "Log entries count mismatched");

    for (size_t i = 0; i < readLogEntries.size(); ++i) {
        auto expectedLog = logs[i].mLogEntry;
        expectedLog.mContent.Trim("\n");

        auto readLog = readLogEntries[i];
        readLog.mContent.Trim("\n");

        zassert_equal(readLog.mContent, expectedLog.mContent, "Log entry mismatched");
        zassert_equal(readLog.mTime, expectedLog.mTime, "Log time mismatched");
    }
}

} // namespace aos::zephyr::logprovider
