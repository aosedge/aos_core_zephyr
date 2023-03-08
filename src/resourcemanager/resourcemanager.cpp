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

static constexpr auto cUnitConfigFilePath = CONFIG_AOS_UNIT_CONFIG_PATH;

aos::Error ResourceManager::GetUnitConfigInfo(char* version) const
{
    aos::Error err = aos::ErrorEnum::eNone;

    auto file = open(cUnitConfigFilePath, O_RDONLY);
    if (file < 0) {
        return AOS_ERROR_WRAP(file);
    }

    auto ret = read(file, version, sVersionLen);
    if (ret < 0) {
        err = AOS_ERROR_WRAP(ret);
    }

    ret = close(file);
    if (err.IsNone() && (ret < 0)) {
        err = AOS_ERROR_WRAP(ret);
    }

    return err;
}

aos::Error ResourceManager::CheckUnitConfig(const char* version, const char* unitConfig) const
{
    return aos::ErrorEnum::eNone;
}

aos::Error ResourceManager::UpdateUnitConfig(const char* version, const char* unitConfig)
{
    aos::Error err = aos::ErrorEnum::eNone;

    auto file = open(cUnitConfigFilePath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file < 0) {
        return AOS_ERROR_WRAP(file);
    }

    auto written = write(file, version, strlen(version));
    if (written < 0) {
        err = AOS_ERROR_WRAP(written);
    } else if ((size_t)written != strlen(version)) {
        err = AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    auto ret = close(file);
    if (err.IsNone() && (ret < 0)) {
        err = AOS_ERROR_WRAP(ret);
    }

    return err;
}
