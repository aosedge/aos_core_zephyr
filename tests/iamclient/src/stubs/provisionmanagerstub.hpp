/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROVISIONMANAGERSTUB_HPP_
#define PROVISIONMANAGERSTUB_HPP_

#include <aos/iam/certprovider.hpp>
#include <aos/iam/provisionmanager.hpp>

namespace aos::zephyr {

/**
 * Cert provider stub.
 */
class CertProviderStub : public iam::certprovider::CertProviderItf {
public:
    /**
     * Returns certificate info.
     *
     * @param certType certificate type.
     * @param issuer issuer name.
     * @param serial serial number.
     * @param[out] resCert result certificate.
     * @returns Error.
     */
    Error GetCert(const String& certType, const Array<uint8_t>& issuer, const Array<uint8_t>& serial,
        iam::certhandler::CertInfo& resCert) const override
    {
        return ErrorEnum::eNone;
    }

    /**
     * Subscribes certificates receiver.
     *
     * @param certType certificate type.
     * @param certReceiver certificate receiver.
     * @returns Error.
     */
    Error SubscribeCertChanged(const String& certType, iam::certhandler::CertReceiverItf& certReceiver) override
    {
        (void)certType;
        (void)certReceiver;

        return ErrorEnum::eNone;
    }

    /**
     * Unsubscribes certificate receiver.
     *
     * @param certReceiver certificate receiver.
     * @returns Error.
     */
    Error UnsubscribeCertChanged(iam::certhandler::CertReceiverItf& certReceiver) override
    {
        (void)certReceiver;

        return ErrorEnum::eNone;
    }
};

/**
 * Provision manager stub.
 */
class ProvisionManagerStub : public iam::provisionmanager::ProvisionManagerItf {
public:
    Error StartProvisioning(const String& password) override
    {
        mPassword = password;

        return ErrorEnum::eNone;
    }

    RetWithError<iam::provisionmanager::CertTypes> GetCertTypes() const { return mCertTypes; }

    Error CreateKey(const String& certType, const String& subject, const String& password, String& csr) override
    {
        mCertType = certType;
        mSubject  = subject;
        mPassword = password;
        csr       = mCSR;

        return ErrorEnum::eNone;
    }

    Error ApplyCert(const String& certType, const String& pemCert, iam::certhandler::CertInfo& certInfo) override
    {
        mCertType = certType;
        mCert     = pemCert;
        certInfo  = mCertInfo;

        return ErrorEnum::eNone;
    }

    Error FinishProvisioning(const String& password) override
    {
        mPassword = password;

        return ErrorEnum::eNone;
    }

    Error Deprovision(const String& password) override
    {
        mPassword = password;

        return ErrorEnum::eNone;
    }

    String GetPassword() const { return mPassword; }
    String GetCertType() const { return mCertType; }
    String GetSubject() const { return mSubject; }
    String GetCSR() const { return mCSR; }
    String GetCert() const { return mCert; }

    void SetCSR(const String& csr) { mCSR = csr; }
    void SetCertTypes(const iam::provisionmanager::CertTypes& certTypes) { mCertTypes = certTypes; }
    void SetCertInfo(const iam::certhandler::CertInfo& certInfo) { mCertInfo = certInfo; }

private:
    StaticString<iam::certhandler::cPasswordLen> mPassword;
    iam::provisionmanager::CertTypes             mCertTypes;
    StaticString<iam::certhandler::cCertTypeLen> mCertType;
    StaticString<crypto::cSubjectCommonNameLen>  mSubject;
    StaticString<crypto::cCSRPEMLen>             mCSR;
    StaticString<crypto::cCertPEMLen>            mCert;
    iam::certhandler::CertInfo                   mCertInfo;
};

} // namespace aos::zephyr

#endif
