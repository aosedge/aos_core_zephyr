/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/log.hpp>

#include "clocksync/clocksync.hpp"

using namespace aos;

bool                gSendClockSyncRequestCalled {};
Mutex               gWaitMessageMutex;
ConditionalVariable gWaitMessageCondVar(gWaitMessageMutex);

class MockConnectionPublisher : public ConnectionPublisherItf {
public:
    aos::Error Subscribes(ConnectionSubscriberItf* subscriber) override
    {
        mSubscriber = subscriber;

        return ErrorEnum::eNone;
    }

    void Unsubscribes(ConnectionSubscriberItf* subscriber) override
    {
        mSubscriber = nullptr;

        return;
    }

    void Notify() const
    {
        mSubscriber->OnConnect();

        return;
    }

private:
    ConnectionSubscriberItf* mSubscriber {};
};

class MockSender : public ClockSyncSenderItf {
public:
    ~MockSender() = default;

    aos::Error SendClockSyncRequest() override
    {
        gSendClockSyncRequestCalled = true;
        gWaitMessageCondVar.NotifyOne();
        return ErrorEnum::eNone;
    }
};

void TestLogCallback(LogModule module, LogLevel level, const aos::String& message)
{
    printk("[clocksync] %s \n", message.CStr());
}

ZTEST_SUITE(clocksync, NULL, NULL, NULL, NULL, NULL);

ZTEST(clocksync, test_clocksync_request)
{
    aos::Log::SetCallback(TestLogCallback);
    MockConnectionPublisher connectionPublisher {};
    MockSender              sender {};

    ClockSync clockSync;

    zassert_equal(clockSync.Init(sender, connectionPublisher), ErrorEnum::eNone, "Failed to initialize clock sync");

    connectionPublisher.Notify();

    gWaitMessageCondVar.Wait([] { return gSendClockSyncRequestCalled; });

    zassert_equal(gSendClockSyncRequestCalled, true, "Failed to send clock sync request");

    gSendClockSyncRequestCalled = false;

    gWaitMessageCondVar.Wait([] { return gSendClockSyncRequestCalled; });

    zassert_equal(gSendClockSyncRequestCalled, true, "Failed to send clock sync request");
}
