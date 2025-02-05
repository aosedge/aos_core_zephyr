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

#include "stubs/senderstub.hpp"
#include "stubs/subscriberstub.hpp"
#include "utils/log.hpp"
#include "utils/utils.hpp"

using namespace aos::zephyr;

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cWaitTimeout = std::chrono::seconds {5};

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct clocksync_fixture {
    SenderStub           mSender;
    SubscriberStub       mSubscriber;
    clocksync::ClockSync mClockSync;
};

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    clocksync, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new clocksync_fixture;

        auto err = fixture->mClockSync.Init(fixture->mSender);
        zassert_true(err.IsNone(), "Can't initialize clock sync: %s", utils::ErrorToCStr(err));

        err = fixture->mClockSync.Subscribe(fixture->mSubscriber);
        zassert_true(err.IsNone(), "Can't subscribe for clock sync: %s", utils::ErrorToCStr(err));

        return fixture;
    },
    [](void* fixture) {
        auto f = static_cast<clocksync_fixture*>(fixture);

        f->mSender.Clear();
    },
    nullptr,
    [](void* fixture) {
        auto f = static_cast<clocksync_fixture*>(fixture);

        auto err = f->mClockSync.Stop();
        zassert_true(err.IsNone(), "Can't stop clock sync: %s", utils::ErrorToCStr(err));

        delete f;
    });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(clocksync, test_Synced)
{
    auto err = fixture->mClockSync.Start();
    zassert_true(err.IsNone(), "Error starting clock sync: %s", utils::ErrorToCStr(err));

    err = fixture->mSender.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting sync request: %s", utils::ErrorToCStr(err));

    zassert_true(fixture->mSender.IsSyncRequest());

    err = fixture->mClockSync.Sync(aos::Time::Now());
    zassert_true(err.IsNone(), "Error sync clock: %s", utils::ErrorToCStr(err));

    err = fixture->mSubscriber.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock synced: %s", utils::ErrorToCStr(err));

    zassert_true(fixture->mSubscriber.IsSynced());
}

ZTEST_F(clocksync, test_Unsynced)
{
    auto err = fixture->mSubscriber.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock unsynced: %s", utils::ErrorToCStr(err));

    zassert_false(fixture->mSubscriber.IsSynced());

    err = fixture->mSender.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting sync request: %s", utils::ErrorToCStr(err));

    zassert_true(fixture->mSender.IsSyncRequest());
}
