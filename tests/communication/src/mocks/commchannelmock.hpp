/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMCHANNELMOCK_HPP_
#define COMMCHANNELMOCK_HPP_

#include <condition_variable>
#include <mutex>
#include <vector>

#include <aos/common/tools/string.hpp>

#include "communication/commchannel.hpp"

class CommChannelMock : public CommChannelItf {
public:
    aos::Error SetSecure(const aos::String& certType) override { return aos::ErrorEnum::eNone; }

    aos::Error Connect() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mConnected = true;

        return mConnectError;
    }

    aos::Error Close() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mConnected = false;

        mCV.notify_one();

        return mConnectError;
    }

    aos::Error Read(aos::Array<uint8_t>& data, size_t size) override
    {
        std::unique_lock<std::mutex> lock(mMutex);

        data.Clear();

        mCV.wait(lock, [&] { return mReadData.size() >= size || !mReadError.IsNone() || !mConnected; });

        if (!mConnected) {
            return aos::ErrorEnum::eFailed;
        }

        if (!mReadError.IsNone()) {
            auto err   = mReadError;
            mReadError = aos::ErrorEnum::eNone;

            return err;
        }

        data += aos::Array<uint8_t>(mReadData.data(), size);
        mReadData.erase(mReadData.begin(), mReadData.begin() + size);

        return aos::ErrorEnum::eNone;
    }

    aos::Error Write(const aos::Array<uint8_t>& data) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mWriteData.insert(mWriteData.end(), data.begin(), data.end());

        mCV.notify_one();

        return mWriteError;
    }

    bool IsConnected() const override
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mConnected;
    }

    aos::Error WaitWrite(std::vector<uint8_t>& data, size_t size, const std::chrono::duration<double>& timeout,
        aos::Error err = aos::ErrorEnum::eNone)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        mWriteError = err;

        if (!mCV.wait_for(lock, timeout, [&] { return mWriteData.size() >= size; })) {
            return aos::ErrorEnum::eTimeout;
        }

        data.assign(mWriteData.begin(), mWriteData.begin() + size);
        mWriteData.erase(mWriteData.begin(), mWriteData.begin() + size);

        return aos::ErrorEnum::eNone;
    }

    void SendRead(const std::vector<uint8_t>& data, aos::Error err = aos::ErrorEnum::eNone)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mReadError = err;
        mReadData.insert(mReadData.end(), data.begin(), data.end());

        mCV.notify_one();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mReadData.clear();
        mWriteData.clear();
        mConnectError = aos::ErrorEnum::eNone;
        mReadError    = aos::ErrorEnum::eNone;
        mWriteError   = aos::ErrorEnum::eNone;
    }

private:
    bool                    mConnected = false;
    std::vector<uint8_t>    mReadData;
    std::vector<uint8_t>    mWriteData;
    aos::Error              mConnectError;
    aos::Error              mReadError;
    aos::Error              mWriteError;
    std::condition_variable mCV;
    mutable std::mutex      mMutex;
};

#endif
