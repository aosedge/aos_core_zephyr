/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEMONITORSTUB_HPP_
#define RESOURCEMONITORSTUB_HPP_

#include <aos/common/monitoring/monitoring.hpp>

namespace aos::zephyr {

class ResourceMonitorStub : public aos::monitoring::ResourceMonitorItf {
public:
    /**
     * Starts instance monitoring.
     *
     * @param instanceID instance ID.
     * @param monitoringConfig monitoring config.
     * @return Error.
     */
    Error StartInstanceMonitoring(
        const String& instanceID, const aos::monitoring::InstanceMonitorParams& monitoringConfig) override
    {
        (void)instanceID;
        (void)monitoringConfig;

        return ErrorEnum::eNone;
    }

    /**
     * Updates instance's run state.
     *
     * @param instanceID instance ID.
     * @param runState run state.
     * @return Error.
     */
    Error UpdateInstanceRunState(const String& instanceID, InstanceRunState runState) override
    {
        (void)instanceID;
        (void)runState;

        return ErrorEnum::eNone;
    }

    /**
     * Stops instance monitoring.
     *
     * @param instanceID instance ID.
     * @return Error.
     */
    Error StopInstanceMonitoring(const String& instanceID) override
    {
        (void)instanceID;

        return ErrorEnum::eNone;
    }

    /**
     * Returns average monitoring data.
     *
     * @param[out] monitoringData monitoring data.
     * @return Error.
     */
    Error GetAverageMonitoringData(aos::monitoring::NodeMonitoringData& monitoringData) override
    {
        monitoringData = mMonitoringData;

        return ErrorEnum::eNone;
    }

    /**
     * Sets monitoring data.
     *
     * @param monitoringData monitoring data.
     */
    void SetMonitoringData(const aos::monitoring::NodeMonitoringData& monitoringData)
    {
        mMonitoringData = monitoringData;
    }

private:
    aos::monitoring::NodeMonitoringData mMonitoringData;
};

} // namespace aos::zephyr

#endif
