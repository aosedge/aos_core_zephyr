/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUNNER_HPP_
#define RUNNER_HPP_

#include <aos/sm/runner.hpp>

/**
 * Runner instance.
 */
class Runner : public aos::sm::runner::RunnerItf, private aos::NonCopyable {
public:
    /**
     * Creates runner instance.
     */
    Runner()
        : mStatusReceiver(nullptr)
    {
    }

    /**
     * Initializes runner instance.
     * @param launcher instance launcher.
     * @return aos::Error.
     */
    aos::Error Init(aos::sm::runner::RunStatusReceiverItf& statusReceiver);

    /**
     * Starts instance.
     *
     * @param instanceID instance ID.
     * @param runtimeDir directory with runtime spec.
     * @return RunStatus.
     */
    aos::sm::runner::RunStatus StartInstance(const char* instanceID, const char* runtimeDir) override;

    /**
     * Stops instance.
     *
     * @param instanceID instance ID>
     * @return Error.
     */
    aos::Error StopInstance(const char* instanceID) override;

private:
    aos::sm::runner::RunStatusReceiverItf* mStatusReceiver;
};

#endif
