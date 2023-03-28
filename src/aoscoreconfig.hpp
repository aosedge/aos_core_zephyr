/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOSCORECONFIG_HPP_
#define AOSCORECONFIG_HPP_

#ifndef CONFIG_BOARD_NATIVE_POSIX

/**
 * Set thread stack size.
 */
#define AOS_CONFIG_THREAD_DEFAULT_STACK_SIZE 4096

/**
 * Set thread stack alignment.
 */
#define AOS_CONFIG_THREAD_STACK_ALIGN ARCH_STACK_PTR_ALIGN

/**
 * Set timer signal event notification.
 */
#define AOS_CONFIG_TIMER_SIGEV_NOTIFY SIGEV_SIGNAL

#endif // CONFIG_POSIX_API

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
#define AOS_CONFIG_LAUNCHER_RUNTIME_DIR "/aos/runtime"

/**
 * Set Aos services dir.
 */
#define AOS_CONFIG_SERVICEMANAGER_SERVICES_DIR "/aos/services"

#endif
