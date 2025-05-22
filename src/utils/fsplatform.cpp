/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/stat.h>
#include <unistd.h>

#ifdef CONFIG_NATIVE_APPLICATION
#include <sys/statvfs.h>
#else
#include <zephyr/fs/fs.h>
#endif

#include <aos/common/tools/fs.hpp>

#include "utils/fsplatform.hpp"

namespace aos::zephyr::utils {

RetWithError<StaticString<cFilePathLen>> FSPlatform::GetMountPoint(const String& dir) const
{
    (void)dir;

    return {CONFIG_AOS_DISK_MOUNT_POINT};
}

RetWithError<size_t> FSPlatform::GetTotalSize(const String& dir) const
{
#ifdef CONFIG_NATIVE_APPLICATION
    struct statvfs sbuf;

    if (auto ret = statvfs(dir.CStr(), &sbuf); ret != 0) {
        return {{}, AOS_ERROR_WRAP(ret)};
    }
#else
    struct fs_statvfs sbuf;

    if (auto ret = fs_statvfs(dir.CStr(), &sbuf); ret != 0) {
        return {{}, AOS_ERROR_WRAP(ret)};
    }
#endif

    return size_t(sbuf.f_frsize) * sbuf.f_blocks;
}

RetWithError<size_t> FSPlatform::GetDirSize(const String& dir) const
{
    return fs::CalculateSize(dir);
}

RetWithError<size_t> FSPlatform::GetAvailableSize(const String& dir) const
{
#ifdef CONFIG_NATIVE_APPLICATION
    struct statvfs sbuf;

    if (auto ret = statvfs(dir.CStr(), &sbuf); ret != 0) {
        return {{}, AOS_ERROR_WRAP(ret)};
    }
#else
    struct fs_statvfs sbuf;

    if (auto ret = fs_statvfs(dir.CStr(), &sbuf); ret != 0) {
        return {{}, AOS_ERROR_WRAP(ret)};
    }
#endif

    return size_t(sbuf.f_frsize) * sbuf.f_bfree;
}

} // namespace aos::zephyr::utils
