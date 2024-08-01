/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mutex>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/fs.hpp>
#include <aos/common/tools/log.hpp>

#include "provisioning/provisioning.hpp"

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct provisioning_fixture {
    Provisioning mProvisioning;
};

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

void TestLogCallback(const char* module, aos::LogLevel level, const aos::String& message)
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
    provisioning, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new provisioning_fixture;

        return fixture;
    },
    [](void* fixture) {
        auto err = aos::FS::RemoveAll(aos::FS::Dir(CONFIG_AOS_PROVISIONING_FILE));
        zassert_true(err.IsNone(), "Can't remove provisioning dir: %s", err.Message());
    },
    nullptr,
    [](void* fixture) {
        auto err = aos::FS::RemoveAll(aos::FS::Dir(CONFIG_AOS_PROVISIONING_FILE));
        zassert_true(err.IsNone(), "Can't remove provisioning dir: %s", err.Message());

        delete static_cast<provisioning_fixture*>(fixture);
    });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(provisioning, test_FinishProvisioning)
{
    auto err = fixture->mProvisioning.Init();
    zassert_true(err.IsNone(), "Can't initialize provisioning: %s", err.Message());

    zassert_false(fixture->mProvisioning.IsProvisioned());

    err = fixture->mProvisioning.FinishProvisioning();
    zassert_true(err.IsNone(), "Can't finish provisioning: %s", err.Message());

    zassert_true(fixture->mProvisioning.IsProvisioned());

    err = fixture->mProvisioning.Init();
    zassert_true(err.IsNone(), "Can't initialize provisioning: %s", err.Message());

    zassert_true(fixture->mProvisioning.IsProvisioned());
}
