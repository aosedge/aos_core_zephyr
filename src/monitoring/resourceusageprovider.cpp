/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xstat.h>

#include <sys/stat.h>
#ifdef CONFIG_NATIVE_APPLICATION
#include <sys/statvfs.h>
#else
#include <zephyr/fs/fs.h>
#endif

#include "log.hpp"
#include "resourceusageprovider.hpp"

namespace aos::zephyr::monitoring {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ResourceUsageProvider::Init()
{
    LOG_DBG() << "Init resource usage provider";

    return ErrorEnum::eNone;
}

Error ResourceUsageProvider::GetNodeMonitoringData(
    const String& nodeID, aos::monitoring::MonitoringData& monitoringData)
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

        monitoringData.mCPU = (cpuTimeDiff_ns * 100.0 / 1000.0 / us_elapsed);
    }

    LOG_DBG() << "Get node monitoring data: RAM(K): " << (monitoringData.mRAM / 1024)
              << ", CPU: " << monitoringData.mCPU;

    mPrevNodeCPUTime = domain.cpu_ns;
    mPrevTime        = curTime;

    for (size_t i = 0; i < monitoringData.mPartitions.Size(); ++i) {
#ifdef CONFIG_NATIVE_APPLICATION
        struct statvfs sbuf;

        if (ret = statvfs(monitoringData.mPartitions[i].mPath.CStr(), &sbuf); ret != 0) {
            return ret;
        }
#else
        struct fs_statvfs sbuf;

        if (ret = fs_statvfs(monitoringData.mPartitions[i].mPath.CStr(), &sbuf); ret != 0) {
            return AOS_ERROR_WRAP(ret);
        }
#endif
        monitoringData.mPartitions[i].mUsedSize = (uint64_t)(sbuf.f_blocks - sbuf.f_bfree) * (uint64_t)sbuf.f_frsize;

        LOG_DBG() << "Partition: " << monitoringData.mPartitions[i].mName
                  << ", used size(K): " << (monitoringData.mPartitions[i].mUsedSize / 1024);
    }

    return ErrorEnum::eNone;
}

Error ResourceUsageProvider::GetInstanceMonitoringData(
    const String& instanceID, aos::monitoring::InstanceMonitoringData& monitoringData)
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

        monitoringData.mMonitoringData.mRAM = domain.cur_mem;

        auto findInstanceCPUTime = mPrevInstanceCPUTime.FindIf([&instanceID](const InstanceCPUData& instanceCpuData) {
            return instanceCpuData.mInstanceID == instanceID;
        });

        timeval curTime {};
        gettimeofday(&curTime, nullptr);

        if (findInstanceCPUTime == mPrevInstanceCPUTime.end()) {
            if (auto err = mPrevInstanceCPUTime.EmplaceBack(instanceID, domain.cpu_ns, curTime);
                err != ErrorEnum::eNone) {
                return err;
            }

            monitoringData.mMonitoringData.mCPU = 0;

            break;
        }

        unsigned long long prevCPUTime = findInstanceCPUTime->mInstanceCPUTime;
        auto               prevTime    = findInstanceCPUTime->mInstancePrevTime;

        findInstanceCPUTime->mInstanceCPUTime  = domain.cpu_ns;
        findInstanceCPUTime->mInstancePrevTime = curTime;

        auto cpuTimeDiff = domain.cpu_ns - prevCPUTime;
        auto us_elapsed  = (curTime.tv_sec - prevTime.tv_sec) * 1000000.0 + (curTime.tv_usec - prevTime.tv_usec);

        monitoringData.mMonitoringData.mCPU = ((cpuTimeDiff / 10.0) / us_elapsed);

        break;
    }

    LOG_DBG() << "RAM(K): " << (monitoringData.mMonitoringData.mRAM / 1024)
              << ", CPU: " << monitoringData.mMonitoringData.mCPU;

    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::monitoring
