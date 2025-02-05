/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MONITORINGSTUB_HPP_
#define MONITORINGSTUB_HPP_

#include <aos/common/monitoring.hpp>

class ResourceMonitorStub : public aos::monitoring::ResourceMonitorItf {
public:
    aos::Error GetNodeInfo(aos::NodeInfo& nodeInfo) const override
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

    void SetNodeInfo(const aos::NodeInfo& nodeInfo) { mNodeInfo = nodeInfo; }

    void Clear() { mNodeInfo = aos::NodeInfo(); }

private:
    aos::NodeInfo mNodeInfo;
};

#endif
