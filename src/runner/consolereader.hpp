/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONSOLEREADER_HPP_
#define CONSOLEREADER_HPP_

#include <aos/common/tools/error.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/common/tools/thread.hpp>
#include <aos/common/types.hpp>

namespace aos::zephyr::runner {

/**
 * Console reader class handles console output from Xen domains.
 */
class ConsoleReader {
public:
    /**
     * Subscribes to console output for a specific instance ID.
     *
     * @param instanceID domain instance ID to subscribe to.
     * @return Error.
     */
    Error Subscribe(const String& instanceID);

    /**
     * Unsubscribes from console output for a specific instance ID.
     *
     * @param instanceID domain instance ID to subscribe to.
     * @return Error.
     */
    Error Unsubscribe(const String& instanceID);

private:
    class ConsoleHandler {
    public:
        explicit ConsoleHandler(const String& instanceID)
            : mInstanceID(instanceID)
        {
        }

        ~ConsoleHandler();

        void OnConsoleFeed(char ch);

        String GetInstanceID() const { return mInstanceID; }

    private:
        void Log();

        StaticString<Log::cMaxLineLen> mBuffer;
        StaticString<cInstanceIDLen>   mInstanceID;
    };

    static void OnConsoleFeed(char ch, void* data);

    StaticArray<ConsoleHandler, cMaxNumInstances> mHandlers;
    Mutex                                         mMutex;
};

} // namespace aos::zephyr::runner

#endif
