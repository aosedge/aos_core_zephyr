/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/fs.hpp>

#include "log.hpp"
#include "provisioning.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error Provisioning::Init()
{
    LOG_DBG() << "Init provisioning";

    aos::StaticString<strlen(cProvisioningText)> payload;

    auto err = aos::FS::ReadFileToString(cProvisioningFile, payload);
    if (!err.IsNone() || payload != cProvisioningText) {
        LOG_INF() << "Unit is not provisioned";

        mProvisioned = false;
    } else {
        LOG_DBG() << "Unit is provisioned";

        mProvisioned = true;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Provisioning::FinishProvisioning()
{
    auto err = aos::FS::MakeDirAll(aos::FS::Dir(cProvisioningFile));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    err = aos::FS::WriteStringToFile(cProvisioningFile, cProvisioningText, 0600);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mProvisioned = true;

    LOG_INF() << "Provisioning finished";

    return aos::ErrorEnum::eNone;
}
