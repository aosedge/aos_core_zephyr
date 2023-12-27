/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CMCLIENT_HPP_
#define CMCLIENT_HPP_

#include <aos/sm/launcher.hpp>

#include <proto/servicemanager/v3/servicemanager.pb.h>

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
     * @param messageSender message sender instance.
     * @return aos::Error.
     */
    aos::Error Init(
        aos::sm::launcher::LauncherItf& launcher, ResourceManagerItf& resourceManager, MessageSenderItf& messageSender);

    /**
     * Processes received message.
     *
     * @param methodName protocol method name.
     * @param requestID protocol request ID.
     * @param data raw message data.
     * @return aos::Error.
     */
    aos::Error ProcessMessage(const aos::String& methodName, uint64_t requestID, const aos::Array<uint8_t>& data);

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
    aos::Error ProcessGetUnitConfigStatus();
    aos::Error ProcessCheckUnitConfig(const servicemanager_v3_CheckUnitConfig& pbUnitConfig);
    aos::Error ProcessSetUnitConfig(const servicemanager_v3_SetUnitConfig& pbUnitConfig);
    aos::Error ProcessRunInstances(const servicemanager_v3_RunInstances& pbRunInstances);
    aos::Error SendOutgoingMessage(
        const servicemanager_v3_SMOutgoingMessages& message, aos::Error messageError = aos::ErrorEnum::eNone);

    aos::sm::launcher::LauncherItf* mLauncher {};
    ResourceManagerItf*             mResourceManager {};
    MessageSenderItf*               mMessageSender {};

    aos::StaticAllocator<sizeof(servicemanager_v3_SMIncomingMessages) + sizeof(servicemanager_v3_SMOutgoingMessages)
        + sizeof(aos::ServiceInfoStaticArray) + sizeof(aos::LayerInfoStaticArray)
        + sizeof(aos::InstanceInfoStaticArray)>
        mAllocator;

    aos::StaticBuffer<servicemanager_v3_SMIncomingMessages_size> mReceiveBuffer;
    aos::StaticBuffer<servicemanager_v3_SMOutgoingMessages_size> mSendBuffer;
};

#endif
