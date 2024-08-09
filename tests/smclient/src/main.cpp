/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mutex>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "smclient/smclient.hpp"

#include "stubs/channelmanagerstub.hpp"
#include "stubs/clocksyncstub.hpp"
#include "utils/log.hpp"
#include "utils/pbmessages.hpp"
#include "utils/utils.hpp"

using namespace aos::zephyr;

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cWaitTimeout = std::chrono::seconds {5};

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct smclient_fixture {
    ClockSyncStub      mClockSync;
    ChannelManagerStub mChannelManager;
    smclient::SMClient mSMClient;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static aos::Error ReceiveSMOutgoingMessage(ChannelStub* channel, servicemanager_v4_SMOutgoingMessages& message)
{
    return ReceivePBMessage(channel, cWaitTimeout, &message, servicemanager_v4_SMOutgoingMessages_size,
        &servicemanager_v4_SMOutgoingMessages_msg);
}

static aos::Error SendSMIncomingMessage(ChannelStub* channel, const servicemanager_v4_SMIncomingMessages& message)
{
    return SendPBMessage(
        channel, &message, servicemanager_v4_SMIncomingMessages_size, &servicemanager_v4_SMIncomingMessages_msg);
}

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    smclient, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new smclient_fixture;

        auto err = fixture->mSMClient.Init(fixture->mClockSync, fixture->mChannelManager);

        zassert_true(err.IsNone(), "Can't initialize SM client: %s", AosErrorToStr(err));

        return fixture;
    },
    nullptr, nullptr, [](void* fixture) { delete static_cast<smclient_fixture*>(fixture); });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(smclient, test_ClockSync)
{
    // Wait clock sync start

    auto err = fixture->mClockSync.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock sync start: %s", AosErrorToStr(err));

    zassert_true(fixture->mClockSync.GetStarted());

    // Wait clock sync request

    err = fixture->mSMClient.SendClockSyncRequest();

    zassert_true(err.IsNone(), "Error sending clock sync request: %s", AosErrorToStr(err));

    ChannelStub* channel;

    aos::Tie(channel, err) = fixture->mChannelManager.GetChannel(CONFIG_AOS_SM_OPEN_PORT);
    zassert_true(err.IsNone(), "Getting channel error: %s", AosErrorToStr(err));

    servicemanager_v4_SMOutgoingMessages outgoingMessage servicemanager_v4_SMOutgoingMessages_init_default;

    err = ReceiveSMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", AosErrorToStr(err));

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v4_SMOutgoingMessages_clock_sync_request_tag);

    // Send clock sync

    servicemanager_v4_SMIncomingMessages incomingMessage servicemanager_v4_SMIncomingMessages_init_default;

    incomingMessage.which_SMIncomingMessage                       = servicemanager_v4_SMIncomingMessages_clock_sync_tag;
    incomingMessage.SMIncomingMessage.clock_sync.has_current_time = true;
    incomingMessage.SMIncomingMessage.clock_sync.current_time.seconds = 43;
    incomingMessage.SMIncomingMessage.clock_sync.current_time.nanos   = 234;

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", AosErrorToStr(err));

    err = fixture->mClockSync.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock sync start: %s", AosErrorToStr(err));

    zassert_equal(fixture->mClockSync.GetSyncTime(),
        aos::Time::Unix(incomingMessage.SMIncomingMessage.clock_sync.current_time.seconds,
            incomingMessage.SMIncomingMessage.clock_sync.current_time.nanos));
}
