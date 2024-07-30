/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMUNICATIONMANAGER_HPP_
#define COMMUNICATIONMANAGER_HPP_

#include <aos/common/tools/error.hpp>
#include <aos/common/tools/thread.hpp>

#include "types.hpp"

class CommunicationManager {
public:
    aos::Error Init(TransportIntf& transport);
    aos::Error AddChannel(int port, ChanIntf& channel);
    aos::Error Run();
    aos::Error Stop();

private:
    static constexpr int cMaxChannels = 3;
    static constexpr int cHeaderSize  = 64;

    struct ChannelInfo {
        int       mPort;
        ChanIntf* mChannel;
    };

    aos::Error Connect();
    aos::Error HandleRead();

    aos::StaticArray<ChannelInfo, aos::cMaxChannels> mChannels;
    TransportIntf*                                   mTransport {};
    aos::Thread<>                                    mThread;
    aos::Mutex                                       mMutex;
    bool                                             mClose {false};
    aos::StaticArray<uint8_t, aos::cHeaderSize>      mHeader;
};

#endif // _COMMUNICATIONMANAGER_HPP_