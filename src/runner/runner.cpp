/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xrun.h>

#include "log.hpp"
#include "runner.hpp"

using namespace aos::sm::runner;

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error Runner::Init(RunStatusReceiverItf& statusReceiver)
{
    LOG_DBG() << "Initialize runner";

    mStatusReceiver = &statusReceiver;

    return aos::ErrorEnum::eNone;
}

RunStatus Runner::StartInstance(const aos::String& instanceID, const aos::String& runtimeDir)
{
    RunStatus runStatus {instanceID, aos::InstanceRunStateEnum::eActive, aos::ErrorEnum::eNone};

    auto ret = xrun_run(runtimeDir.CStr(), cConsoleSocket, instanceID.CStr());
    if (ret != 0) {
        runStatus.mState = aos::InstanceRunStateEnum::eFailed;
        runStatus.mError = AOS_ERROR_WRAP(ret);
    }

    return runStatus;
}

aos::Error Runner::StopInstance(const aos::String& instanceID)
{
    auto ret = xrun_kill(instanceID.CStr());
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    return aos::ErrorEnum::eNone;
}
