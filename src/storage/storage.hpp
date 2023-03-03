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
};

#endif
