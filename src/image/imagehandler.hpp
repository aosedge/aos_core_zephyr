/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IMAGEHANDLER_HPP_
#define IMAGEHANDLER_HPP_

#include <aos/sm/image/imagehandler.hpp>

namespace aos::zephyr::image {

/**
 * Image handler.
 */
class ImageHandler : public aos::sm::image::ImageHandlerItf {
public:
    /**
     * Initializes image handler.
     *
     * @param layerSpaceAllocator layer space allocator.
     * @param serviceSpaceAllocator service space allocator.
     * @return Error.
     */
    Error Init(spaceallocator::SpaceAllocatorItf& layerSpaceAllocator,
        spaceallocator::SpaceAllocatorItf&        serviceSpaceAllocator);

    /**
     * Installs layer from the provided archive.
     *
     * @param archivePath archive path.
     * @param installBasePath installation base path.
     * @param layer layer info.
     * @param space[out] installed layer space.
     * @return RetWithError<StaticString<cFilePathLen>>.
     */
    RetWithError<StaticString<cFilePathLen>> InstallLayer(const String& archivePath, const String& installBasePath,
        const LayerInfo& layer, UniquePtr<aos::spaceallocator::SpaceItf>& space) override;

    /**
     * Installs service from the provided archive.
     *
     * @param archivePath archive path.
     * @param installBasePath installation base path.
     * @param service service info.
     * @param space[out] installed service space.
     * @return RetWithError<StaticString<cFilePathLen>>.
     */
    RetWithError<StaticString<cFilePathLen>> InstallService(const String& archivePath, const String& installBasePath,
        const ServiceInfo& service, UniquePtr<aos::spaceallocator::SpaceItf>& space) override;

    /**
     * Validates service.
     *
     * @param path service path.
     * @return Error.
     */
    Error ValidateService(const String& path) const override;

    /**
     * Calculates digest for the given path or file.
     *
     * @param path root folder or file.
     * @return RetWithError<StaticString<cMaxDigestLen>>.
     */
    RetWithError<StaticString<oci::cMaxDigestLen>> CalculateDigest(const String& path) const override;

private:
    spaceallocator::SpaceAllocatorItf* mLayerSpaceAllocator   = nullptr;
    spaceallocator::SpaceAllocatorItf* mServiceSpaceAllocator = nullptr;
};

} // namespace aos::zephyr::image

#endif
