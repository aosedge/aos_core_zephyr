/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEUSAGEPROVIDER_HPP_
#define RESOURCEUSAGEPROVIDER_HPP_

#include <sys/time.h>

#include <aos/common/monitoring.hpp>

class ResourceUsageProvider : public aos::monitoring::ResourceUsageProviderItf {
public:
    /**
     * Init resource usage provider
     * @return Error
     */
    aos::Error Init() override;

    /**
     * Returns system info
     *
     * @param systemInfo system info
     * @return Error
     */
    aos::Error GetNodeInfo(aos::NodeInfo& systemInfo) const override;

    /**
     * Returns node monitoring data
     *
     * @param nodeID node ident
     * @param monitoringData monitoring data
     * @return Error
     */
    aos::Error GetNodeMonitoringData(
        const aos::String& nodeID, aos::monitoring::MonitoringData& monitoringData) override;

    /**
     * Returns instance monitoring data
     *
     * @param instance instance ident
     * @param monitoringData monitoring data
     * @return Error
     */
    aos::Error GetInstanceMonitoringData(
        const aos::String& instanceID, aos::monitoring::MonitoringData& monitoringData) override;

private:
    static constexpr auto cNodeID             = CONFIG_AOS_NODE_ID;
    static constexpr auto cDiskPartitionPoint = CONFIG_AOS_DISK_MOUNT_POINT;
    static constexpr auto cDiskPartitionName  = "Aos";
    static constexpr auto cDom0ID             = 0;

    struct InstanceCPUData {
        InstanceCPUData(
            const aos::String& instanceID, unsigned long long instanceCPUTime, const timeval& instancePrevTime)
            : mInstanceCPUTime(instanceCPUTime)
            , mInstanceID(instanceID)
            , mInstancePrevTime(instancePrevTime)
        {
        }

        unsigned long long                     mInstanceCPUTime;
        aos::StaticString<aos::cInstanceIDLen> mInstanceID;
        timeval                                mInstancePrevTime;
    };

    unsigned long long                                       mPrevNodeCPUTime {};
    timeval                                                  mPrevTime {};
    aos::StaticArray<InstanceCPUData, aos::cMaxNumInstances> mPrevInstanceCPUTime {};
    aos::Mutex                                               mMutex {};
    aos::NodeInfo                                            mNodeInfo {};
};

#endif
