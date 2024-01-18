/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROVISIONINGMOCK_HPP_
#define PROVISIONINGMOCK_HPP_

#include <mutex>

#include "provisioning/provisioning.hpp"

class ProvisioningMock : public ProvisioningItf {
public:
    aos::Error FinishProvisioning() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mProvisioned = true;

        return aos::ErrorEnum::eNone;
    }

    bool IsProvisioned() const override
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mProvisioned;
    }

    void SetProvisioned(bool provisioned)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mProvisioned = provisioned;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mProvisioned = false;
    }

private:
    mutable std::mutex mMutex;
    bool               mProvisioned = false;
};

#endif
