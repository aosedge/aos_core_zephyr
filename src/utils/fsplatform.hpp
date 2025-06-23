/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UTILS_FSPLATFORM_HPP
#define UTILS_FSPLATFORM_HPP

#include <aos/common/tools/fs.hpp>

namespace aos::zephyr::utils {

class FSPlatform : public fs::FSPlatformItf {
public:
    /**
     * Gets mount point for the given directory.
     *
     * @param dir directory path.
     * @return RetWithError<StaticString<cFilePathLen>>.
     */
    RetWithError<StaticString<cFilePathLen>> GetMountPoint(const String& dir) const override;

    /**
     * Gets total size of the file system.
     *
     * @param dir directory path.
     * @return RetWithError<size_t>.
     */
    RetWithError<size_t> GetTotalSize(const String& dir) const override;

    /**
     * Gets directory size.
     *
     * @param dir directory path.
     * @return RetWithError<size_t>.
     */
    RetWithError<size_t> GetDirSize(const String& dir) const override;

    /**
     * Gets available size.
     *
     * @param dir directory path.
     * @return RetWithError<size_t>.
     */
    RetWithError<size_t> GetAvailableSize(const String& dir) const override;

private:
    static constexpr size_t cStatBlockSize = 512;
    static constexpr auto   cMtabPath      = "/proc/mounts";
};

} // namespace aos::zephyr::utils

#endif // UTILS_FSPLATFORM_HPP
