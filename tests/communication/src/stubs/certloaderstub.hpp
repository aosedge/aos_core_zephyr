/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CERTLOADERSTUB_HPP_
#define CERTLOADERSTUB_HPP_

#include <memory>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <psa/crypto.h>

#include <aos/common/crypto.hpp>
#include <aos/common/cryptoutils.hpp>

#include "rsaprivatekey.hpp"

class CertLoaderStub : public aos::cryptoutils::CertLoaderItf {
public:
    aos::Error Init(aos::crypto::x509::ProviderItf& cryptoProvider, aos::pkcs11::PKCS11Manager& pkcs11Manager) override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::RetWithError<aos::SharedPtr<aos::crypto::x509::CertificateChain>> LoadCertsChainByURL(
        const aos::String& url) override
    {

        auto certChain = aos::MakeShared<aos::crypto::x509::CertificateChain>(&mCertAllocator);

        aos::StaticString<aos::crypto::cCertPEMLen> mCertChainPem;

        auto err = aos::FS::ReadFileToString(url, mCertChainPem);
        if (!err.IsNone()) {
            return {nullptr, err};
        }

        aos::crypto::x509::Certificate cert;
        cert.mRaw
            = aos::Array<uint8_t>(reinterpret_cast<const uint8_t*>(mCertChainPem.Get()), mCertChainPem.Size() + 1);

        certChain->PushBack(cert);

        return {certChain, aos::ErrorEnum::eNone};
    }

    aos::RetWithError<aos::SharedPtr<aos::crypto::PrivateKeyItf>> LoadPrivKeyByURL(const aos::String& url) override
    {
        aos::StaticString<aos::crypto::cPrivKeyPEMLen> mPKPem;
        mbedtls_pk_context                             pkContext;
        mbedtls_ctr_drbg_context                       ctrDrbg;

        mbedtls_pk_init(&pkContext);
        mbedtls_ctr_drbg_init(&ctrDrbg);

        std::unique_ptr<mbedtls_ctr_drbg_context, decltype(&mbedtls_ctr_drbg_free)> ctrDrbgPtr(
            &ctrDrbg, mbedtls_ctr_drbg_free);

        auto err = aos::FS::ReadFileToString(url, mPKPem);
        if (!err.IsNone()) {
            return {nullptr, err};
        }

        auto ret = mbedtls_pk_parse_key(&pkContext, reinterpret_cast<const unsigned char*>(mPKPem.Get()),
            mPKPem.Size() + 1, nullptr, 0, mbedtls_ctr_drbg_random, &ctrDrbg);
        if (ret != 0) {
            return {nullptr, ret};
        }

        if (mbedtls_pk_get_type(&pkContext) != MBEDTLS_PK_RSA) {
            return {nullptr, aos::ErrorEnum::eNotSupported};
        }

        mbedtls_rsa_context* rsa_context = mbedtls_pk_rsa(pkContext);

        mbedtls_mpi mpiN, mpiE;

        mbedtls_mpi_init(&mpiN);
        mbedtls_mpi_init(&mpiE);

        std::unique_ptr<mbedtls_mpi, decltype(&mbedtls_mpi_free)> mpiNPtr(&mpiN, mbedtls_mpi_free);
        std::unique_ptr<mbedtls_mpi, decltype(&mbedtls_mpi_free)> mpiEPtr(&mpiE, mbedtls_mpi_free);

        if ((ret = mbedtls_rsa_export(rsa_context, mpiNPtr.get(), nullptr, nullptr, nullptr, mpiEPtr.get())) != 0) {
            return {nullptr, ret};
        }

        aos::StaticArray<uint8_t, aos::crypto::cRSAModulusSize>     mN;
        aos::StaticArray<uint8_t, aos::crypto::cRSAPubExponentSize> mE;

        if ((ret = ConvertMbedtlsMpiToArray(mpiNPtr.get(), mN)) != 0) {
            return {nullptr, ret};
        }

        if ((ret = ConvertMbedtlsMpiToArray(mpiEPtr.get(), mE)) != 0) {
            return {nullptr, ret};
        }

        aos::crypto::RSAPublicKey rsaPublicKey(mN, mE);

        return {aos::MakeShared<RSAPrivateKey>(&mKeyAllocator, rsaPublicKey, pkContext), aos::ErrorEnum::eNone};
    }

private:
    static constexpr auto cCertAllocatorSize = 2 * sizeof(aos::crypto::x509::CertificateChain);
    static constexpr auto cKeyAllocatorSize  = 2 * sizeof(RSAPrivateKey);

    int ConvertMbedtlsMpiToArray(const mbedtls_mpi* mpi, aos::Array<uint8_t>& outArray)
    {
        outArray.Resize(mbedtls_mpi_size(mpi));

        return mbedtls_mpi_write_binary(mpi, outArray.Get(), outArray.Size());
    }

    aos::StaticAllocator<cCertAllocatorSize> mCertAllocator;
    aos::StaticAllocator<cKeyAllocatorSize>  mKeyAllocator;
};

#endif
