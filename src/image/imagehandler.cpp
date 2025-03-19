/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/fs.hpp>

#include "imagehandler.hpp"
#include "log.hpp"

namespace aos::zephyr::image {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ImageHandler::Init(
    spaceallocator::SpaceAllocatorItf& layerSpaceAllocator, spaceallocator::SpaceAllocatorItf& serviceSpaceAllocator)

{
    LOG_INF() << "Initialize image handler";

    mLayerSpaceAllocator   = &layerSpaceAllocator;
    mServiceSpaceAllocator = &serviceSpaceAllocator;

    return ErrorEnum::eNone;
}

RetWithError<StaticString<cFilePathLen>> ImageHandler::InstallLayer(const String& archivePath,
    const String& installBasePath, const LayerInfo& layer, UniquePtr<aos::spaceallocator::SpaceItf>& space)
{
    LOG_DBG() << "Install layer: archive=" << archivePath << ", digest=" << layer.mLayerDigest;

    return {{}, ErrorEnum::eNotSupported};
}

RetWithError<StaticString<cFilePathLen>> ImageHandler::InstallService(const String& archivePath,
    const String& installBasePath, const ServiceInfo& service, UniquePtr<aos::spaceallocator::SpaceItf>& space)
{
    LOG_DBG() << "Install service: archive=" << archivePath << ", installBasePath=" << installBasePath
              << ", serviceID=" << service.mServiceID;

    auto installedPath = fs::JoinPath(installBasePath, service.mServiceID);
    installedPath.Append("-v").Append(service.mVersion);

    Error err = fs::Rename(archivePath, installedPath);
    if (!err.IsNone()) {
        LOG_ERR() << "Can't rename archive: err=" << err;

        return {{}, AOS_ERROR_WRAP(err)};
    }

    size_t serviceSize = 0;

    Tie(serviceSize, err) = fs::CalculateSize(installedPath);
    if (!err.IsNone()) {
        return {installedPath, err};
    }

    Tie(space, err) = mServiceSpaceAllocator->AllocateSpace(serviceSize);
    if (!err.IsNone()) {
        return {installedPath, err};
    }

    return {installedPath, ErrorEnum::eNone};
}

Error ImageHandler::ValidateService(const String& path) const
{
    (void)path;

    return ErrorEnum::eNone;
}

RetWithError<StaticString<oci::cMaxDigestLen>> ImageHandler::CalculateDigest(const String& path) const
{
    (void)path;

    return {StaticString<oci::cMaxDigestLen>(), ErrorEnum::eNone};
}

} // namespace aos::zephyr::image
