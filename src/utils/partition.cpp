/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_NATIVE_APPLICATION
#include <sys/statvfs.h>
#else
#include <zephyr/fs/fs.h>
#endif

#include "partition.hpp"

namespace aos::zephyr::utils {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

RetWithError<size_t> CalculatePartitionSize(const String& name)
{
#ifdef CONFIG_NATIVE_APPLICATION
    struct statvfs sbuf;

    if (auto ret = statvfs(name.CStr(), &sbuf); ret != 0) {
        return {0, ret};
    }
#else
    struct fs_statvfs sbuf;

    if (auto ret = fs_statvfs(name.CStr(), &sbuf); ret != 0) {
        return {0, ret};
    }
#endif

    return {sbuf.f_bsize * sbuf.f_blocks, ErrorEnum::eNone};
}

RetWithError<uint64_t> CalculatePartitionUsedSize(const String& path)
{
#ifdef CONFIG_NATIVE_APPLICATION
    struct statvfs sbuf;

    if (auto ret = statvfs(path.CStr(), &sbuf); ret != 0) {
        return {0, AOS_ERROR_WRAP(ret)};
    }
#else
    struct fs_statvfs sbuf;

    if (auto ret = fs_statvfs(path.CStr(), &sbuf); ret != 0) {
        return {0, AOS_ERROR_WRAP(ret)};
    }
#endif

    auto usedSize = static_cast<uint64_t>(sbuf.f_blocks - sbuf.f_bfree) * sbuf.f_bsize;

    return {usedSize, ErrorEnum::eNone};
}

} // namespace aos::zephyr::utils
