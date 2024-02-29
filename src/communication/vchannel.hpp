/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VCHANNEL_HPP_
#define VCHANNEL_HPP_

#include <aos/common/tools/string.hpp>

#include <vch.h>

#include "commchannel.hpp"

class VChannel : public CommChannelItf {
public:
    static constexpr auto cXSOpenReadPath   = CONFIG_AOS_VCHAN_OPEN_TX_PATH;
    static constexpr auto cXSOpenWritePath  = CONFIG_AOS_VCHAN_OPEN_RX_PATH;
    static constexpr auto cXSCloseReadPath  = CONFIG_AOS_VCHAN_SECURE_TX_PATH;
    static constexpr auto cXSCloseWritePath = CONFIG_AOS_VCHAN_SECURE_RX_PATH;

    /**
     * Initializes vchan.
     *
     * @param xsReadPath Xen store read vchan path.
     * @param xsWritePath Xen store write vchan path.
     * @return aos::Error.
     */
    aos::Error Init(const aos::String& name, const aos::String& xsReadPath, const aos::String& xsWritePath);

    /**
     * Set TLS config.
     *
     * @param certType certificate type.
     * @return aos::Error
     */
    aos::Error SetTLSConfig(const aos::String& certType) override;

    /**
     * Connects to communication channel.
     *
     * @return aos::Error.
     */
    aos::Error Connect() override;

    /**
     * Closes current connection.
     */
    aos::Error Close() override;

    /**
     * Returns if channel is connected.
     *
     * @return bool.
     */
    bool IsConnected() const override { return mConnected; };

    /**
     * Reads data from channel to array.
     *
     * @param data buffer where data is placed to.
     * @param size specifies how many bytes to read.
     * @return int num read bytes.
     */
    int Read(void* data, size_t size) override;

    /**
     * Writes data from array to channel.
     *
     * @param data data buffer.
     * @param size specifies how many bytes to write.
     * @return int num written bytes.
     */
    int Write(const void* data, size_t size) override;

private:
    static constexpr auto cXSPathLen      = 128;
    static constexpr auto cDomdID         = CONFIG_AOS_DOMD_ID;
    static constexpr auto cChannelNameLen = 32;

    aos::StaticString<cXSPathLen> mXSReadPath;
    aos::StaticString<cXSPathLen> mXSWritePath;

    vch_handle mReadHandle;
    vch_handle mWriteHandle;

    bool mConnected = false;

    aos::StaticString<cChannelNameLen> mName;
};

#endif
