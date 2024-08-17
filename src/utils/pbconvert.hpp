/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PBCONVERT_HPP_
#define PBCONVERT_HPP_

#include <aos/common/types.hpp>

#include <proto/common/v1/common.pb.h>

#include "utils.hpp"

namespace aos::zephyr::utils {

/**
 * Converts PB string to Aos enum.
 *
 * @tparam T Aos enum type.
 * @tparam cSize max PB string size.
 * @param dst Aos enum to convert to.
 */

template <typename T, size_t cSize>
constexpr void PBToEnum(const char (&src)[cSize], T& dst)
{
    auto len = strlen(src);

    if (len > cSize - 1) {
        assert(false);
        len = cSize - 1;
    }

    auto err = dst.FromString(String(src, len));
    assert(err.IsNone());
}

/**
 * Converts Aos error to PB error info.
 *
 * @param aosError Aos error to convert from.
 * @param[out] errorInfo PB error info to convert to.
 */
template <typename T>
void ErrorToPB(const Error& aosError, T& errorInfo)
{
    if (aosError.IsNone()) {
        errorInfo.has_error = false;
        return;
    }

    errorInfo.has_error       = true;
    errorInfo.error.aos_code  = int(aosError.Value());
    errorInfo.error.exit_code = aosError.Errno();

    StringFromCStr(errorInfo.error.message).Convert(aosError);
}

/**
 * Converts Aos byte array to PB byte array.
 *
 * @tparam T PB byte array type.
 * @param src Aos array to convert from.
 * @param dst PB array to covert to.
 */
template <typename T>
constexpr void ByteArrayToPB(const Array<uint8_t>& src, T& dst)
{
    Array<uint8_t>(dst.bytes, sizeof(dst.bytes)) = src;
    dst.size                                     = src.Size();
}

/**
 * Converts PB byte array to Aos byte array.
 *
 * @tparam T PB byte array type.
 * @param src PB array to convert from.
 * @param dst Aos array to covert to.
 */
template <typename T>
constexpr void PBToByteArray(const T& src, Array<uint8_t>& dst)
{
    auto size = src.size;

    if (size > sizeof(src.bytes)) {
        size = sizeof(src.bytes);
    }

    dst = Array<uint8_t>(src.bytes, size);
}

/**
 * Converts Aos instance ident to PB instance ident.
 *
 * @param aosIdent Aos instance ident.
 * @param pbIdent PB instance ident.
 */
inline void InstanceIdentToPB(const InstanceIdent& aosIdent, common_v1_InstanceIdent& pbIdent)
{
    StringFromCStr(pbIdent.service_id) = aosIdent.mServiceID;
    StringFromCStr(pbIdent.subject_id) = aosIdent.mSubjectID;
    pbIdent.instance                   = aosIdent.mInstance;
}

/**
 * Converts PV instance ident to Aos instance ident.
 *
 * @param pbIdent PB instance ident.
 * @param aosIdent Aos instance ident.
 */
inline void PBToInstanceIdent(const common_v1_InstanceIdent& pbIdent, InstanceIdent& aosIdent)
{
    aosIdent.mServiceID = StringFromCStr(pbIdent.service_id);
    aosIdent.mSubjectID = StringFromCStr(pbIdent.subject_id);
    aosIdent.mInstance  = pbIdent.instance;
}

} // namespace aos::zephyr::utils

#endif
