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

// cppcheck-suppress unusedFunction
sm::runner::RunStatus Runner::StartInstance(const String& instanceID, const String& runtimeDir)
{
    sm::runner::RunStatus runStatus {instanceID, InstanceRunStateEnum::eActive, ErrorEnum::eNone};

    auto ret = xrun_run(runtimeDir.CStr(), cConsoleSocket, instanceID.CStr());
    if (ret != 0) {
        runStatus.mState = InstanceRunStateEnum::eFailed;
        runStatus.mError = AOS_ERROR_WRAP(ret);
    }

    return runStatus;
}

// cppcheck-suppress unusedFunction
Error Runner::StopInstance(const String& instanceID)
{
    auto ret = xrun_kill(instanceID.CStr());
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::runner
