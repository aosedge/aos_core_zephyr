/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NODEINFOPROVIDER_HPP_
#define NODEINFOPROVIDER_HPP_

#include <aos/common/tools/map.hpp>
#include <aos/common/tools/thread.hpp>
#include <aos/iam/nodeinfoprovider.hpp>

namespace aos::zephyr::nodeinfoprovider {

/**
 * Node info provider.
 */
class NodeInfoProvider : public iam::nodeinfoprovider::NodeInfoProviderItf {
public:
    /**
     * Initializes the node info provider.
     *
     * @return Error
     */
    Error Init();

    /**
     * Gets the node info object.
     *
     * @param[out] nodeInfo node info
     * @return Error
     */
    Error GetNodeInfo(NodeInfo& nodeInfo) const override;

    /**
     * Sets the node status.
     *
     * @param status node status
     * @return Error
     */
    Error SetNodeStatus(const NodeStatus& status) override;

    /**
     * Subscribes on node status changed event.
     *
     * @param observer node status changed observer
     * @return Error
     */
    Error SubscribeNodeStatusChanged(iam::nodeinfoprovider::NodeStatusObserverItf& observer) override;

    /**
     * Unsubscribes from node status changed event.
     *
     * @param observer node status changed observer
     * @return Error
     */
    Error UnsubscribeNodeStatusChanged(iam::nodeinfoprovider::NodeStatusObserverItf& observer) override;

private:
    static constexpr auto cNodeStatusLen            = 16;
    static constexpr auto cNodeName                 = CONFIG_AOS_NODE_NAME;
    static constexpr auto cDiskPartitionPoint       = CONFIG_AOS_DISK_MOUNT_POINT;
    static constexpr auto cMaxDMIPS                 = CONFIG_AOS_MAX_CPU_DMIPS;
    static constexpr auto cNodeType                 = CONFIG_AOS_NODE_TYPE;
    static constexpr auto cProvisioningStateFile    = CONFIG_AOS_PROVISION_STATE_FILE;
    static constexpr auto cDiskPartitionName        = "aos";
    static constexpr auto cNodeRunner               = "xrun";
    static constexpr auto cAosComponents            = "iam,sm";
    static constexpr auto cMaxNodeStatusSubscribers = 4;
#ifdef CONFIG_NATIVE_APPLICATION
    static constexpr auto cNodeIDFile = "/etc/machine-id";
#endif

    Error                    InitNodeID();
    Error                    InitAttributes();
    Error                    InitPartitionInfo();
    Error                    StoreNodeStatus(const NodeStatus& status) const;
    RetWithError<NodeStatus> ReadNodeStatus() const;
    Error                    NotifyNodeStatusChanged(const NodeStatus& status);

    NodeInfo                                                                                  mNodeInfo;
    StaticMap<iam::nodeinfoprovider::NodeStatusObserverItf*, bool, cMaxNodeStatusSubscribers> mStatusChangedSubscribers;
    mutable Mutex                                                                             mMutex;
};

} // namespace aos::zephyr::nodeinfoprovider

#endif
