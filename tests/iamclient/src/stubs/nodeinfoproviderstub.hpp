/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NODEINFOPROVIDERSTUB_HPP_
#define NODEINFOPROVIDERSTUB_HPP_

#include <algorithm>
#include <mutex>
#include <vector>

#include <aos/iam/nodeinfoprovider.hpp>

class NodeInfoProviderStub : public aos::iam::nodeinfoprovider::NodeInfoProviderItf {
public:
    aos::Error GetNodeInfo(aos::NodeInfo& nodeInfo) const override
    {
        std::lock_guard lock {mMutex};

        nodeInfo = mNodeInfo;

        return aos::ErrorEnum::eNone;
    }

    aos::Error SetNodeStatus(const aos::NodeStatus& status) override
    {
        std::lock_guard lock {mMutex};

        if (mNodeInfo.mStatus == status) {
            return aos::ErrorEnum::eNone;
        }

        for (auto observer : mObservers) {
            observer->OnNodeStatusChanged(mNodeInfo.mNodeID, status);
        }

        mNodeInfo.mStatus = status;

        return aos::ErrorEnum::eNone;
    }

    aos::Error SubscribeNodeStatusChanged(aos::iam::nodeinfoprovider::NodeStatusObserverItf& observer) override
    {
        std::lock_guard lock {mMutex};

        if (std::find(mObservers.begin(), mObservers.end(), &observer) != mObservers.end()) {
            return aos::ErrorEnum::eAlreadyExist;
        }

        mObservers.push_back(&observer);

        return aos::ErrorEnum::eNone;
    }

    aos::Error UnsubscribeNodeStatusChanged(aos::iam::nodeinfoprovider::NodeStatusObserverItf& observer) override
    {
        std::lock_guard lock {mMutex};

        if (std::find(mObservers.begin(), mObservers.end(), &observer) == mObservers.end()) {
            return aos::ErrorEnum::eNotFound;
        }

        mObservers.erase(std::remove(mObservers.begin(), mObservers.end(), &observer), mObservers.end());

        return aos::ErrorEnum::eNone;
    }

    void SetNodeInfo(const aos::NodeInfo& nodeInfo) { mNodeInfo = nodeInfo; }

private:
    aos::NodeInfo                                                   mNodeInfo;
    mutable std::mutex                                              mMutex;
    std::vector<aos::iam::nodeinfoprovider::NodeStatusObserverItf*> mObservers;
};

#endif
