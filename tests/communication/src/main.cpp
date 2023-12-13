/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <condition_variable>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/log.hpp>

#include "communication/communication.hpp"

#include "mocks/commchannelmock.hpp"
#include "mocks/connectionsubscribermock.hpp"

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cWaitTimeout = std::chrono::seconds {1};

/***********************************************************************************************************************
 * Vars
 **********************************************************************************************************************/

static CommChannelMock sOpenChannel;
static CommChannelMock sSecureChannel;
static Communication   sCommunication;

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
    communication, NULL,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto err = sCommunication.Init(sOpenChannel, sSecureChannel);
        zassert_true(err.IsNone(), "Can't initialize communication: %s", err.Message());

        return nullptr;
    },
    [](void*) {}, NULL, NULL);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(communication, test_Connection)
{
    ConnectionSubscriberMock subscriber;

    sCommunication.Subscribes(subscriber);

    sSecureChannel.SendRead(std::vector<uint8_t>(), aos::ErrorEnum::eFailed);

    auto err = subscriber.WaitDisconnect(cWaitTimeout);
    zassert_true(err.IsNone(), "Wait connection error: %s", err.Message());

    err = subscriber.WaitConnect(cWaitTimeout);
    zassert_true(err.IsNone(), "Wait connection error: %s", err.Message());

    sCommunication.Unsubscribes(subscriber);
}
