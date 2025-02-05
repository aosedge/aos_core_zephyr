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

#include "transport.hpp"

namespace aos::zephyr::communication {

class XenVChan : public TransportItf {
public:
    static constexpr auto cReadPath  = CONFIG_AOS_CHAN_TX_PATH;
    static constexpr auto cWritePath = CONFIG_AOS_CHAN_RX_PATH;

    /**
     * Initializes vchan.
     *
     * @param xsReadPath Xen store read vchan path.
     * @param xsWritePath Xen store write vchan path.
     * @return aos::Error.
     */
    aos::Error Init(const aos::String& xsReadPath, const aos::String& xsWritePath);

    /**
     * Opens vchan.
     *
     * @return aos::Error.
     */
    aos::Error Open() override;

    /**
     * Closes vchan.
     */
    aos::Error Close() override;

    /**
     * Returns if vchan is opened.
     *
     * @return bool.
     */
    bool IsOpened() const override { return mOpened; };

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
    static constexpr auto cXSPathLen = 128;
    static constexpr auto cDomdID    = CONFIG_AOS_DOMD_ID;

    aos::StaticString<cXSPathLen> mXSReadPath;
    aos::StaticString<cXSPathLen> mXSWritePath;

    vch_handle mReadHandle;
    vch_handle mWriteHandle;

    bool mOpened = false;
};

} // namespace aos::zephyr::communication

#endif
