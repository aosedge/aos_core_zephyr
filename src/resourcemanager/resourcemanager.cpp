/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>

#include <fcntl.h>
#include <unistd.h>

#include "log.hpp"
#include "resourcemanager.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error ResourceManager::GetUnitConfigInfo(aos::String& version) const
{
    aos::Error err = aos::ErrorEnum::eNone;

    auto file = open(cUnitConfigFilePath, O_RDONLY);
    if (file < 0) {
        return AOS_ERROR_WRAP(errno);
    }

    auto ret = read(file, version.Get(), version.MaxSize());
    if (ret < 0) {
        err = AOS_ERROR_WRAP(errno);
    } else {
        version.Resize(ret);
    }

    ret = close(file);
    if (err.IsNone() && (ret < 0)) {
        err = AOS_ERROR_WRAP(errno);
    }

    return err;
}

aos::Error ResourceManager::CheckUnitConfig(const aos::String& version, const aos::String& unitConfig) const
{
    return aos::ErrorEnum::eNone;
}

aos::Error ResourceManager::UpdateUnitConfig(const aos::String& version, const aos::String& unitConfig)
{
    aos::Error err = aos::ErrorEnum::eNone;

    auto file = open(cUnitConfigFilePath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file < 0) {
        return AOS_ERROR_WRAP(errno);
    }

    auto written = write(file, version.Get(), version.Size());
    if (written < 0) {
        err = AOS_ERROR_WRAP(written);
    } else if ((size_t)written != version.Size()) {
        err = AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    auto ret = close(file);
    if (err.IsNone() && (ret < 0)) {
        err = AOS_ERROR_WRAP(errno);
    }

    return err;
}
