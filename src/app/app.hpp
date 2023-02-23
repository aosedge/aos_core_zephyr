/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/error.hpp>

/**
 * Aos zephyr application.
 */
class App {
public:
    /**
     * Initializes application.
     *
     * @return aos::Error
     */
    aos::Error Init();

    /**
     * Returns Aos application instance.
     * @return App&
     */
    static App& Get()
    {
        static App sApp;

        return sApp;
    };

private:
    App() {};
    App(App const&) = delete;
};
