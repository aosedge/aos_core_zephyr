/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/stat.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/fs/fs.h>

#include <xstat.h>

#include <aos/common/tools/fs.hpp>

#include "log.hpp"
#include "nodeinfoprovider.hpp"

namespace aos::zephyr::nodeinfoprovider {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error NodeInfoProvider::Init()
{
    LOG_DBG() << "Init node info provider";

    if (auto err = InitNodeID(); !err.IsNone() && !err.Is(ErrorEnum::eNotSupported)) {
        return Error(AOS_ERROR_WRAP(err), "failed to init node id");
    }

    xenstat xstat {};

    if (int ret = xstat_getstat(&xstat); ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mNodeInfo.mTotalRAM = xstat.tot_mem;
    mNodeInfo.mNodeType = cNodeType;
    mNodeInfo.mMaxDMIPS = cMaxDMIPS;

    if (auto err = InitAttributes(); !err.IsNone()) {
        return Error(AOS_ERROR_WRAP(err), "failed to init node attributes");
    }

    if (auto err = InitPartitionInfo(); !err.IsNone()) {
        return Error(AOS_ERROR_WRAP(err), "failed to init node partition info");
    }

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::GetNodeInfo(NodeInfo& nodeInfo) const
{
    NodeStatus status;

    if (auto err = ReadNodeStatus(status); !err.IsNone()) {
        LOG_ERR() << "Get node info failed: error=" << err;

        return AOS_ERROR_WRAP(err);
    }

    LOG_DBG() << "Get node info: status=" << status.ToString();

    nodeInfo         = mNodeInfo;
    nodeInfo.mStatus = status;

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::SetNodeStatus(const NodeStatus& status)
{
    LOG_DBG() << "Set node status: status=" << status.ToString();

    if (auto err = StoreNodeStatus(status); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(err, "failed to store node status"));
    }

    if (status == mNodeInfo.mStatus) {
        return ErrorEnum::eNone;
    }

    {
        LockGuard lock {mMutex};

        mNodeInfo.mStatus = status;
    }

    LOG_DBG() << "Node status updated: status=" << mNodeInfo.mStatus.ToString();

    if (auto err = NotifyNodeStatusChanged(status); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(err, "failed to notify node status observers"));
    }

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::SubscribeNodeStatusChanged(iam::nodeinfoprovider::NodeStatusObserverItf& observer)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Subscribe on node status changed event";

    if (auto err = mStatusChangedSubscribers.Set(&observer, true); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::UnsubscribeNodeStatusChanged(iam::nodeinfoprovider::NodeStatusObserverItf& observer)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Unsubscribe from node status changed event";

    if (auto err = mStatusChangedSubscribers.Remove(&observer); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error NodeInfoProvider::InitNodeID()
{
    uint8_t buffer[cNodeIDLen];
    memset(buffer, 0, sizeof(buffer));

    auto ret = hwinfo_get_device_id(buffer, sizeof(buffer) - 1);

    if (ret == -ENOSYS) {
        LOG_WRN() << "hwinfo_get_device_id is not supported";

        return ErrorEnum::eNotSupported;
    } else if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mNodeInfo.mNodeID = String(reinterpret_cast<char*>(buffer), ret);

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::InitAttributes()
{
    if (auto err = mNodeInfo.mAttrs.PushBack({iam::nodeinfoprovider::cAttrAosComponents, cAosComponents});
        !err.IsNone()) {
        return err;
    }

    if (auto err = mNodeInfo.mAttrs.PushBack({iam::nodeinfoprovider::cAttrNodeRunners, cNodeRunner}); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::InitPartitionInfo()
{
    LOG_DBG() << "Init partition info";

    PartitionInfo partitionInfo;

    partitionInfo.mName = cDiskPartitionName;
    partitionInfo.mPath = cDiskPartitionPoint;
    partitionInfo.mTypes.EmplaceBack("services");
    partitionInfo.mTypes.EmplaceBack("layers");

    if (auto err = mNodeInfo.mPartitions.PushBack(partitionInfo); !err.IsNone()) {
        return err;
    }

    for (auto& partition : mNodeInfo.mPartitions) {
        struct fs_statvfs sbuf;

        if (auto ret = fs_statvfs(partition.mPath.CStr(), &sbuf); ret != 0) {
            return ret;
        }

        partition.mTotalSize = sbuf.f_bsize * sbuf.f_blocks;

        LOG_DBG() << "Init partition info: name=" << partition.mName << ", size=" << partition.mTotalSize;
    }

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::StoreNodeStatus(const NodeStatus& status) const
{
    auto statusStr = status.ToString();

    if (auto err = FS::WriteStringToFile(cProvisioningStateFile, statusStr, S_IRUSR | S_IWUSR); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::ReadNodeStatus(NodeStatus& status) const
{
    StaticString<cNodeStatusLen> statusStr;

    auto err = FS::ReadFileToString(cProvisioningStateFile, statusStr);
    if (!err.IsNone()) {
        return err;
    }

    if (statusStr.IsEmpty()) {
        return ErrorEnum::eFailed;
    }

    return status.FromString(statusStr);
}

Error NodeInfoProvider::NotifyNodeStatusChanged(const NodeStatus& status)
{
    LockGuard lock {mMutex};

    for (auto& [observer, _] : mStatusChangedSubscribers) {
        if (auto err = observer->OnNodeStatusChanged(mNodeInfo.mNodeID, status); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
    }

    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::nodeinfoprovider
