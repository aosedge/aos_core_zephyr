/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tlsvchan.hpp"

TLSVchan::TLSVchan(mbedtls_ssl_config& conf, FuncDbg funcDbg)
    : mSSLconf(&conf)
    , mFuncDbg(funcDbg)
{
    mbedtls_ssl_init(&mSSLctx);
    mbedtls_entropy_init(&mEntropy);
    mbedtls_ctr_drbg_init(&mCtrDrbg);
}

TLSVchan::~TLSVchan()
{
    mbedtls_ssl_free(&mSSLctx);
    mbedtls_entropy_free(&mEntropy);
    mbedtls_ctr_drbg_free(&mCtrDrbg);
}

aos::Error TLSVchan::Init()
{
    auto err = Vchan::Init();
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    auto ret = mbedtls_ctr_drbg_seed(&mCtrDrbg, mbedtls_entropy_func, &mEntropy, nullptr, 0);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mbedtls_ssl_conf_rng(mSSLconf, mbedtls_ctr_drbg_random, &mCtrDrbg);

    ret = mbedtls_ssl_setup(&mSSLctx, mSSLconf);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mbedtls_ssl_set_bio(&mSSLctx, this, TlsWrite, TlsRead, nullptr);

    if (mFuncDbg != nullptr) {
        mbedtls_ssl_conf_dbg(mSSLconf, mFuncDbg, nullptr);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error TLSVchan::Connect(domid_t domdID, const aos::String& vchanPath)
{
    auto err = Vchan::Connect(domdID, vchanPath);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    auto ret = mbedtls_ssl_handshake(&mSSLctx);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error TLSVchan::Read(uint8_t* buffer, size_t size)
{
    auto ret = mbedtls_ssl_read(&mSSLctx, buffer, size);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error TLSVchan::Write(const uint8_t* buffer, size_t size)
{
    auto ret = mbedtls_ssl_write(&mSSLctx, buffer, size);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    return aos::ErrorEnum::eNone;
}

int TLSVchan::TlsWrite(void* ctx, const unsigned char* buf, size_t len)
{
    auto vchan = static_cast<TLSVchan*>(ctx);

    auto err = vchan->Vchan::Write(buf, len);

    return err == aos::ErrorEnum::eNone ? len : MBEDTLS_ERR_SSL_WANT_WRITE;
}

int TLSVchan::TlsRead(void* ctx, unsigned char* buf, size_t len)
{
    auto vchan = static_cast<TLSVchan*>(ctx);

    auto err = vchan->Vchan::Read(buf, len);

    return err == aos::ErrorEnum::eNone ? len : MBEDTLS_ERR_SSL_WANT_READ;
}
