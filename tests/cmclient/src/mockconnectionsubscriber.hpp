/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aos/common/connectionsubsc.hpp"

using namespace aos;

class MockConnectionSubscriber : public ConnectionSubscriberItf {
public:
    /**
     * Notifies publisher is connected.
     */
    void OnConnect() override;

    /**
     * Notifies publisher is disconnected.
     */
    void OnDisconnect() override;

    /**
     * Returns true if the subscriber is connected.
     */
    bool IsConnected() const;

private:
    bool connected {};
};
