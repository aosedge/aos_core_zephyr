/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHANNELTYPE_HPP_
#define CHANNELTYPE_HPP_

#include <aos/common/tools/array.hpp>
#include <aos/common/tools/enum.hpp>

/**
 * Channel type enum.
 *
 */
class ChannelType {
public:
    enum class Enum {
        eOpen,
        eSecure,
        eNumChannels,
    };

    static const aos::Array<const char* const> GetStrings()
    {
        static const char* const sContentTypeStrings[] = {"open", "secure"};

        return aos::Array<const char* const>(sContentTypeStrings, aos::ArraySize(sContentTypeStrings));
    };
};

using ChannelEnum = ChannelType::Enum;
using Channel     = aos::EnumStringer<ChannelType>;

#endif
