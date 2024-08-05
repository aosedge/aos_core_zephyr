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

using namespace aos::zephyr::clocksync;

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cWaitTimeout = std::chrono::seconds {5};

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct clocksync_fixture {
    SenderStub     mSender;
    SubscriberStub mSubscriber;
    ClockSync      mClockSync;
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
        zassert_true(err.IsNone(), "Can't initialize clock sync: %s", AosErrorToStr(err));

        err = fixture->mClockSync.Subscribe(fixture->mSubscriber);
        zassert_true(err.IsNone(), "Can't subscribe for clock sync: %s", AosErrorToStr(err));

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
    zassert_true(err.IsNone(), "Error starting clock sync: %s", AosErrorToStr(err));

    err = fixture->mSender.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting sync request: %s", AosErrorToStr(err));

    zassert_true(fixture->mSender.IsSyncRequest());

    err = fixture->mClockSync.Sync(aos::Time::Now());
    zassert_true(err.IsNone(), "Error sync clock: %s", AosErrorToStr(err));

    err = fixture->mSubscriber.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock synced: %s", AosErrorToStr(err));

    zassert_true(fixture->mSubscriber.IsSynced());
}

ZTEST_F(clocksync, test_Unsynced)
{
    auto err = fixture->mSubscriber.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock unsynced: %s", AosErrorToStr(err));

    zassert_false(fixture->mSubscriber.IsSynced());

    err = fixture->mSender.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting sync request: %s", AosErrorToStr(err));

    zassert_true(fixture->mSender.IsSyncRequest());
}
