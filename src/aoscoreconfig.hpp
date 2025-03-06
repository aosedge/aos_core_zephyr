/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOSCORECONFIG_HPP_
#define AOSCORECONFIG_HPP_

#ifndef CONFIG_NATIVE_APPLICATION

/**
 * Set thread stack alignment.
 */
#define AOS_CONFIG_THREAD_STACK_ALIGN ARCH_STACK_PTR_ALIGN

/**
 * Set timer signal event notification.
 */
#define AOS_CONFIG_TIMER_SIGEV_NOTIFY SIGEV_SIGNAL

/**
 * Use static PKCS11 lib.
 */
#define AOS_CONFIG_PKCS11_USE_STATIC_LIB 1

/**
 * Default PKCS11 library.
 */
#define AOS_CONFIG_CRYPTOUTILS_DEFAULT_PKCS11_LIB "libckteec"

/**
 * Configures clock ID used for thread time operations.
 */
#define AOS_CONFIG_THREAD_CLOCK_ID CLOCK_MONOTONIC

/**
 * Default static PKCS11 lib.
 */
#define AOS_CONFIG_CRYPTOUTILS_DEFAULT_PKCS11_LIB "libckteec"

#else

/**
 * Set thread stack alignment.
 */
#define AOS_CONFIG_THREAD_STACK_ALIGN      4096

/**
 * Configures thread stack guard size.
 */
#define AOS_CONFIG_THREAD_STACK_GUARD_SIZE 4096

#endif // CONFIG_NATIVE_APPLICATION

/**
 * Set default thread stack size.
 */
#define AOS_CONFIG_THREAD_DEFAULT_STACK_SIZE 32768

// This config also used to generate proto options file. Using Aos new operator causes redefinition error.
// Add condition to enable it for zephyr only to avoid the error.
#if defined(__ZEPHYR__) && !defined(CONFIG_EXTERNAL_LIBCPP)
/**
 * Use Aos new operators.
 */
#define AOS_CONFIG_NEW_USE_AOS 1

#endif

/**
 * Set num SM installs to 1 as downloader supports only one thread right now.
 */
#define AOS_CONFIG_SERVICEMANAGER_NUM_COOPERATE_INSTALLS 1

/**
 * Set Aos runtime dir.
 */
#define AOS_CONFIG_LAUNCHER_RUNTIME_DIR CONFIG_AOS_RUNTIME_DIR

/**
 * Set Aos services dir.
 */
#define AOS_CONFIG_SERVICEMANAGER_SERVICES_DIR CONFIG_AOS_SERVICES_DIR

/**
 * Set launcher thread stack size.
 */

#define AOS_CONFIG_LAUNCHER_THREAD_STACK_SIZE CONFIG_AOS_LAUNCHER_THREAD_STACK_SIZE

/**
 * Configures thread stack usage logging.
 */
#define AOS_CONFIG_THREAD_STACK_USAGE 1

/**
 * Maximum number of functions for functional service.
 */
#define AOS_CONFIG_TYPES_FUNCTIONS_MAX_COUNT 8

/**
 * Max number of instances.
 */
#define AOS_CONFIG_TYPES_MAX_NUM_INSTANCES 16

/**
 * Max number of services.
 */
#define AOS_CONFIG_TYPES_MAX_NUM_SERVICES 16

/**
 * Max number of layers.
 */
#define AOS_CONFIG_TYPES_MAX_NUM_LAYERS 16

#endif
