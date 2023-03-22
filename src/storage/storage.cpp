/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error Storage::Init()
{
    LOG_DBG() << "Initialize storage";

    return aos::ErrorEnum::eNone;
}

aos::Error Storage::AddInstance(const aos::InstanceInfo& instance)
{
    return aos::ErrorEnum::eNone;
}

aos::Error Storage::UpdateInstance(const aos::InstanceInfo& instance)
{
    return aos::ErrorEnum::eNone;
}

aos::Error Storage::RemoveInstance(const aos::InstanceIdent& instanceIdent)
{
    return aos::ErrorEnum::eNone;
}

aos::Error Storage::GetAllInstances(aos::Array<aos::InstanceInfo>& instances)
{
    return aos::ErrorEnum::eNone;
}
