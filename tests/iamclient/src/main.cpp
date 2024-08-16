/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "iamclient/iamclient.hpp"

#include "stubs/channelmanagerstub.hpp"
#include "stubs/clocksyncstub.hpp"
#include "stubs/nodeinfoproviderstub.hpp"
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

struct iamclient_fixture {
    ClockSyncStub        mClockSync;
    NodeInfoProviderStub mNodeInfoProvider;
    ChannelManagerStub   mChannelManager;
    iamclient::IAMClient mIAMClient;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static aos::Error ReceiveIAMOutgoingMessage(ChannelStub* channel, iamanager_v5_IAMOutgoingMessages& message)
{
    return ReceivePBMessage(
        channel, cWaitTimeout, &message, iamanager_v5_IAMOutgoingMessages_size, &iamanager_v5_IAMOutgoingMessages_msg);
}

static aos::Error SendIAMIncomingMessage(ChannelStub* channel, const iamanager_v5_IAMIncomingMessages& message)
{
    return SendPBMessage(
        channel, &message, iamanager_v5_IAMIncomingMessages_size, &iamanager_v5_IAMIncomingMessages_msg);
}

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    iamclient, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new iamclient_fixture;

        auto err = fixture->mIAMClient.Init(fixture->mClockSync, fixture->mNodeInfoProvider, fixture->mChannelManager);

        zassert_true(err.IsNone(), "Can't initialize SM client: %s", AosErrorToStr(err));

        return fixture;
    },
    nullptr, nullptr, [](void* fixture) { delete static_cast<iamclient_fixture*>(fixture); });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(iamclient, test_Provisioning)
{
}
