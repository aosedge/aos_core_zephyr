/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "provisionmanagercallback.hpp"

namespace aos::zephyr::provisionmanager {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ProvisionManagerCallback::OnStartProvisioning(const String& password)
{
    return ErrorEnum::eNone;
}

Error ProvisionManagerCallback::OnFinishProvisioning(const String& password)
{
    return ErrorEnum::eNone;
}

Error ProvisionManagerCallback::OnDeprovision(const String& password)
{
    if (auto err = FS::ClearDir(cHSMDir); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error ProvisionManagerCallback::OnEncryptDisk(const String& password)
{
    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::provisionmanager
