/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <aos/common/tools/memory.hpp>

#include "utils/checksum.hpp"

#include "communication.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Communication::~Communication()
{
    {
        aos::LockGuard lock(mMutex);

        LOG_INF() << "Close communication channels";

        for (auto i = 0; i < static_cast<int>(ChannelEnum::eNumChannels); i++) {
            if (mChannels[i]->IsConnected()) {
                auto err = mChannels[i]->Close();
                if (!err.IsNone()) {
                    LOG_ERR() << "Can't close channel: " << err;
                }
            }
        }

        mClose = true;
        mCondVar.NotifyOne();
    }

    for (auto i = 0; i < static_cast<int>(ChannelEnum::eNumChannels); i++) {
        mChannelThreads[i].Join();
    }

    {
        aos::LockGuard lock(mMutex);

        mConnectionSubscribers.Clear();
    }
}

aos::Error Communication::Init(CommChannelItf& openChannel, CommChannelItf& secureChannel,
    aos::sm::launcher::LauncherItf& launcher, aos::iam::certhandler::CertHandlerItf& certHandler,
    ResourceManagerItf& resourceManager, aos::monitoring::ResourceMonitorItf& resourceMonitor,
    DownloadReceiverItf& downloader, ClockSyncItf& clockSync, ProvisioningItf& provisioning)
{
    LOG_DBG() << "Initialize communication";

    mClockSync    = &clockSync;
    mProvisioning = &provisioning;

    auto err = mCMClient.Init(launcher, resourceManager, resourceMonitor, downloader, clockSync, *this);
    if (!err.IsNone()) {
        return err;
    }

    err = mIAMServer.Init(certHandler, provisioning, *this);
    if (!err.IsNone()) {
        return err;
    }

    mChannels[static_cast<int>(ChannelEnum::eOpen)]   = &openChannel;
    mChannels[static_cast<int>(ChannelEnum::eSecure)] = &secureChannel;

    err = StartChannelThreads();
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Communication::InstancesRunStatus(const aos::Array<aos::InstanceStatus>& instances)
{
    aos::LockGuard lock(mMutex);

    return mCMClient.InstancesRunStatus(instances);
}

aos::Error Communication::InstancesUpdateStatus(const aos::Array<aos::InstanceStatus>& instances)
{
    aos::LockGuard lock(mMutex);

    return mCMClient.InstancesUpdateStatus(instances);
}

aos::Error Communication::SendImageContentRequest(const ImageContentRequest& request)
{
    aos::LockGuard lock(mMutex);

    return mCMClient.SendImageContentRequest(request);
}

aos::Error Communication::SendMonitoringData(const aos::monitoring::NodeMonitoringData& monitoringData)
{
    aos::LockGuard lock(mMutex);

    return mCMClient.SendMonitoringData(monitoringData);
}

aos::Error Communication::SendClockSyncRequest()
{
    aos::LockGuard lock(mMutex);

    return mCMClient.SendClockSyncRequest();
}

void Communication::ClockSynced()
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Clock synced";

    mClockSynced = true;
    mCondVar.NotifyOne();
}

void Communication::ClockUnsynced()
{
    aos::LockGuard lock(mMutex);

    LOG_WRN() << "Clock unsynced";

    mClockSynced = false;
    mCondVar.NotifyOne();
}

aos::Error Communication::Subscribes(aos::ConnectionSubscriberItf& subscriber)
{
    aos::LockGuard lock(mMutex);

    auto err = mConnectionSubscribers.PushBack(&subscriber);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

void Communication::Unsubscribes(aos::ConnectionSubscriberItf& subscriber)
{
    aos::LockGuard lock(mMutex);

    auto it = mConnectionSubscribers.Find(&subscriber);
    if (it.mError.IsNone()) {
        mConnectionSubscribers.Remove(it.mValue);
    }
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::Error Communication::StartChannelThreads()
{
    auto err = mChannelThreads[static_cast<int>(ChannelEnum::eOpen)].Run(
        [this](void*) { ChannelHandler(ChannelEnum::eOpen); });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    err = mChannelThreads[static_cast<int>(ChannelEnum::eSecure)].Run(
        [this](void*) { ChannelHandler(ChannelEnum::eSecure); });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

void Communication::ChannelHandler(Channel channel)
{
    while (true) {
        aos::Error err;

        {
            aos::UniqueLock lock(mMutex);

            if (mClose) {
                return;
            }

            err = ConnectChannel(lock, channel);
        }

        if (!err.IsNone()) {
            LOG_ERR() << "Can't connect to channel: channel = " << channel << ", err = " << err;
            LOG_WRN() << "Reconnect in " << cConnectionTimeoutSec << " sec...";

            sleep(cConnectionTimeoutSec);

            continue;
        }

        err = ProcessMessages(channel);
        if (!err.IsNone()) {
            LOG_ERR() << "Error processing messages: channel = " << channel << ", err = " << err;
        }

        {
            aos::LockGuard lock(mMutex);

            if (mClose) {
                return;
            }

            err = CloseChannel(channel);
            if (!err.IsNone()) {
                LOG_ERR() << "Error closing channel: channel = " << channel << ", err = " << err;
            }
        }
    }
}

aos::Error Communication::WaitTimeSynced(aos::UniqueLock& lock)
{
    LOG_DBG() << "Wait open channel is connected...";

    auto err
        = mCondVar.Wait(lock, [this]() { return mChannels[Channel(ChannelEnum::eOpen)]->IsConnected() || mClose; });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (mClose) {
        return aos::ErrorEnum::eNone;
    }

    err = mClockSync->Start();
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    LOG_DBG() << "Wait clock is synced...";

    err = mCondVar.Wait(lock, [this]() { return mClockSynced || mClose; });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Communication::ConnectChannel(aos::UniqueLock& lock, Channel channel)
{
    if (channel == ChannelEnum::eSecure) {
        if (!mClockSynced) {
            auto err = WaitTimeSynced(lock);
            if (!err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }

            if (mClose) {
                return aos::ErrorEnum::eNone;
            }
        }

        if (mProvisioning->IsProvisioned()) {
            LOG_INF() << "Set secure channel TLS config: certType = " << cSecureChannelCertType;

            auto err = mChannels[channel]->SetTLSConfig(cSecureChannelCertType);
            if (!err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }
    }

    auto err = mChannels[channel]->Connect();
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    LOG_INF() << "Channel connected: channel = " << channel;

    mCondVar.NotifyOne();

    if (GetNumConnectedChannels() == static_cast<size_t>(ChannelEnum::eNumChannels) && mProvisioning->IsProvisioned()) {
        err = mCMClient.SendNodeConfiguration();
        if (!err.IsNone()) {
            LOG_ERR() << "Can't send node configuration: " << err;
        }

        ConnectNotification(true);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Communication::CloseChannel(Channel channel)
{
    auto err = mChannels[channel]->Close();

    LOG_DBG() << "Channel closed: channel = " << channel;

    if (GetNumConnectedChannels() == static_cast<size_t>(ChannelEnum::eNumChannels) - 1) {
        ConnectNotification(false);
    }

    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Communication::SendMessage(Channel channel, AosVChanSource source, const aos::String& methodName,
    uint64_t requestID, const aos::Array<uint8_t> data, aos::Error messageError)
{
    LOG_DBG() << "Send message: channel = " << channel << ", source = " << source << ", method = " << methodName
              << ", size = " << data.Size() << " error = " << messageError;

    VChanMessageHeader header = {source, static_cast<uint32_t>(data.Size()), requestID, messageError.Errno(),
        static_cast<int32_t>(messageError.Value())};

    if (data.Size()) {
        auto checksum = CalculateSha256(data);
        if (!checksum.mError.IsNone()) {
            return AOS_ERROR_WRAP(checksum.mError);
        }

        aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(header.mSha256), aos::cSHA256Size) = checksum.mValue;
    }

    auto err
        = WriteMessage(channel, aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(&header), sizeof(VChanMessageHeader)));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    err = WriteMessage(channel, data);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Communication::ProcessMessages(Channel channel)
{
    auto headerData = aos::StaticArray<uint8_t, sizeof(VChanMessageHeader)>();

    while (true) {
        auto err = ReadMessage(channel, headerData, sizeof(VChanMessageHeader));
        if (!err.IsNone()) {
            return err;
        }

        auto header = reinterpret_cast<VChanMessageHeader*>(headerData.Get());

        LOG_DBG() << "Receive message: channel = " << channel << ", source = " << header->mSource
                  << ", method = " << header->mMethodName << ", size = " << header->mDataSize;

        MessageHandlerItf* handler = nullptr;

        switch (header->mSource) {
        case AOS_VCHAN_SM:
            handler = &mCMClient;
            break;

        case AOS_VCHAN_IAM:
            handler = &mIAMServer;
            break;

        default:
            LOG_ERR() << "Wrong source received: " << header->mSource;

            continue;
        }

        if (header->mDataSize > handler->GetReceiveBuffer().Size()) {
            LOG_ERR() << "Not enough mem in receive buffer";
            continue;
        }

        aos::Array<uint8_t> data(
            static_cast<uint8_t*>(handler->GetReceiveBuffer().Get()), handler->GetReceiveBuffer().Size());

        if (header->mDataSize) {
            err = ReadMessage(channel, data, header->mDataSize);
            if (!err.IsNone()) {
                return err;
            }

            auto checksum = CalculateSha256(data);
            if (!checksum.mError.IsNone()) {
                LOG_ERR() << "Can't calculate checksum: " << checksum.mError;
                continue;
            }

            if (checksum.mValue != aos::Array<uint8_t>(header->mSha256, aos::cSHA256Size)) {
                LOG_ERR() << "Checksum error";
                continue;
            }
        }

        aos::LockGuard lock(mMutex);

        err = handler->ReceiveMessage(channel, header->mMethodName, header->mRequestID, data);
        if (!err.IsNone()) {
            LOG_ERR() << "Error processing message: " << err;
        }
    }
}

size_t Communication::GetNumConnectedChannels()
{
    size_t count = 0;

    for (auto i = 0; i < static_cast<int>(ChannelEnum::eNumChannels); i++) {
        if (mChannels[i]->IsConnected()) {
            count++;
        }
    }

    return count;
}

void Communication::ConnectNotification(bool connected)
{
    LOG_INF() << "Connection notification: " << (connected ? "connected" : "disconnected");

    for (auto& subscriber : mConnectionSubscribers) {
        if (connected) {
            subscriber->OnConnect();
        } else {
            subscriber->OnDisconnect();
        }
    }
}

aos::Error Communication::ReadMessage(Channel channel, aos::Array<uint8_t>& data, size_t size)
{
    size_t read = 0;

    auto err = data.Resize(size);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    while (read < size) {
        auto ret = mChannels[channel]->Read(data.Get() + read, size - read);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        read += ret;
    }

    assert(read <= data.MaxSize());

    if (read != size) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eFailed);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Communication::WriteMessage(Channel channel, const aos::Array<uint8_t>& data)
{
    size_t written = 0;

    while (written < data.Size()) {
        auto ret = mChannels[channel]->Write(data.Get() + written, data.Size() - written);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        written += ret;
    }

    if (written != data.Size()) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eFailed);
    }

    return aos::ErrorEnum::eNone;
}
