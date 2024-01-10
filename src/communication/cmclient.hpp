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

#include "downloader/downloader.hpp"
#include "resourcemanager/resourcemanager.hpp"

#include "messagesender.hpp"

/**
 * CM client instance.
 */
class CMClient {
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
     * @param messageSender message sender instance.
     * @return aos::Error.
     */
    aos::Error Init(aos::sm::launcher::LauncherItf& launcher, ResourceManagerItf& resourceManager,
        aos::monitoring::ResourceMonitorItf& resourceMonitor, DownloadReceiverItf& downloader,
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
     * Processes received message.
     *
     * @param channel communication channel on which message is received.
     * @param methodName protocol method name.
     * @param requestID protocol request ID.
     * @param data raw message data.
     * @return aos::Error.
     */
    aos::Error ProcessMessage(
        Channel channel, const aos::String& methodName, uint64_t requestID, const aos::Array<uint8_t>& data);

    /**
     * Returns pointer for receive buffer.
     *
     * @return void*.
     */
    void* GetReceiveBuffer() const { return mReceiveBuffer.Get(); }

    /**
     * Returns receive buffer size.
     *
     * @return size_t
     */
    size_t GetReceiveBufferSize() const { return mReceiveBuffer.Size(); }

private:
    static constexpr auto cNodeID   = CONFIG_AOS_NODE_ID;
    static constexpr auto cNodeType = CONFIG_AOS_NODE_TYPE;
    static constexpr auto cRunner   = "xrun";

    aos::Error ProcessGetUnitConfigStatus(Channel channel);
    aos::Error ProcessCheckUnitConfig(Channel channel, const servicemanager_v3_CheckUnitConfig& pbUnitConfig);
    aos::Error ProcessSetUnitConfig(Channel channel, const servicemanager_v3_SetUnitConfig& pbUnitConfig);
    aos::Error ProcessRunInstances(const servicemanager_v3_RunInstances& pbRunInstances);
    aos::Error ProcessImageContentInfo(const servicemanager_v3_ImageContentInfo& pbContentInfo);
    aos::Error ProcessImageContent(const servicemanager_v3_ImageContent& pbContent);
    aos::Error SendOutgoingMessage(Channel channel, const servicemanager_v3_SMOutgoingMessages& message,
        aos::Error messageError = aos::ErrorEnum::eNone);

    aos::sm::launcher::LauncherItf*      mLauncher {};
    ResourceManagerItf*                  mResourceManager {};
    aos::monitoring::ResourceMonitorItf* mResourceMonitor {};
    DownloadReceiverItf*                 mDownloader {};
    MessageSenderItf*                    mMessageSender {};

    aos::StaticAllocator<sizeof(servicemanager_v3_SMIncomingMessages) + sizeof(servicemanager_v3_SMOutgoingMessages)
            + aos::Max(sizeof(aos::ServiceInfoStaticArray) + sizeof(aos::LayerInfoStaticArray)
                    + sizeof(aos::InstanceInfoStaticArray),
                sizeof(ImageContentInfo) + sizeof(FileChunk)),
        sizeof(aos::monitoring::NodeInfo)>
        mAllocator;

    aos::StaticBuffer<servicemanager_v3_SMIncomingMessages_size> mReceiveBuffer;
    aos::StaticBuffer<servicemanager_v3_SMOutgoingMessages_size> mSendBuffer;
};

#endif
