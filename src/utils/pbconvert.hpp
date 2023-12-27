/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PBCONVERT_HPP_
#define PBCONVERT_HPP_

#include <aos/common/types.hpp>

#include <proto/servicemanager/v3/servicemanager.pb.h>

/**
 * Converts Aos string to PB string.
 *
 * @tparam cSize max PB string size.
 * @param src Aos string to convert from.
 */
template <size_t cSize>
constexpr void StringToPB(const aos::String& src, char (&dst)[cSize])
{
    aos::String(dst, cSize - 1) = src;
}

/**
 * Converts PB string to Aos string.
 *
 * @tparam cSize max PB string size.
 * @param dst Aos string to convert to.
 */
template <size_t cSize>
constexpr void PBToString(const char (&src)[cSize], aos::String& dst)
{
    auto len = strlen(src);

    if (len > cSize - 1) {
        len = cSize - 1;
    }

    dst = aos::String(src, len);
}

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

    auto err = dst.FromString(aos::String(src, len));
    assert(err.IsNone());
}

/**
 * Converts Aos error to PB string.
 *
 * @tparam cSize max PB string size.
 * @param aosError Aos error to convert from.
 */
template <size_t cSize>
constexpr void ErrorToPB(const aos::Error& aosError, char (&dst)[cSize])
{
    aos::String str(dst, cSize - 1);

    str.Clear();

    if (aosError.IsNone()) {
        return;
    }

    str.Convert(aosError);
}

/**
 * Converts Aos byte array to PB byte array.
 *
 * @tparam T PB byte array type.
 * @param src Aos array to convert from.
 * @param dst PB array to covert to.
 */
template <typename T>
constexpr void ByteArrayToPB(const aos::Array<uint8_t>& src, T& dst)
{
    aos::Array<uint8_t>(dst.bytes, sizeof(dst.bytes)) = src;
    dst.size                                          = src.Size();
}

/**
 * Converts PB byte array to Aos byte array.
 *
 * @tparam T PB byte array type.
 * @param src PB array to convert from.
 * @param dst Aos array to covert to.
 */
template <typename T>
constexpr void PBToByteArray(const T& src, aos::Array<uint8_t>& dst)
{
    auto size = src.size;

    if (size > sizeof(src.bytes)) {
        size = sizeof(src.bytes);
    }

    dst = aos::Array<uint8_t>(src.bytes, size);
}

/**
 * Converts PB version info to Aos version info.
 *
 * @param pbVersion PB version info.
 * @param aosVersion Aos version info.
 */
constexpr void PBToVersionInfo(const servicemanager_v3_VersionInfo pbVersion, aos::VersionInfo& aosVersion)
{
    aosVersion.mAosVersion = pbVersion.aos_version;
    PBToString(pbVersion.vendor_version, aosVersion.mVendorVersion);
    PBToString(pbVersion.description, aosVersion.mDescription);
}

/**
 * Converts Aos version info PB version info.
 *
 * @param aosVersion Aos version info.
 * @param pbVersion PB version info.
 */
constexpr void VersionInfoToPB(const aos::VersionInfo& aosVersion, servicemanager_v3_VersionInfo& pbVersion)
{
    pbVersion.aos_version = aosVersion.mAosVersion;
    StringToPB(aosVersion.mVendorVersion, pbVersion.vendor_version);
    StringToPB(aosVersion.mDescription, pbVersion.description);
}

/**
 * Converts Aos instance ident to PB instance ident.
 *
 * @param aosIdent Aos instance ident.
 * @param pbIdent PB instance ident.
 */
constexpr void InstanceIdentToPB(const aos::InstanceIdent& aosIdent, servicemanager_v3_InstanceIdent& pbIdent)
{
    StringToPB(aosIdent.mServiceID, pbIdent.service_id);
    StringToPB(aosIdent.mSubjectID, pbIdent.subject_id);
    pbIdent.instance = aosIdent.mInstance;
}

/**
 * Converts PV instance ident to Aos instance ident.
 *
 * @param pbIdent PB instance ident.
 * @param aosIdent Aos instance ident.
 */
constexpr void PBToInstanceIdent(const servicemanager_v3_InstanceIdent& pbIdent, aos::InstanceIdent& aosIdent)
{
    PBToString(pbIdent.service_id, aosIdent.mServiceID);
    PBToString(pbIdent.subject_id, aosIdent.mSubjectID);
    aosIdent.mInstance = pbIdent.instance;
}

#endif
