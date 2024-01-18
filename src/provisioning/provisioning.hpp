/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_PROVISIONING_HPP_
#define AOS_PROVISIONING_HPP_

#include <aos/common/tools/error.hpp>

/**
 * Provisioning interface.
 */
class ProvisioningItf {
public:
    /**
     * Finishes provisioning.
     *
     * @return aos::Error.
     */
    virtual aos::Error FinishProvisioning() = 0;

    /**
     * Synchronizes system clock.
     *
     * @return bool.
     */
    virtual bool IsProvisioned() const = 0;

    /**
     * Destructor.
     */
    virtual ~ProvisioningItf() = default;
};

/**
 * Provisioning instance.
 */
class Provisioning : public ProvisioningItf {
public:
    /**
     * Initializes provisioning.
     *
     * @return aos::Error.
     */
    aos::Error Init();

    /**
     * Finishes aos::provisioning.
     *
     * @return Error.
     */
    aos::Error FinishProvisioning() override;

    /**
     * Synchronizes system clock.
     *
     * @return bool.
     */
    bool IsProvisioned() const override { return mProvisioned; }

private:
    static constexpr auto cProvisioningFile = CONFIG_AOS_PROVISIONING_FILE;
    static constexpr auto cProvisioningText = "provisioned";

    bool mProvisioned = false;
};

#endif
