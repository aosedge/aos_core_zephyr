/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOSCORECONFIG_HPP_
#define AOSCORECONFIG_HPP_

/**
 * Use Aos new operators.
 */
#define AOS_CONFIG_NEW_USE_AOS 1

/**
 * Set num SM installs to 1 as downloader supports only one thread right now.
 */
#define AOS_CONFIG_SERVICEMANAGER_NUM_COOPERATE_INSTALLS 1

/**
 * Set Aos runtime dir.
 * TODO: we should have memory disk for runtime
 */
#define AOS_CONFIG_LAUNCHER_RUNTIME_DIR ".aos_storage/runtime"

/**
 * Set Aos services dir.
 */
#define AOS_CONFIG_SERVICEMANAGER_SERVICES_DIR ".aos_storage/services"

#endif
