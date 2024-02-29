/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RSAPRIVATEKEY_HPP_
#define RSAPRIVATEKEY_HPP_

#include <memory>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>

#include <aos/common/crypto.hpp>

class RSAPrivateKey : public aos::crypto::PrivateKeyItf {
public:
    RSAPrivateKey(const aos::crypto::RSAPublicKey& publicKey, mbedtls_pk_context pkContext)
        : mPublicKey(publicKey)
        , mPkContext(pkContext)
    {
    }

    ~RSAPrivateKey() { mbedtls_pk_free(&mPkContext); }

    const aos::crypto::PublicKeyItf& GetPublic() const { return mPublicKey; }

    aos::Error Sign(const aos::Array<uint8_t>& digest, const aos::crypto::SignOptions& options,
        aos::Array<uint8_t>& signature) const override
    {
        mbedtls_entropy_context  entropy;
        mbedtls_ctr_drbg_context ctrDrbg;
        const char*              pers = "rsa_sign";

        mbedtls_ctr_drbg_init(&ctrDrbg);
        mbedtls_entropy_init(&entropy);

        std::unique_ptr<mbedtls_ctr_drbg_context, decltype(&mbedtls_ctr_drbg_free)> ctrDrbgPtr(
            &ctrDrbg, mbedtls_ctr_drbg_free);

        std::unique_ptr<mbedtls_entropy_context, decltype(&mbedtls_entropy_free)> entropyPtr(
            &entropy, mbedtls_entropy_free);

        auto ret = mbedtls_ctr_drbg_seed(
            ctrDrbgPtr.get(), mbedtls_entropy_func, entropyPtr.get(), (const unsigned char*)pers, strlen(pers));
        if (ret != 0) {
            return ret;
        }

        size_t signatureLen;
        auto   pkContext = mPkContext;

        ret = mbedtls_pk_sign(&pkContext, MBEDTLS_MD_SHA256, digest.Get(), digest.Size(), signature.Get(),
            signature.Size(), &signatureLen, mbedtls_ctr_drbg_random, ctrDrbgPtr.get());
        if (ret != 0) {
            return ret;
        }

        signature.Resize(signatureLen);

        return aos::ErrorEnum::eNone;
    }

    aos::Error Decrypt(const aos::Array<unsigned char>&, aos::Array<unsigned char>&) const
    {
        return aos::ErrorEnum::eNotSupported;
    }

public:
    aos::crypto::RSAPublicKey mPublicKey;
    mbedtls_pk_context        mPkContext;
};

#endif /* RSAPRIVATEKEY_HPP_ */
