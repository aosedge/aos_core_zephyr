/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 EPAM Systems
 */

#ifndef XENCONSOLE_H_
#define XENCONSOLE_H_

extern "C" {

/**
 * @typedef on_console_feed_cb_t
 * @brief Function callback, that is called when console receives a character.
 *
 * @param ch character received from console
 * @param cb_data private data, passed to callback
 */
typedef void (*on_console_feed_cb_t)(char ch, void* cb_data);

/**
 * Sets the console feed cb object.
 *
 * @param domain - domain, where console feed cb will be set
 *
 * @param cb - callback function to be called on console feed
 *
 * @param cb_data - data to be passed to the callback function
 *
 * @return - zero on success, negative errno on failure
 */
int set_console_feed_cb(struct xen_domain* domain, on_console_feed_cb_t cb, void* cb_data);

} // extern "C"

#endif /* XRUN_H_ */
