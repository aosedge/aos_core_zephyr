/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MONITORINGMOCK_HPP_
#define MONITORINGMOCK_HPP_

#include <aos/common/monitoring.hpp>

class ResourceMonitorMock : public aos::monitoring::ResourceMonitorItf {
public:
    aos::Error GetNodeInfo(aos::monitoring::NodeInfo& nodeInfo) const override
    {
        nodeInfo = mNodeInfo;
        return aos::ErrorEnum::eNone;
    }

    aos::Error StartInstanceMonitoring(
        const aos::String& instanceID, const aos::monitoring::InstanceMonitorParams& monitoringConfig)
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error StopInstanceMonitoring(const aos::String& instanceID) { return aos::ErrorEnum::eNone; }

    void SetNodeInfo(const aos::monitoring::NodeInfo& nodeInfo) { mNodeInfo = nodeInfo; }

private:
    aos::monitoring::NodeInfo mNodeInfo;
};

#endif
