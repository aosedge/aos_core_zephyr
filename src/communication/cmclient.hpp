/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CMCLIENT_HPP_
#define CMCLIENT_HPP_

#include <aos/common/monitoring.hpp>
#include <aos/sm/launcher.hpp>

#include <proto/servicemanager/v3/servicemanager.pb.h>

#include "clocksync/clocksync.hpp"
#include "downloader/downloader.hpp"
#include "resourcemanager/resourcemanager.hpp"

#include "messagehandler.hpp"

/**
 * Number of SM PB services.
 */
constexpr auto cSMServicesCount = 1;

/**
 * CM client instance.
 */
class CMClient : public MessageHandler<servicemanager_v3_SMIncomingMessages_size,
                     servicemanager_v3_SMOutgoingMessages_size, cSMServicesCount> {
public:
    /**
     * Creates CM client.
     */
    CMClient() = default;

    /**
     * Initializes CM client instance.
     *
     * @param launcher launcher instance.
     * @param resourceManager resource manager instance.
     * @param resourceMonitor resource monitor instance.
     * @param downloader downloader instance.
     * @param clockSync clock sync instance.
     * @param messageSender message sender instance.
     * @return aos::Error.
     */
    aos::Error Init(aos::sm::launcher::LauncherItf& launcher, ResourceManagerItf& resourceManager,
        aos::monitoring::ResourceMonitorItf& resourceMonitor, DownloadReceiverItf& downloader, ClockSyncItf& clockSync,
        MessageSenderItf& messageSender);

    /**
     * Sends instances run status.
     *
     * @param instances instances status array.
     * @return Error.
     */
    aos::Error InstancesRunStatus(const aos::Array<aos::InstanceStatus>& instances);

    /**
     * Sends instances update status.
     * @param instances instances status array.
     *
     * @return Error.
     */
    aos::Error InstancesUpdateStatus(const aos::Array<aos::InstanceStatus>& instances);

    /**
     * Send image content request
     *
     * @param request image content request
     * @return Error.
     */
    aos::Error SendImageContentRequest(const ImageContentRequest& request);

    /**
     * Sends monitoring data
     *
     * @param monitoringData monitoring data
     * @return Error.
     */
    aos::Error SendMonitoringData(const aos::monitoring::NodeMonitoringData& monitoringData);

    /**
     * Sends node configuration.
     *
     * @return Error.
     */
    aos::Error SendNodeConfiguration();

    /**
     * Sends clock sync request.
     *
     * @return Error.
     */
    aos::Error SendClockSyncRequest();

private:
    static constexpr auto cMethodCount = 1;
    static constexpr auto cRunner      = "xrun";

    aos::Error ProcessMessage(Channel channel, PBServiceItf& service, const aos::String& methodName, uint64_t requestID,
        const aos::Array<uint8_t>& data) override;
    aos::Error ProcessGetUnitConfigStatus(Channel channel, uint64_t requestID);
    aos::Error ProcessCheckUnitConfig(
        Channel channel, uint64_t requestID, const servicemanager_v3_CheckUnitConfig& pbUnitConfig);
    aos::Error ProcessSetUnitConfig(
        Channel channel, uint64_t requestID, const servicemanager_v3_SetUnitConfig& pbUnitConfig);
    aos::Error ProcessRunInstances(const servicemanager_v3_RunInstances& pbRunInstances);
    aos::Error ProcessImageContentInfo(const servicemanager_v3_ImageContentInfo& pbContentInfo);
    aos::Error ProcessImageContent(const servicemanager_v3_ImageContent& pbContent);
    aos::Error ProcessClockSync(const servicemanager_v3_ClockSync& pbClockSync);

    aos::sm::launcher::LauncherItf*      mLauncher {};
    ResourceManagerItf*                  mResourceManager {};
    aos::monitoring::ResourceMonitorItf* mResourceMonitor {};
    DownloadReceiverItf*                 mDownloader {};
    ClockSyncItf*                        mClockSync {};

    aos::StaticAllocator<sizeof(servicemanager_v3_SMIncomingMessages) + sizeof(servicemanager_v3_SMOutgoingMessages)
            + aos::Max(sizeof(aos::ServiceInfoStaticArray) + sizeof(aos::LayerInfoStaticArray)
                    + sizeof(aos::InstanceInfoStaticArray),
                sizeof(ImageContentInfo) + sizeof(FileChunk)),
        sizeof(aos::monitoring::NodeInfo)>
        mAllocator;

    PBService<cMethodCount> mSMService;
};

#endif
