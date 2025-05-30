/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <domain.h>
#include <xen_console.h>
#include <xen_dom_mgmt.h>

#include "runner/consolereader.hpp"

#include "log.hpp"

namespace aos::zephyr::runner {

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

namespace {

RetWithError<xen_domain*> DomainByInstanceID(const String& instanceID)
{
    auto domID = find_domain_by_name(const_cast<char*>(instanceID.CStr()));
    if (domID == 0) {
        return {nullptr, AOS_ERROR_WRAP(ErrorEnum::eNotFound)};
    }

    auto domain = get_domain(domID);
    if (!domain) {
        return {nullptr, AOS_ERROR_WRAP(ErrorEnum::eNotFound)};
    }

    return domain;
}

} // namespace

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ConsoleReader::Subscribe(const String& instanceID)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Subscribe console reader" << Log::Field("instanceID", instanceID);

    auto [domain, err] = DomainByInstanceID(instanceID);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (mHandlers.FindIf([instanceID](const auto& reader) { return reader.GetInstanceID() == instanceID; })
        != mHandlers.end()) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eAlreadyExist);
    }

    err = mHandlers.EmplaceBack(domain->name);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto ret = set_console_feed_cb(domain, OnConsoleFeed, &mHandlers.Back()); ret != 0) {
        return AOS_ERROR_WRAP(-ret);
    }

    return ErrorEnum::eNone;
}

Error ConsoleReader::Unsubscribe(const String& instanceID)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Unsubscribe console reader" << Log::Field("instanceID", instanceID);

    auto [domain, err] = DomainByInstanceID(instanceID);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto ret = set_console_feed_cb(domain, nullptr, nullptr); ret != 0) {
        LOG_WRN() << "Could not unregister console feed callback" << Log::Field("instanceID", instanceID)
                  << Log::Field("code", ret);
    }

    auto it = mHandlers.FindIf([instanceID](const auto& reader) { return reader.GetInstanceID() == instanceID; });
    if (it == mHandlers.end()) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eNotFound);
    }

    mHandlers.Erase(it);

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

ConsoleReader::ConsoleHandler::~ConsoleHandler()
{
    LOG_DBG() << "Destroy domain console reader" << aos::Log::Field("instanceID", mInstanceID);

    if (!mBuffer.IsEmpty()) {
        Log();
    }
}

void ConsoleReader::ConsoleHandler::OnConsoleFeed(char ch)
{
    if (mBuffer.IsFull()) {
        Log();
    }

    if (ch == '\n' || ch == '\0') {
        Log();
    } else {
        mBuffer.Append(aos::String(&ch, 1));
    }
}

void ConsoleReader::ConsoleHandler::Log()
{
    mBuffer.Trim(" \t\r\n");

    LOG_INF() << "[" << mInstanceID << "]" << mBuffer;

    mBuffer.Clear();
}

void ConsoleReader::OnConsoleFeed(char ch, void* data)
{
    auto* reader = static_cast<ConsoleHandler*>(data);

    reader->OnConsoleFeed(ch);
}

} // namespace aos::zephyr::runner
