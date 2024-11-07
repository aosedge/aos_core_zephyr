/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROVISIONMANAGERSTUB_HPP_
#define PROVISIONMANAGERSTUB_HPP_

#include <aos/iam/provisionmanager.hpp>

class ProvisionManagerStub : public aos::iam::provisionmanager::ProvisionManagerItf {
public:
    aos::Error StartProvisioning(const aos::String& password) override
    {
        mPassword = password;

        return aos::ErrorEnum::eNone;
    }

    aos::RetWithError<aos::iam::provisionmanager::CertTypes> GetCertTypes() const { return mCertTypes; }

    aos::Error CreateKey(
        const aos::String& certType, const aos::String& subject, const aos::String& password, aos::String& csr) override
    {
        mCertType = certType;
        mSubject  = subject;
        mPassword = password;
        csr       = mCSR;

        return aos::ErrorEnum::eNone;
    }

    aos::Error ApplyCert(
        const aos::String& certType, const aos::String& pemCert, aos::iam::certhandler::CertInfo& certInfo) override
    {
        mCertType = certType;
        mCert     = pemCert;
        certInfo  = mCertInfo;

        return aos::ErrorEnum::eNone;
    }

    aos::Error GetCert(const aos::String& certType, const aos::Array<uint8_t>& issuer,
        const aos::Array<uint8_t>& serial, aos::iam::certhandler::CertInfo& resCert) const override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error SubscribeCertChanged(
        const aos::String& certType, aos::iam::certhandler::CertReceiverItf& certReceiver) override
    {
        (void)certType;
        (void)certReceiver;

        return aos::ErrorEnum::eNone;
    }

    aos::Error UnsubscribeCertChanged(aos::iam::certhandler::CertReceiverItf& certReceiver) override
    {
        (void)certReceiver;

        return aos::ErrorEnum::eNone;
    }

    aos::Error FinishProvisioning(const aos::String& password) override
    {
        mPassword = password;

        return aos::ErrorEnum::eNone;
    }

    aos::Error Deprovision(const aos::String& password) override
    {
        mPassword = password;

        return aos::ErrorEnum::eNone;
    }

    aos::String GetPassword() const { return mPassword; }
    aos::String GetCertType() const { return mCertType; }
    aos::String GetSubject() const { return mSubject; }
    aos::String GetCSR() const { return mCSR; }
    aos::String GetCert() const { return mCert; }

    void SetCSR(const aos::String& csr) { mCSR = csr; }
    void SetCertTypes(const aos::iam::provisionmanager::CertTypes& certTypes) { mCertTypes = certTypes; }
    void SetCertInfo(const aos::iam::certhandler::CertInfo& certInfo) { mCertInfo = certInfo; }

private:
    aos::StaticString<aos::iam::certhandler::cPasswordLen> mPassword;
    aos::iam::provisionmanager::CertTypes                  mCertTypes;
    aos::StaticString<aos::iam::certhandler::cCertTypeLen> mCertType;
    aos::StaticString<aos::crypto::cSubjectCommonNameLen>  mSubject;
    aos::StaticString<aos::crypto::cCSRPEMLen>             mCSR;
    aos::StaticString<aos::crypto::cCertPEMLen>            mCert;
    aos::iam::certhandler::CertInfo                        mCertInfo;
};

#endif
