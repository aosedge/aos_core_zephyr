/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xstat.h>

#include <sys/stat.h>
#include <zephyr/fs/fs.h>

#include "log.hpp"
#include "resourceusageprovider.hpp"

aos::Error ResourceUsageProvider::Init()
{
    LOG_DBG() << "Init resource usage provider";

    xenstat xstat {};

    int ret = xstat_getstat(&xstat);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mNodeInfo.mNodeID = cNodeID;
    mNodeInfo.mNumCPUs = xstat.num_cpus;
    mNodeInfo.mTotalRAM = xstat.tot_mem;

    aos::monitoring::PartitionInfo partitionInfo;
    partitionInfo.mName = cDiskPartitionName;
    partitionInfo.mPath = cDiskPartitionPoint;
    partitionInfo.mTypes.EmplaceBack("services");
    partitionInfo.mTypes.EmplaceBack("layers");

    mNodeInfo.mPartitions.PushBack(partitionInfo);

    LOG_DBG() << "Number of CPUs: " << mNodeInfo.mNumCPUs << ", total RAM(K): " << (mNodeInfo.mTotalRAM / 1024);

    for (size_t i = 0; i < mNodeInfo.mPartitions.Size(); ++i) {
        struct fs_statvfs sbuf;

        ret = fs_statvfs(mNodeInfo.mPartitions[i].mPath.CStr(), &sbuf);
        if (ret != 0) {
            return AOS_ERROR_WRAP(ret);
        }

        mNodeInfo.mPartitions[i].mTotalSize = sbuf.f_bsize * sbuf.f_blocks;

        LOG_DBG() << "Partition: " << mNodeInfo.mPartitions[i].mName
                  << ", total size(K): " << (mNodeInfo.mPartitions[i].mTotalSize / 1024);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error ResourceUsageProvider::GetNodeInfo(aos::monitoring::NodeInfo& nodeInfo) const
{
    LOG_DBG() << "Get node info";

    nodeInfo = mNodeInfo;

    return aos::ErrorEnum::eNone;
}

aos::Error ResourceUsageProvider::GetNodeMonitoringData(
    const aos::String& nodeID, aos::monitoring::MonitoringData& monitoringData)
{
    xenstat_domain domain;

    int ret = xstat_getdominfo(&domain, cDom0ID, 1);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    monitoringData.mRAM = domain.cur_mem;
    monitoringData.mCPU = 0;

    timeval curTime {};

    gettimeofday(&curTime, nullptr);

    if (mPrevNodeCPUTime != 0) {
        auto cpuTimeDiff_ns = domain.cpu_ns - mPrevNodeCPUTime;

        auto us_elapsed = (curTime.tv_sec - mPrevTime.tv_sec) * 1000000.0 + (curTime.tv_usec - mPrevTime.tv_usec);

        monitoringData.mCPU = ((cpuTimeDiff_ns / 10.0) / us_elapsed);
    }

    LOG_DBG() << "Get node monitoring data: "
              << "RAM(K): " << (monitoringData.mRAM / 1024) << ", CPU: " << monitoringData.mCPU;

    mPrevNodeCPUTime = domain.cpu_ns;
    mPrevTime = curTime;

    for (size_t i = 0; i < monitoringData.mDisk.Size(); ++i) {
        struct fs_statvfs sbuf;

        ret = fs_statvfs(monitoringData.mDisk[i].mPath.CStr(), &sbuf);
        if (ret != 0) {
            return AOS_ERROR_WRAP(ret);
        }

        monitoringData.mDisk[i].mUsedSize = (uint64_t)(sbuf.f_blocks - sbuf.f_bfree) * (uint64_t)sbuf.f_bsize;

        LOG_DBG() << "Disk: " << monitoringData.mDisk[i].mName
                  << ", used size(K): " << (monitoringData.mDisk[i].mUsedSize / 1024);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error ResourceUsageProvider::GetInstanceMonitoringData(
    const aos::String& instanceID, aos::monitoring::MonitoringData& monitoringData)
{
    LOG_DBG() << "Get monitoring data for instance: " << instanceID;

    int            max_domid = 0;
    xenstat_domain domain;

    while (true) {
        int ret = xstat_getdominfo(&domain, max_domid, 1);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        if (ret == 0) {
            break;
        }

        if (domain.id + 1 > max_domid) {
            max_domid = domain.id + 1;
        }

        if (instanceID != domain.name) {
            continue;
        }

        monitoringData.mRAM = domain.cur_mem;

        auto findInstanceCPUTime = mPrevInstanceCPUTime.Find([&instanceID](const InstanceCPUData& instanceCpuData) {
            return instanceCpuData.mInstanceID == instanceID;
        });

        timeval curTime {};
        gettimeofday(&curTime, nullptr);

        if (!findInstanceCPUTime.mError.IsNone()) {
            mPrevInstanceCPUTime.EmplaceBack(instanceID, domain.cpu_ns, curTime);
            monitoringData.mCPU = 0;

            break;
        }

        unsigned long long prevCPUTime = findInstanceCPUTime.mValue->mInstanceCPUTime;
        auto               prevTime = findInstanceCPUTime.mValue->mInstancePrevTime;

        findInstanceCPUTime.mValue->mInstanceCPUTime = domain.cpu_ns;
        findInstanceCPUTime.mValue->mInstancePrevTime = curTime;

        auto cpuTimeDiff = domain.cpu_ns - prevCPUTime;
        auto us_elapsed = (curTime.tv_sec - prevTime.tv_sec) * 1000000.0 + (curTime.tv_usec - prevTime.tv_usec);

        monitoringData.mCPU = ((cpuTimeDiff / 10.0) / us_elapsed);

        break;
    }

    LOG_DBG() << "RAM(K): " << (monitoringData.mRAM / 1024) << ", CPU: " << monitoringData.mCPU;

    return aos::ErrorEnum::eNone;
}
