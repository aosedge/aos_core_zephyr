/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/stat.h>
#include <zephyr/drivers/hwinfo.h>
#ifdef CONFIG_NATIVE_APPLICATION
#include <sys/statvfs.h>
#else
#include <zephyr/fs/fs.h>
#endif

#include <xstat.h>

#include <aos/common/tools/fs.hpp>
#include <aos/common/tools/uuid.hpp>

#include "utils/utils.hpp"

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

    mNodeInfo.mName     = cNodeName;
    mNodeInfo.mTotalRAM = xstat.tot_mem;
    mNodeInfo.mNodeType = cNodeType;
    mNodeInfo.mMaxDMIPS = cMaxDMIPS;

    Error err;

    if (Tie(mNodeInfo.mStatus, err) = ReadNodeStatus(); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(err, "failed to get node status"));
    }

    if (err = InitAttributes(); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(err, "failed to init node attributes"));
    }

    if (err = InitPartitionInfo(); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(err, "failed to init node partition info"));
    }

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::GetNodeInfo(NodeInfo& nodeInfo) const
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Get node info: status=" << nodeInfo.mStatus;

    nodeInfo = mNodeInfo;

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::SetNodeStatus(const NodeStatus& status)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Set node status: status=" << status.ToString();

    if (status == mNodeInfo.mStatus) {
        return ErrorEnum::eNone;
    }

    if (auto err = StoreNodeStatus(status); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(err, "failed to store node status"));
    }

    mNodeInfo.mStatus = status;

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
    if (auto err = FS::ReadFileToString(cNodeIDFile, mNodeInfo.mNodeID); !err.IsNone()) {
        LOG_WRN() << "Failed to read node id: path=" << cNodeIDFile << ", err=" << err;
    } else if (mNodeInfo.mNodeID.IsEmpty()) {
        LOG_WRN() << "Node id is empty: path=" << cNodeIDFile;
    } else {
        mNodeInfo.mNodeID.Trim("\r\n");

        LOG_DBG() << "Read node id: id=" << mNodeInfo.mNodeID << ", path=" << cNodeIDFile;

        return ErrorEnum::eNone;
    }

    LOG_DBG() << "Generate node id";

    auto uuidStr = uuid::UUIDToString(uuid::CreateUUID());

    if (auto err = FS::WriteStringToFile(cNodeIDFile, uuidStr, S_IRUSR | S_IWUSR); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mNodeInfo.mNodeID = uuidStr;

    LOG_DBG() << "Store node id: id=" << uuidStr << ", path=" << cNodeIDFile;

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
#ifdef CONFIG_NATIVE_APPLICATION
        struct statvfs sbuf;

        if (auto ret = statvfs(partition.mPath.CStr(), &sbuf); ret != 0) {
            return ret;
        }
#else
        struct fs_statvfs sbuf;

        if (auto ret = fs_statvfs(partition.mPath.CStr(), &sbuf); ret != 0) {
            return ret;
        }
#endif
        partition.mTotalSize = sbuf.f_bsize * sbuf.f_blocks;

        LOG_DBG() << "Init partition info: name=" << partition.mName << ", totalSize=" << partition.mTotalSize;
    }

    return ErrorEnum::eNone;
}

Error NodeInfoProvider::StoreNodeStatus(const NodeStatus& status) const
{
    auto statusStr = status.ToString();

    if (auto err = FS::WriteStringToFile(cNodeStatusFile, statusStr, S_IRUSR | S_IWUSR); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

RetWithError<NodeStatus> NodeInfoProvider::ReadNodeStatus() const
{
    StaticString<cNodeStatusLen> statusStr;

    auto err = FS::ReadFileToString(cNodeStatusFile, statusStr);
    if (!err.IsNone()) {
        if (err == -ENOENT) {
            return {NodeStatusEnum::eUnprovisioned, ErrorEnum::eNone};
        }

        return {NodeStatusEnum::eUnprovisioned, AOS_ERROR_WRAP(err)};
    }

    if (statusStr.IsEmpty()) {
        return {NodeStatusEnum::eUnprovisioned, AOS_ERROR_WRAP(Error(ErrorEnum::eFailed, "node status is empty"))};
    }

    NodeStatus status;

    if (err = status.FromString(statusStr); !err.IsNone()) {
        return {NodeStatusEnum::eUnprovisioned, AOS_ERROR_WRAP(err)};
    }

    return status;
}

Error NodeInfoProvider::NotifyNodeStatusChanged(const NodeStatus& status)
{
    for (auto& [observer, _] : mStatusChangedSubscribers) {
        if (auto err = observer->OnNodeStatusChanged(mNodeInfo.mNodeID, status); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
    }

    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::nodeinfoprovider
