/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern "C" {
#include <zephyr_mbedtls_priv.h>
}

#include <mbedtls/debug.h>
#include <psa/crypto.h>

#include <aos/common/crypto/mbedtls/driverwrapper.hpp>
#include <aos/common/cryptoutils.hpp>
#include <aos/iam/certhandler.hpp>

#include "log.hpp"
#include "tlschannel.hpp"

extern unsigned char __aos_root_ca_start[];
extern unsigned char __aos_root_ca_end[];

namespace aos::zephyr::communication {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

TLSChannel::~TLSChannel()
{
    Cleanup();
}

Error TLSChannel::Init(const String& name, iam::certhandler::CertHandlerItf& certHandler,
    cryptoutils::CertLoaderItf& certLoader, ChannelItf& channel)
{
    mName        = name;
    mChannel     = &channel;
    mCertHandler = &certHandler;
    mCertLoader  = &certLoader;

    LOG_DBG() << "Init TLS channel: name=" << mName;

    return ErrorEnum::eNone;
}

Error TLSChannel::SetTLSConfig(const String& certType)
{
    LOG_DBG() << "Set TLS config: name=" << mName << ", certType=" << certType;

    Cleanup();

    if (certType.IsEmpty()) {
        return ErrorEnum::eNone;
    }

    if (auto err = SetupSSLConfig(certType); !err.IsNone()) {
        return err;
    }

    mCertType = certType;

    return ErrorEnum::eNone;
}

Error TLSChannel::Connect()
{
    LOG_DBG() << "Connect TLS channel: name=" << mName;

    if (mCertType.IsEmpty()) {
        return mChannel->Connect();
    }

    auto err = TLSConnect();
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return mbedtls_ssl_get_verify_result(&mSSL);
}

Error TLSChannel::Close()
{
    LOG_DBG() << "Close TLS channel: name=" << mName;

    return mChannel->Close();
}

bool TLSChannel::IsConnected() const
{
    return mChannel->IsConnected();
}

int TLSChannel::Read(void* data, size_t size)
{
    if (mCertType.IsEmpty()) {
        return mChannel->Read(data, size);
    }

    return mbedtls_ssl_read(&mSSL, static_cast<unsigned char*>(data), size);
}

int TLSChannel::Write(const void* data, size_t size)
{
    if (mCertType.IsEmpty()) {
        return mChannel->Write(data, size);
    }

    return mbedtls_ssl_write(&mSSL, static_cast<const unsigned char*>(data), size);
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

RetWithError<mbedtls_svc_key_id_t> TLSChannel::SetupOpaqueKey(mbedtls_pk_context& pk)
{
    auto statusAddKey = AosPsaAddKey(*mPrivKey);
    if (!statusAddKey.mError.IsNone()) {
        return {0, statusAddKey.mError};
    }

    auto ret = mbedtls_pk_setup_opaque(&pk, statusAddKey.mValue.mKeyID);
    if (ret != 0) {
        AosPsaRemoveKey(statusAddKey.mValue.mKeyID);

        return {statusAddKey.mValue.mKeyID, AOS_ERROR_WRAP(ret)};
    }

    return {statusAddKey.mValue.mKeyID, ErrorEnum::eNone};
}

Error TLSChannel::TLSConnect()
{
    mbedtls_ssl_session_reset(&mSSL);

    auto err = mChannel->Connect();
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return mbedtls_ssl_handshake(&mSSL);
}

void TLSChannel::Cleanup()
{
    mbedtls_x509_crt_free(&mCertChain);
    mbedtls_x509_crt_free(&mCACert);
    mbedtls_pk_free(&mPrivKeyCtx);
    mbedtls_entropy_free(&mEntropy);
    mbedtls_ctr_drbg_free(&mCtrDrbg);
    mbedtls_ssl_config_free(&mConf);
    mbedtls_ssl_free(&mSSL);

    mPrivKey.Reset();

    if (mKeyID != 0) {
        AosPsaRemoveKey(mKeyID);
    }
}

Error TLSChannel::SetupSSLConfig(const String& certType)
{
    mbedtls_x509_crt_init(&mCertChain);
    mbedtls_x509_crt_init(&mCACert);
    mbedtls_pk_init(&mPrivKeyCtx);
    mbedtls_entropy_init(&mEntropy);
    mbedtls_ctr_drbg_init(&mCtrDrbg);
    mbedtls_ssl_config_init(&mConf);
    mbedtls_ssl_init(&mSSL);

    iam::certhandler::CertInfo certInfo;

    auto err = mCertHandler->GetCertificate(certType, Array<uint8_t>(), Array<uint8_t>(), certInfo);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto retCert = mCertLoader->LoadCertsChainByURL(certInfo.mCertURL);
    if (!retCert.mError.IsNone()) {
        return AOS_ERROR_WRAP(retCert.mError);
    }

    for (auto& cert : *retCert.mValue) {
        auto ret = mbedtls_x509_crt_parse(&mCertChain, cert.mRaw.Get(), cert.mRaw.Size());
        if (ret != 0) {
            return AOS_ERROR_WRAP(ret);
        }
    }

    // cppcheck-suppress comparePointers
    auto ret = mbedtls_x509_crt_parse(&mCACert, __aos_root_ca_start, __aos_root_ca_end - __aos_root_ca_start);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    auto retKey = mCertLoader->LoadPrivKeyByURL(certInfo.mKeyURL);
    if (!retKey.mError.IsNone()) {
        return AOS_ERROR_WRAP(retKey.mError);
    }

    mPrivKey = retKey.mValue;

    auto retOpaqueKey = SetupOpaqueKey(mPrivKeyCtx);
    if (!retOpaqueKey.mError.IsNone()) {
        return AOS_ERROR_WRAP(retOpaqueKey.mError);
    }

    mKeyID = retOpaqueKey.mValue;

    if ((ret = mbedtls_ctr_drbg_seed(
             &mCtrDrbg, mbedtls_entropy_func, &mEntropy, reinterpret_cast<const unsigned char*>(cPers), strlen(cPers)))
        != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    if ((ret = mbedtls_ssl_config_defaults(
             &mConf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT))
        != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mbedtls_ssl_conf_authmode(&mConf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(&mConf, mbedtls_ctr_drbg_random, &mCtrDrbg);

    mbedtls_ssl_conf_ca_chain(&mConf, &mCACert, nullptr);

    if ((ret = mbedtls_ssl_conf_own_cert(&mConf, &mCertChain, &mPrivKeyCtx)) != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mbedtls_ssl_set_bio(&mSSL, this, TLSChannel::TLSWrite, TLSChannel::TLSRead, nullptr);

#if defined(CONFIG_MBEDTLS_DEBUG)
    mbedtls_ssl_conf_dbg(&mConf, zephyr_mbedtls_debug, nullptr);
#endif

    if (ret = mbedtls_ssl_setup(&mSSL, &mConf); ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Static functions
 **********************************************************************************************************************/

int TLSChannel::TLSWrite(void* ctx, const unsigned char* buf, size_t len)
{
    auto channel = static_cast<TLSChannel*>(ctx);

    return channel->mChannel->Write(buf, len);
}

int TLSChannel::TLSRead(void* ctx, unsigned char* buf, size_t len)
{
    auto channel = static_cast<TLSChannel*>(ctx);

    return channel->mChannel->Read(buf, len);
}

} // namespace aos::zephyr::communication
