/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEMONITORSTUB_HPP_
#define RESOURCEMONITORSTUB_HPP_

#include <aos/common/monitoring/monitoring.hpp>

class ResourceMonitorStub : public aos::monitoring::ResourceMonitorItf {
public:
    aos::Error StartInstanceMonitoring(
        const aos::String& instanceID, const aos::monitoring::InstanceMonitorParams& monitoringConfig)
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error StopInstanceMonitoring(const aos::String& instanceID) { return aos::ErrorEnum::eNone; }

    aos::Error GetAverageMonitoringData(aos::monitoring::NodeMonitoringData& monitoringData)
    {
        monitoringData = mMonitoringData;

        return aos::ErrorEnum::eNone;
    }

    void SetMonitoringData(const aos::monitoring::NodeMonitoringData& monitoringData)
    {
        mMonitoringData = monitoringData;
    }

private:
    aos::monitoring::NodeMonitoringData mMonitoringData;
};

#endif
