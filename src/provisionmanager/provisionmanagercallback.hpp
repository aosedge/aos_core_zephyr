/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROVISIONMANAGER_HPP_
#define PROVISIONMANAGER_HPP_

#include <aos/iam/provisionmanager.hpp>

namespace aos::zephyr::provisionmanager {

/**
 * JSON provider.
 */
class ProvisionManagerCallback : public iam::provisionmanager::ProvisionManagerCallbackItf {

public:
    /**
     * Called when provisioning starts.
     *
     * @param password password.
     * @returns Error.
     */
    Error OnStartProvisioning(const String& password) override;

    /**
     * Called when provisioning finishes.
     *
     * @param password password.
     * @returns Error.
     */
    Error OnFinishProvisioning(const String& password) override;

    /**
     * Called on deprovisioning.
     *
     * @param password password.
     * @returns Error.
     */
    Error OnDeprovision(const String& password) override;

    /**
     * Called on disk encryption.
     *
     * @param password password.
     * @returns Error.
     */
    Error OnEncryptDisk(const String& password) override;

private:
    static constexpr auto cHSMDir = CONFIG_AOS_HSM_DIR;
};

} // namespace aos::zephyr::provisionmanager

#endif
