/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/statvfs.h>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/fs.hpp>
#include <aos/common/tools/log.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/common/types.hpp>

#include "nodeinfoprovider/nodeinfoprovider.hpp"
#include "utils/log.hpp"
#include "utils/utils.hpp"

using namespace aos::zephyr;

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cNodeStateFile = CONFIG_AOS_PROVISION_STATE_FILE;
static constexpr auto cUnprovisioned = "unprovisioned";

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

void* setup(void)
{
    aos::Log::SetCallback(TestLogCallback);

    auto err = aos::FS::ClearDir(CONFIG_AOS_DISK_MOUNT_POINT);
    zassert_true(err.IsNone(), "Failed to clear disk mount point: %s", utils::ErrorToCStr(err));

    return NULL;
}

void teardown(void* data)
{
    auto err = aos::FS::RemoveAll(CONFIG_AOS_DISK_MOUNT_POINT);
    zassert_true(err.IsNone(), "Failed to remove disk mount point: %s", utils::ErrorToCStr(err));
}

void before_test(void* data)
{
    auto err = aos::FS::WriteStringToFile(cNodeStateFile, cUnprovisioned, S_IRUSR | S_IWUSR);
    zassert_true(err.IsNone(), "Failed to create provisioning state file: %s", utils::ErrorToCStr(err));
}

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

class TestNodeStatusObserver : public aos::iam::nodeinfoprovider::NodeStatusObserverItf {
public:
    aos::Error OnNodeStatusChanged(const aos::String& nodeID, const aos::NodeStatus& status) override
    {
        mNodeID     = nodeID;
        mNodeStatus = status;
        mNotified   = true;

        return aos::ErrorEnum::eNone;
    }

    void Reset()
    {
        mNotified = false;
        mNodeID.Clear();
        mNodeStatus = aos::NodeStatus();
    }

    aos::StaticString<aos::cNodeIDLen> mNodeID;
    aos::NodeStatus                    mNodeStatus;
    bool                               mNotified = false;
};

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(nodeinfoprovider, nullptr, setup, before_test, nullptr, teardown);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST(nodeinfoprovider, test_node_info)
{
    aos::zephyr::nodeinfoprovider::NodeInfoProvider provider;

    auto err = provider.Init();
    zassert_true(err.IsNone(), "Failed to init node info provider: %s", utils::ErrorToCStr(err));

    aos::NodeInfo nodeInfo;
    err = provider.GetNodeInfo(nodeInfo);

    zassert_true(err.IsNone(), "Failed to get node info: %s", utils::ErrorToCStr(err));

    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::eUnprovisioned, "Node status mismatch");
    zassert_equal(nodeInfo.mMaxDMIPS, CONFIG_AOS_MAX_CPU_DMIPS, "Max DMIPS mismatch");

    // test partition info
    zassert_equal(nodeInfo.mPartitions.Size(), 1, "Expected 1 partition");
    zassert_equal(nodeInfo.mPartitions[0].mName, "aos", "Partition name mismatch");
    zassert_equal(nodeInfo.mPartitions[0].mPath, CONFIG_AOS_DISK_MOUNT_POINT, "Partition path mismatch");

    struct statvfs sbuf;

    auto ret = statvfs(CONFIG_AOS_DISK_MOUNT_POINT, &sbuf);
    zassert_equal(ret, 0, "Failed to get partition info: %s [%d]", strerror(ret), ret);

    const auto expectedSize = sbuf.f_bsize * sbuf.f_blocks;
    zassert_equal(nodeInfo.mPartitions[0].mTotalSize, expectedSize, "Partition total size mismatch");

    zassert_equal(nodeInfo.mPartitions[0].mTypes.Size(), 2, "Expected 2 partition types");
    zassert_equal(nodeInfo.mPartitions[0].mTypes[0], "services", "Partition type mismatch");
    zassert_equal(nodeInfo.mPartitions[0].mTypes[1], "layers", "Partition type mismatch");

    // test attributes
    zassert_equal(nodeInfo.mAttrs.Size(), 2, "Expected 2 attributes");
    zassert_equal(nodeInfo.mAttrs[0].mName, "AosComponents", "Attribute name mismatch");
    zassert_equal(nodeInfo.mAttrs[0].mValue, "iam,sm", "Attribute value mismatch");

    zassert_equal(nodeInfo.mAttrs[1].mName, "NodeRunners", "Attribute name mismatch");
    zassert_equal(nodeInfo.mAttrs[1].mValue, "xrun", "Attribute value mismatch");
}

ZTEST(nodeinfoprovider, test_set_get_node_info)
{
    aos::zephyr::nodeinfoprovider::NodeInfoProvider provider;
    auto                                            err = provider.Init();
    zassert_true(err.IsNone(), "Failed to init node info provider: %s", utils::ErrorToCStr(err));

    aos::NodeInfo nodeInfo;
    err = provider.GetNodeInfo(nodeInfo);

    zassert_true(err.IsNone(), "Failed to get node info: %s", utils::ErrorToCStr(err));
    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::eUnprovisioned, "Node status mismatch");

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::eProvisioned));
    zassert_true(err.IsNone(), "Set node status failed: %s", utils::ErrorToCStr(err));

    err = provider.GetNodeInfo(nodeInfo);
    zassert_true(err.IsNone(), "Get node status failed: %s", utils::ErrorToCStr(err));

    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::eProvisioned, "Node status mismatch");

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::ePaused));
    zassert_true(err.IsNone(), "Set node status failed: %s", utils::ErrorToCStr(err));

    err = provider.GetNodeInfo(nodeInfo);
    zassert_true(err.IsNone(), "Get node status failed: %s", utils::ErrorToCStr(err));

    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::ePaused, "Node status mismatch");
}

ZTEST(nodeinfoprovider, test_node_status_changed_observer)
{
    aos::zephyr::nodeinfoprovider::NodeInfoProvider provider;
    auto                                            err = provider.Init();
    zassert_true(err.IsNone(), "Failed to init node info provider: %s", utils::ErrorToCStr(err));

    aos::NodeInfo nodeInfo;
    err = provider.GetNodeInfo(nodeInfo);

    zassert_true(err.IsNone(), "Failed to get node info: %s", utils::ErrorToCStr(err));
    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::eUnprovisioned, "Node status mismatch");

    TestNodeStatusObserver observer1, observer2;

    err = provider.SubscribeNodeStatusChanged(observer1);
    zassert_true(err.IsNone(), "Failed to subscribe on node status changed: %s", utils::ErrorToCStr(err));

    err = provider.SubscribeNodeStatusChanged(observer2);
    zassert_true(err.IsNone(), "Failed to subscribe on node status changed: %s", utils::ErrorToCStr(err));

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::eProvisioned));
    zassert_true(err.IsNone(), "Set node status failed: %s", utils::ErrorToCStr(err));

    zassert_equal(observer1.mNodeStatus.GetValue(), aos::NodeStatusEnum::eProvisioned, "Node status mismatch");
    zassert_equal(observer1.mNodeID, nodeInfo.mNodeID, "Node id mismatch");

    zassert_equal(observer2.mNodeStatus.GetValue(), aos::NodeStatusEnum::eProvisioned, "Node status mismatch");
    zassert_equal(observer2.mNodeID, nodeInfo.mNodeID, "Node id mismatch");

    observer1.Reset();
    observer2.Reset();

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::ePaused));
    zassert_true(err.IsNone(), "Set node status failed: %s", utils::ErrorToCStr(err));

    zassert_equal(observer1.mNodeStatus.GetValue(), aos::NodeStatusEnum::ePaused, "Node status mismatch");
    zassert_equal(observer1.mNodeID, nodeInfo.mNodeID, "Node id mismatch");

    zassert_equal(observer2.mNodeStatus.GetValue(), aos::NodeStatusEnum::ePaused, "Node status mismatch");
    zassert_equal(observer2.mNodeID, nodeInfo.mNodeID, "Node id mismatch");

    observer1.Reset();
    observer2.Reset();

    // same node status change should not trigger the observer
    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::ePaused));
    zassert_true(err.IsNone(), "Set node status failed: %s", utils::ErrorToCStr(err));

    zassert_false(observer1.mNotified, "Observer should not be notified");
    zassert_false(observer2.mNotified, "Observer should not be notified");

    observer1.Reset();
    observer2.Reset();

    // unsubscribe observer1
    err = provider.UnsubscribeNodeStatusChanged(observer1);
    zassert_true(err.IsNone(), "Failed to unsubscribe from node status changed event: %s", utils::ErrorToCStr(err));

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::eProvisioned));
    zassert_true(err.IsNone(), "Set node status failed: %s", utils::ErrorToCStr(err));

    // observer1 should not receive the event
    zassert_false(observer1.mNotified, "Observer should not be notified");

    // observer2 should receive the event
    zassert_true(observer2.mNotified, "Observer should be notified");
    zassert_equal(observer2.mNodeStatus.GetValue(), aos::NodeStatusEnum::eProvisioned, "Node status mismatch");
}
