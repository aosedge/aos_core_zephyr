/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NODEINFOPROVIDERSTUB_HPP_
#define NODEINFOPROVIDERSTUB_HPP_

#include <aos/iam/nodeinfoprovider.hpp>

class NodeInfoProviderStub : public aos::iam::nodeinfoprovider::NodeInfoProviderItf {
public:
    aos::Error GetNodeInfo(aos::NodeInfo& nodeInfo) const override { return aos::ErrorEnum::eNone; }

    aos::Error SetNodeStatus(const aos::NodeStatus& status) override { return aos::ErrorEnum::eNone; }

    aos::Error SubscribeNodeStatusChanged(aos::iam::nodeinfoprovider::NodeStatusObserverItf& observer) override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error UnsubscribeNodeStatusChanged(aos::iam::nodeinfoprovider::NodeStatusObserverItf& observer) override
    {
        return aos::ErrorEnum::eNone;
    }
};

#endif
