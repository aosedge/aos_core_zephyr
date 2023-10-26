/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aos/common/resourcemonitor.hpp"

using namespace aos;

class MockResourceMonitor : public monitoring::ResourceMonitorItf {
public:
    /**
     * Gets the node info object
     *
     * @param nodeInfo node info
     * @return Error
     */
    Error GetNodeInfo(monitoring::NodeInfo& nodeInfo) const override;

    /**
     * Starts instance monitoring
     *
     * @param instanceID instance ident
     * @param monitoringConfig monitoring config
     * @return Error
     */
    Error StartInstanceMonitoring(
        const String& instanceID, const monitoring::InstanceMonitorParams& monitoringConfig) override;

    /**
     * Stops instance monitoring
     *
     * @param instanceID instance ident
     * @return Error
     */
    Error StopInstanceMonitoring(const String& instanceID) override;
};
