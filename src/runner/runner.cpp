/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xrun.h>

#include "log.hpp"
#include "runner.hpp"

namespace aos::zephyr::runner {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error Runner::Init(sm::runner::RunStatusReceiverItf& statusReceiver)
{
    LOG_DBG() << "Initialize runner";

    mStatusReceiver = &statusReceiver;

    return ErrorEnum::eNone;
}

sm::runner::RunStatus Runner::StartInstance(
    const String& instanceID, const String& runtimeDir, const RunParameters& runParams)
{
    sm::runner::RunStatus runStatus {instanceID, InstanceRunStateEnum::eActive, ErrorEnum::eNone};

    auto ret = xrun_run(runtimeDir.CStr(), cConsoleSocket, instanceID.CStr());
    if (ret != 0) {
        runStatus.mState = InstanceRunStateEnum::eFailed;
        runStatus.mError = AOS_ERROR_WRAP(ret);
    }

    if (auto err = mConsoleReader.Subscribe(instanceID); !err.IsNone()) {
        LOG_WRN() << "Can't subscribe instance console" << Log::Field("instanceID", instanceID) << Log::Field(err);
    }

    return runStatus;
}

Error Runner::StopInstance(const String& instanceID)
{
    if (auto err = mConsoleReader.Unsubscribe(instanceID); !err.IsNone()) {
        LOG_WRN() << "Can't unsubscribe instance console" << Log::Field("instanceID", instanceID) << Log::Field(err);
    }

    auto ret = xrun_kill(instanceID.CStr());
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::runner
