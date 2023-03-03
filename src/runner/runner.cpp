/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runner.hpp"
#include "log.hpp"

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

RunStatus Runner::StartInstance(const char* instanceID, const char* runtimeDir)
{
    (void)instanceID;
    (void)runtimeDir;

    return RunStatus {};
}

aos::Error Runner::StopInstance(const char* instanceID)
{
    (void)instanceID;

    return aos::ErrorEnum::eNone;
}
