/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/fs.hpp>
#include <aos/common/tools/log.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/common/types.hpp>

#include "nodeinfoprovider/nodeinfoprovider.hpp"
#include "utils/log.hpp"

#define LVGL_PARTITION    storage_partition
#define LVGL_PARTITION_ID FIXED_PARTITION_ID(LVGL_PARTITION)

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cNodeStateFile = CONFIG_AOS_PROVISION_STATE_FILE;
static constexpr auto cUnprovisioned = "unprovisioned";

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static struct fs_mount_t sMnt = {
    .type        = FS_LITTLEFS,
    .mnt_point   = CONFIG_AOS_DISK_MOUNT_POINT,
    .fs_data     = &cstorage,
    .storage_dev = (void*)LVGL_PARTITION_ID,
};

static struct fs_statvfs sMntStat;

void* setup_fs(void)
{
    printk("setup_fs\n");

    int ret = fs_mount(&sMnt);
    if (ret < 0) {
        TC_PRINT("Failed to mount file system: %d\n", ret);

        ztest_test_fail();

        return NULL;
    }

    ret = fs_statvfs(CONFIG_AOS_DISK_MOUNT_POINT, &sMntStat);
    if (ret != 0) {
        printk("Failed to get statvfs: %d\n", ret);
    } else {
        printk("Mounted partition: %s\n", CONFIG_AOS_DISK_MOUNT_POINT);
        printk("bsize = %lu\n", sMntStat.f_bsize);
        printk("frsize = %lu\n", sMntStat.f_frsize);
        printk("blocks = %lu\n", sMntStat.f_blocks);
        printk("bfree = %lu\n", sMntStat.f_bfree);
    }

    return NULL;
}

void teardown_fs(void* data)
{
    printk("teardown_fs\n");

    auto ret = fs_unmount(&sMnt);
    if (ret < 0) {
        printk("Failed to unmount file system: %d\n", ret);
    }

    return;
}

void before_test(void* data)
{
    printk("before_test\n");

    auto err = aos::FS::WriteStringToFile(cNodeStateFile, cUnprovisioned, S_IRUSR | S_IWUSR);
    if (!err.IsNone()) {
        printk("Can't create provisioning state file: %s\n", err.Message());
    }
}

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

class TestNodeStatusObserver : public aos::iam::nodeinfoprovider::NodeStatusObserverItf {
public:
    aos::Error OnNodeStatusChanged(const aos::String& nodeID, const aos::NodeStatus& status) override
    {
        printk("Node status changed: %s\n", status.ToString().CStr());

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

ZTEST_SUITE(nodeinfoprovider, NULL, setup_fs, before_test, NULL, teardown_fs);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST(nodeinfoprovider, test_node_info)
{
    aos::Log::SetCallback(TestLogCallback);

    aos::zephyr::nodeinfoprovider::NodeInfoProvider provider;
    auto                                            err = provider.Init();
    zassert_true(err.IsNone(), "Failed to init node info provider: %s", err.Message());

    aos::NodeInfo nodeInfo;
    err = provider.GetNodeInfo(nodeInfo);

    zassert_true(err.IsNone(), "Failed to get node info: %s", err.Message());

    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::eUnprovisioned, "Node status mismatch");
    zassert_equal(nodeInfo.mMaxDMIPS, CONFIG_AOS_MAX_CPU_DMIPS, "Max DMIPS mismatch");

    // test partition info
    zassert_equal(nodeInfo.mPartitions.Size(), 1, "Expected 1 partition");
    zassert_equal(nodeInfo.mPartitions[0].mName, "Aos", "Partition name mismatch");
    zassert_equal(nodeInfo.mPartitions[0].mPath, CONFIG_AOS_DISK_MOUNT_POINT, "Partition path mismatch");

    const auto expectedSize = sMntStat.f_bsize * sMntStat.f_blocks;
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
    aos::Log::SetCallback(TestLogCallback);

    aos::zephyr::nodeinfoprovider::NodeInfoProvider provider;
    auto                                            err = provider.Init();
    zassert_true(err.IsNone(), "Failed to init node info provider: %s", err.Message());

    aos::NodeInfo nodeInfo;
    err = provider.GetNodeInfo(nodeInfo);

    zassert_true(err.IsNone(), "Failed to get node info: %s", err.Message());
    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::eUnprovisioned, "Node status mismatch");

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::eProvisioned));
    zassert_true(err.IsNone(), "Set node status failed: %s", err.Message());

    err = provider.GetNodeInfo(nodeInfo);
    zassert_true(err.IsNone(), "Get node status failed: %s", err.Message());

    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::eProvisioned, "Node status mismatch");

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::ePaused));
    zassert_true(err.IsNone(), "Set node status failed: %s", err.Message());

    err = provider.GetNodeInfo(nodeInfo);
    zassert_true(err.IsNone(), "Get node status failed: %s", err.Message());

    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::ePaused, "Node status mismatch");
}

ZTEST(nodeinfoprovider, test_node_status_changed_observer)
{
    aos::Log::SetCallback(TestLogCallback);

    aos::zephyr::nodeinfoprovider::NodeInfoProvider provider;
    auto                                            err = provider.Init();
    zassert_true(err.IsNone(), "Failed to init node info provider: %s", err.Message());

    aos::NodeInfo nodeInfo;
    err = provider.GetNodeInfo(nodeInfo);

    zassert_true(err.IsNone(), "Failed to get node info: %s", err.Message());
    zassert_equal(nodeInfo.mStatus.GetValue(), aos::NodeStatusEnum::eUnprovisioned, "Node status mismatch");

    TestNodeStatusObserver observer1, observer2;

    err = provider.SubscribeNodeStatusChanged(observer1);
    zassert_true(err.IsNone(), "Failed to subscribe on node status changed: %s", err.Message());

    err = provider.SubscribeNodeStatusChanged(observer2);
    zassert_true(err.IsNone(), "Failed to subscribe on node status changed: %s", err.Message());

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::eProvisioned));
    zassert_true(err.IsNone(), "Set node status failed: %s", err.Message());

    zassert_equal(observer1.mNodeStatus.GetValue(), aos::NodeStatusEnum::eProvisioned, "Node status mismatch");
    zassert_equal(observer1.mNodeID, nodeInfo.mNodeID, "Node id mismatch");

    zassert_equal(observer2.mNodeStatus.GetValue(), aos::NodeStatusEnum::eProvisioned, "Node status mismatch");
    zassert_equal(observer2.mNodeID, nodeInfo.mNodeID, "Node id mismatch");

    observer1.Reset();
    observer2.Reset();

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::ePaused));
    zassert_true(err.IsNone(), "Set node status failed: %s", err.Message());

    zassert_equal(observer1.mNodeStatus.GetValue(), aos::NodeStatusEnum::ePaused, "Node status mismatch");
    zassert_equal(observer1.mNodeID, nodeInfo.mNodeID, "Node id mismatch");

    zassert_equal(observer2.mNodeStatus.GetValue(), aos::NodeStatusEnum::ePaused, "Node status mismatch");
    zassert_equal(observer2.mNodeID, nodeInfo.mNodeID, "Node id mismatch");

    observer1.Reset();
    observer2.Reset();

    // same node status change should not trigger the observer
    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::ePaused));
    zassert_true(err.IsNone(), "Set node status failed: %s", err.Message());

    zassert_false(observer1.mNotified, "Observer should not be notified");
    zassert_false(observer2.mNotified, "Observer should not be notified");

    observer1.Reset();
    observer2.Reset();

    // unsubscribe observer1
    err = provider.UnsubscribeNodeStatusChanged(observer1);
    zassert_true(err.IsNone(), "Failed to unsubscribe from node status changed event: %s", err.Message());

    err = provider.SetNodeStatus(aos::NodeStatus(aos::NodeStatusEnum::eProvisioned));
    zassert_true(err.IsNone(), "Set node status failed: %s", err.Message());

    // observer1 should not receive the event
    zassert_false(observer1.mNotified, "Observer should not be notified");

    // observer2 should receive the event
    zassert_true(observer2.mNotified, "Observer should be notified");
    zassert_equal(observer2.mNodeStatus.GetValue(), aos::NodeStatusEnum::eProvisioned, "Node status mismatch");
}
