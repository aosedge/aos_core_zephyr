/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NODEINFOPROVIDER_HPP_
#define NODEINFOPROVIDER_HPP_

#include <aos/iam/nodeinfoprovider.hpp>

/**
 * Node info provider.
 */
class NodeInfoProvider : public aos::iam::nodeinfoprovider::NodeInfoProviderItf {
public:
    /**
     * Initializes the node info provider.
     *
     * @return Error
     */
    aos::Error Init();

    /**
     * Gets the node info object.
     *
     * @param[out] nodeInfo node info
     * @return Error
     */
    aos::Error GetNodeInfo(aos::NodeInfo& nodeInfo) const override;

    /**
     * Sets the node status.
     *
     * @param status node status
     * @return Error
     */
    aos::Error SetNodeStatus(const aos::NodeStatus& status) override;

private:
    aos::Error InitNodeID();
    aos::Error InitAttributes();
    aos::Error InitPartitionInfo();
    aos::Error StoreNodeStatus(const aos::NodeStatus& status) const;
    aos::Error ReadNodeStatus(aos::NodeStatus& status) const;

    static constexpr auto cDiskPartitionPoint    = CONFIG_AOS_DISK_MOUNT_POINT;
    static constexpr auto cMaxDMIPS              = CONFIG_AOS_MAX_CPU_DMIPS;
    static constexpr auto cNodeType              = CONFIG_AOS_NODE_TYPE;
    static constexpr auto cProvisioningStateFile = CONFIG_AOS_PROVISION_STATE_FILE;
    static constexpr auto cDiskPartitionName     = "Aos";
    static constexpr auto cNodeRunner            = "xrun";
    static constexpr auto cAosComponents         = "iam,sm";

    aos::NodeInfo mNodeInfo;
};

#endif
