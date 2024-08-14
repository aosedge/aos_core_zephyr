/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEUSAGEPROVIDER_HPP_
#define RESOURCEUSAGEPROVIDER_HPP_

#include <sys/time.h>

#include <aos/common/monitoring/monitoring.hpp>
#include <aos/iam/nodeinfoprovider.hpp>

namespace aos::zephyr::monitoring {

class ResourceUsageProvider : public aos::monitoring::ResourceUsageProviderItf {
public:
    /**
     * Initializes resource usage provider.
     *
     * @param nodeInfoProvider node info provider.
     * @return Error
     */
    Error Init(iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider);

    /**
     * Returns node monitoring data.
     *
     * @param nodeID node ident.
     * @param monitoringData monitoring data.
     * @return Error.
     */
    Error GetNodeMonitoringData(const String& nodeID, aos::monitoring::MonitoringData& monitoringData) override;

    /**
     * Returns instance monitoring data.
     *
     * @param instance instance ident.
     * @param monitoringData monitoring data.
     * @return Error.
     */
    Error GetInstanceMonitoringData(const String& instanceID, aos::monitoring::MonitoringData& monitoringData) override;

private:
    static constexpr auto cDom0ID = 0;

    struct InstanceCPUData {
        InstanceCPUData(const String& instanceID, unsigned long long instanceCPUTime, const timeval& instancePrevTime)
            : mInstanceCPUTime(instanceCPUTime)
            , mInstanceID(instanceID)
            , mInstancePrevTime(instancePrevTime)
        {
        }

        unsigned long long                mInstanceCPUTime;
        StaticString<aos::cInstanceIDLen> mInstanceID;
        timeval                           mInstancePrevTime;
    };

    unsigned long long                                  mPrevNodeCPUTime {};
    timeval                                             mPrevTime {};
    StaticArray<InstanceCPUData, aos::cMaxNumInstances> mPrevInstanceCPUTime {};
    Mutex                                               mMutex {};
    NodeInfo                                            mNodeInfo {};
};

} // namespace aos::zephyr::monitoring

#endif
