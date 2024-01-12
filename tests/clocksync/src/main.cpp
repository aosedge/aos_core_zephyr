/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mutex>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "clocksync/clocksync.hpp"

#include "mocks/sendermock.hpp"

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cWaitTimeout = std::chrono::seconds {5};

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct clocksync_fixture {
    SenderMock mSender;
    ClockSync  mClockSync;
};

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

void TestLogCallback(aos::LogModule module, aos::LogLevel level, const aos::String& message)
{
    static std::mutex mutex;
    static auto       startTime = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

    const char* levelStr = "unknown";

    switch (level.GetValue()) {
    case aos::LogLevelEnum::eDebug:
        levelStr = "dbg";
        break;

    case aos::LogLevelEnum::eInfo:
        levelStr = "inf";
        break;

    case aos::LogLevelEnum::eWarning:
        levelStr = "wrn";
        break;

    case aos::LogLevelEnum::eError:
        levelStr = "err";
        break;

    default:
        levelStr = "n/d";
        break;
    }

    printk("%0.3f [%s] %s\n", now, levelStr, message.CStr());
}

ZTEST_SUITE(
    clocksync, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new clocksync_fixture;

        auto err = fixture->mClockSync.Init(fixture->mSender);
        zassert_true(err.IsNone(), "Can't initialize clock sync: %s", err.Message());

        return fixture;
    },
    [](void* fixture) {
        auto f = static_cast<clocksync_fixture*>(fixture);

        f->mSender.Clear();
    },
    nullptr, [](void* fixture) { delete static_cast<clocksync_fixture*>(fixture); });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(clocksync, test_Synced)
{
    auto err = fixture->mClockSync.Start();
    zassert_true(err.IsNone(), "Error starting clock sync: %s", err.Message());

    err = fixture->mSender.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting sync request: %s", err.Message());

    zassert_true(fixture->mSender.IsSyncRequest());

    err = fixture->mClockSync.Sync(aos::Time::Now());
    zassert_true(err.IsNone(), "Error sync clock: %s", err.Message());

    err = fixture->mSender.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock synced: %s", err.Message());

    zassert_true(fixture->mSender.IsSynced());
}

ZTEST_F(clocksync, test_Unsynced)
{
    auto err = fixture->mSender.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock unsynced: %s", err.Message());

    zassert_true(fixture->mSender.IsSynced());

    err = fixture->mSender.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting sync request: %s", err.Message());

    zassert_true(fixture->mSender.IsSyncRequest());
}
