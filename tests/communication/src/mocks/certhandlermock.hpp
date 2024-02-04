/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CERTHANDLERMOCK_HPP_
#define CERTHANDLERMOCK_HPP_

#include <mutex>
#include <vector>

#include <aos/iam/certhandler.hpp>

class CertHandlerMock : public aos::iam::certhandler::CertHandlerItf {
public:
    aos::Error GetCertTypes(aos::Array<aos::StaticString<aos::iam::certhandler::cCertTypeLen>>& certTypes) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        for (const auto& type : mCertTypes) {
            certTypes.PushBack(type.c_str());
        }

        return aos::ErrorEnum::eNone;
    }

    aos::Error SetOwner(const aos::String& certType, const aos::String& password) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mCertType = certType.CStr();
        mPassword = password.CStr();

        return aos::ErrorEnum::eNone;
    }

    aos::Error Clear(const aos::String& certType) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mClear = true;

        return aos::ErrorEnum::eNone;
    }

    aos::Error CreateKey(const aos::String& certType, const aos::String& subjectCommonName, const aos::String& password,
        aos::String& pemCSR) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mCertType = certType.CStr();
        mSubject  = subjectCommonName.CStr();
        mPassword = password.CStr();

        pemCSR = mCSR.c_str();

        return aos::ErrorEnum::eNone;
    }

    aos::Error ApplyCertificate(
        const aos::String& certType, const aos::String& pemCert, aos::iam::certhandler::CertInfo& info) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mCertificate.assign(reinterpret_cast<const char*>(pemCert.Get()), pemCert.Size());
        info = mCertInfo;

        return aos::ErrorEnum::eNone;
    }

    aos::Error GetCertificate(const aos::String& certType, const aos::Array<uint8_t>& issuer,
        const aos::Array<uint8_t>& serial, aos::iam::certhandler::CertInfo& resCert) override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error CreateSelfSignedCert(const aos::String& certType, const aos::String& password) override
    {
        return aos::ErrorEnum::eNone;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mCertTypes.clear();
        mCertType.clear();
        mPassword.clear();
        mClear = false;
        mSubject.clear();
        mCSR.clear();
        mCertificate.clear();
        mCertInfo = {};
    }

    void SetCertTypes(const std::vector<std::string>& certTypes)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mCertTypes = certTypes;
    }

    void SetCSR(const std::string& csr)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mCSR = csr;
    }

    std::string GetCertType() const
    {
        std::lock_guard<std::mutex> lock(mMutex);

        return mCertType;
    }

    std::string GetPassword() const
    {
        std::lock_guard<std::mutex> lock(mMutex);

        return mPassword;
    }

    bool GetClear() const
    {
        std::lock_guard<std::mutex> lock(mMutex);

        return mClear;
    }

    std::string GetSubject() const
    {
        std::lock_guard<std::mutex> lock(mMutex);

        return mSubject;
    }

    std::string GetCertificate() const
    {
        std::lock_guard<std::mutex> lock(mMutex);

        return mCertificate;
    }

    void SetCertInfo(const aos::iam::certhandler::CertInfo& info) { mCertInfo = info; }

private:
    mutable std::mutex mMutex;

    std::vector<std::string>        mCertTypes;
    std::string                     mCertType;
    std::string                     mPassword;
    bool                            mClear = false;
    std::string                     mSubject;
    std::string                     mCSR;
    std::string                     mCertificate;
    aos::iam::certhandler::CertInfo mCertInfo;
};

#endif
