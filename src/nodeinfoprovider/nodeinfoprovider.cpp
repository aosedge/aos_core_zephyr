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

#include "log.hpp"
#include "nodeinfoprovider.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error NodeInfoProvider::Init()
{
    LOG_DBG() << "Init node info provider";

    if (auto err = InitNodeID(); !err.IsNone() && !err.Is(aos::ErrorEnum::eNotSupported)) {
        return aos::Error(AOS_ERROR_WRAP(err), "failed to init node id");
    }

    xenstat xstat {};

    if (int ret = xstat_getstat(&xstat); ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mNodeInfo.mTotalRAM = xstat.tot_mem;
    mNodeInfo.mNodeType = cNodeType;
    mNodeInfo.mMaxDMIPS = cMaxDMIPS;

    if (auto err = InitAttributes(); !err.IsNone()) {
        aos::Error(AOS_ERROR_WRAP(err), "failed to init node attributes");
    }

    if (auto err = InitPartitionInfo(); !err.IsNone()) {
        aos::Error(AOS_ERROR_WRAP(err), "failed to init node partition info");
    }

    return aos::ErrorEnum::eNone;
}

aos::Error NodeInfoProvider::GetNodeInfo(aos::NodeInfo& nodeInfo) const
{
    aos::NodeStatus status;

    if (auto err = ReadNodeStatus(status); !err.IsNone() && !err.Is(aos::ErrorEnum::eNotFound)) {
        LOG_ERR() << "Get node info failed: error=" << err;

        aos::Error(AOS_ERROR_WRAP(err), "failed to read node status");
    }

    LOG_DBG() << "Get node info: status=" << status.ToString();

    nodeInfo         = mNodeInfo;
    nodeInfo.mStatus = status;

    return aos::ErrorEnum::eNone;
}

aos::Error NodeInfoProvider::SetNodeStatus(const aos::NodeStatus& status)
{
    LOG_DBG() << "Set node status: status=" << status.ToString();

    if (auto err = StoreNodeStatus(status); !err.IsNone()) {
        aos::Error(AOS_ERROR_WRAP(err), "failed to store node status");
    }

    LOG_DBG() << "Node status updated: status=" << status.ToString();

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::Error NodeInfoProvider::InitNodeID()
{
    uint8_t buffer[aos::cNodeIDLen];
    memset(buffer, 0, sizeof(buffer));

    auto ret = hwinfo_get_device_id(buffer, sizeof(buffer) - 1);

    if (ret == -ENOSYS) {
        LOG_WRN() << "hwinfo_get_device_id is not supported";

        return aos::ErrorEnum::eNotSupported;
    } else if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mNodeInfo.mNodeID = aos::String(reinterpret_cast<char*>(buffer), ret);

    return aos::ErrorEnum::eNone;
}

aos::Error NodeInfoProvider::InitAttributes()
{
    if (auto err = mNodeInfo.mAttrs.PushBack({aos::iam::nodeinfoprovider::cAttrAosComponents, cAosComponents});
        !err.IsNone()) {
        return err;
    }

    if (auto err = mNodeInfo.mAttrs.PushBack({aos::iam::nodeinfoprovider::cAttrNodeRunners, cNodeRunner});
        !err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error NodeInfoProvider::InitPartitionInfo()
{
    LOG_DBG() << "Init partition info";

    aos::PartitionInfo partitionInfo;

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

    return aos::ErrorEnum::eNone;
}

aos::Error NodeInfoProvider::StoreNodeStatus(const aos::NodeStatus& status) const
{
    auto statusStr = status.ToString();

    auto fd = open(cProvisioningStateFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return aos::Error(errno, "failed to open provisioning state file");
    }

    aos::Error err = aos::ErrorEnum::eNone;

    ssize_t nwrite = write(fd, statusStr.Get(), statusStr.Size());
    if (nwrite != static_cast<ssize_t>(statusStr.Size())) {
        err = nwrite < 0 ? AOS_ERROR_WRAP(errno) : aos::ErrorEnum::eRuntime;
    }

    close(fd);

    return err;
}

aos::Error NodeInfoProvider::ReadNodeStatus(aos::NodeStatus& status) const
{
    auto fd = open(cProvisioningStateFile, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            return aos::ErrorEnum::eNotFound;
        }

        return AOS_ERROR_WRAP(errno);
    }

    char buffer[64];

    aos::Error err = aos::ErrorEnum::eNone;

    ssize_t nread = read(fd, buffer, sizeof(buffer));
    if (nread < 0) {
        err = aos::Error(errno, "failed to read provisioning state file");
    } else if (nread == 0) {
        status = aos::NodeStatus();
    } else {
        err = status.FromString(aos::String(buffer, nread));
    }

    close(fd);

    return err;
}
