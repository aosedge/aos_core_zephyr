
/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mockresourcemonitor.hpp"

Error MockResourceMonitor::GetNodeInfo(monitoring::NodeInfo& nodeInfo) const
{
    return ErrorEnum::eNone;
}

Error MockResourceMonitor::StartInstanceMonitoring(
    const String& instanceID, const monitoring::InstanceMonitorParams& monitoringConfig)
{
    return ErrorEnum::eNone;
}

Error MockResourceMonitor::StopInstanceMonitoring(const String& instanceID)
{
    return ErrorEnum::eNone;
}
