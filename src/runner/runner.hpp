/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUNNER_HPP_
#define RUNNER_HPP_

#include <aos/sm/runner.hpp>

namespace aos::zephyr::runner {

/**
 * Runner instance.
 */
class Runner : public sm::runner::RunnerItf, private NonCopyable {
public:
    /**
     * Initializes runner instance.
     * @param launcher instance launcher.
     * @return Error.
     */
    Error Init(sm::runner::RunStatusReceiverItf& statusReceiver);

    /**
     * Starts instance.
     *
     * @param instanceID instance ID.
     * @param runtimeDir directory with runtime spec.
     * @param runParams runtime parameters.
     * @return RunStatus.
     */
    sm::runner::RunStatus StartInstance(
        const String& instanceID, const String& runtimeDir, const RunParameters& runParams) override;

    /**
     * Stops instance.
     *
     * @param instanceID instance ID>
     * @return Error.
     */
    Error StopInstance(const String& instanceID) override;

private:
    static constexpr int cConsoleSocket = 0;

    sm::runner::RunStatusReceiverItf* mStatusReceiver {};
};

} // namespace aos::zephyr::runner

#endif
