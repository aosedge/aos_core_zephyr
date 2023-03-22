/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STORAGE_HPP_
#define STORAGE_HPP_

#include <aos/sm/launcher.hpp>

/**
 * Storage instance.
 */
class Storage : public aos::sm::launcher::StorageItf, private aos::NonCopyable {
public:
    /**
     * Initializes storage instance.
     * @return aos::Error.
     */
    aos::Error Init();

    /**
     * Adds new instance to storage.
     *
     * @param instance instance to add.
     * @return Error.
     */
    aos::Error AddInstance(const aos::InstanceInfo& instance) override;

    /**
     * Updates previously stored instance.
     *
     * @param instance instance to update.
     * @return Error.
     */
    aos::Error UpdateInstance(const aos::InstanceInfo& instance) override;

    /**
     * Removes previously stored instance.
     *
     * @param instanceID instance ID to remove.
     * @return Error.
     */
    aos::Error RemoveInstance(const aos::InstanceIdent& instanceIdent) override;

    /**
     * Returns all stored instances.
     *
     * @param instances array to return stored instances.
     * @return Error.
     */
    aos::Error GetAllInstances(aos::Array<aos::InstanceInfo>& instances) override;
};

#endif
