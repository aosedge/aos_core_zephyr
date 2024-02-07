/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <future>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <psa/crypto.h>

#include <aos/common/tools/fs.hpp>

#include "communication/commchannel.hpp"
#include "communication/tlschannel.hpp"

#include "mocks/certhandlermock.hpp"
#include "mocks/certloadermock.hpp"
#include "mocks/rsaprivatekey.hpp"

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

class ClientChannel : public CommChannelItf {
public:
    ClientChannel() { mbedtls_net_init(&mServerFd); }
    ~ClientChannel() { mbedtls_net_free(&mServerFd); }

    aos::Error SetTLSConfig(const aos::String& certType) override { return aos::ErrorEnum::eNone; }

    aos::Error Connect() override
    {
        return mbedtls_net_connect(&mServerFd, "localhost", "4433", MBEDTLS_NET_PROTO_TCP);
    }

    aos::Error Close() override { return aos::ErrorEnum::eNone; }

    bool IsConnected() const override { return true; }

    int Read(void* data, size_t size) override
    {
        return mbedtls_net_recv(&mServerFd, static_cast<unsigned char*>(data), size);
    }

    int Write(const void* data, size_t size) override
    {
        return mbedtls_net_send(&mServerFd, static_cast<const unsigned char*>(data), size);
    }

private:
    mbedtls_net_context mServerFd;
};

class Server {
public:
    Server(std::promise<void> listen)
        : mListen(std::move(listen))
    {
    }

    ~Server()
    {
        mbedtls_net_free(&mClientFd);
        mbedtls_net_free(&mListenFd);
        mbedtls_pk_free(&mPrivKeyCtx);
        mbedtls_x509_crt_free(&mCertChain);
        mbedtls_x509_crt_free(&mCACert);
        mbedtls_ctr_drbg_free(&mCtrDrbg);
        mbedtls_entropy_free(&mEntropy);
        mbedtls_ssl_config_free(&mConf);
        mbedtls_ssl_free(&mSsl);
    }

    aos::Error Run()
    {
        auto err = Init();
        if (!err.IsNone()) {
            return err;
        }

        if ((err = ParseCACert()) != aos::ErrorEnum::eNone) {
            return err;
        }

        if ((err = ParseCertChain()) != aos::ErrorEnum::eNone) {
            return err;
        }

        if ((err = ParsePK()) != aos::ErrorEnum::eNone) {
            return err;
        }

        if ((err = SetupSSLConfig()) != aos::ErrorEnum::eNone) {
            return err;
        }

        if ((err = Bind()) != aos::ErrorEnum::eNone) {
            return err;
        }

        if ((err = Listen()) != aos::ErrorEnum::eNone) {
            return err;
        }

        return err;
    }

private:
    static constexpr auto pers = "ssl_server";

    aos::Error Init()
    {
        mbedtls_ssl_init(&mSsl);
        mbedtls_ssl_config_init(&mConf);
        mbedtls_entropy_init(&mEntropy);
        mbedtls_ctr_drbg_init(&mCtrDrbg);
        mbedtls_x509_crt_init(&mCACert);
        mbedtls_x509_crt_init(&mCertChain);
        mbedtls_pk_init(&mPrivKeyCtx);
        mbedtls_net_init(&mListenFd);
        mbedtls_net_init(&mClientFd);

        return mbedtls_ctr_drbg_seed(
            &mCtrDrbg, mbedtls_entropy_func, &mEntropy, reinterpret_cast<const unsigned char*>(pers), strlen(pers));
    }

    aos::Error ParseCACert()
    {
        aos::StaticString<aos::crypto::cCertPEMLen> mCACertPem;

        auto err = aos::FS::ReadFileToString(CERT_DIR "/ca.pem", mCACertPem);
        if (!err.IsNone()) {
            return err;
        }

        return mbedtls_x509_crt_parse(
            &mCACert, reinterpret_cast<const unsigned char*>(mCACertPem.Get()), mCACertPem.Size() + 1);
    }

    aos::Error ParseCertChain()
    {
        aos::StaticString<aos::crypto::cCertPEMLen> mCertChainPem;

        auto err = aos::FS::ReadFileToString(CERT_DIR "/server.cer", mCertChainPem);
        if (!err.IsNone()) {
            return err;
        }

        return mbedtls_x509_crt_parse(
            &mCertChain, reinterpret_cast<const unsigned char*>(mCertChainPem.Get()), mCertChainPem.Size() + 1);
    }

    aos::Error ParsePK()
    {
        aos::StaticString<4096> mPKPem;

        auto err = aos::FS::ReadFileToString(CERT_DIR "/server.key", mPKPem);
        if (!err.IsNone()) {
            return err;
        }

        return mbedtls_pk_parse_key(&mPrivKeyCtx, reinterpret_cast<const unsigned char*>(mPKPem.Get()),
            mPKPem.Size() + 1, nullptr, 0, mbedtls_ctr_drbg_random, &mCtrDrbg);
    }

    aos::Error SetupSSLConfig()
    {
        auto ret = mbedtls_ssl_config_defaults(
            &mConf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
        if (ret != 0) {
            return ret;
        }

        mbedtls_ssl_conf_authmode(&mConf, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_rng(&mConf, mbedtls_ctr_drbg_random, &mCtrDrbg);
        mbedtls_ssl_conf_ca_chain(&mConf, &mCACert, nullptr);
        mbedtls_ssl_conf_own_cert(&mConf, &mCertChain, &mPrivKeyCtx);

        return mbedtls_ssl_setup(&mSsl, &mConf);
    }

    aos::Error Bind() { return mbedtls_net_bind(&mListenFd, nullptr, "4433", MBEDTLS_NET_PROTO_TCP); }

    aos::Error Listen()
    {
        mbedtls_net_free(&mClientFd);

        mbedtls_ssl_session_reset(&mSsl);

        mListen.set_value();

        auto ret = mbedtls_net_accept(&mListenFd, &mClientFd, nullptr, 0, nullptr);
        if (ret != 0) {
            return ret;
        }

        mbedtls_ssl_set_bio(&mSsl, &mClientFd, mbedtls_net_send, mbedtls_net_recv, nullptr);

        return mbedtls_ssl_handshake(&mSsl);
    }

    mbedtls_ssl_context      mSsl;
    mbedtls_ssl_config       mConf;
    mbedtls_entropy_context  mEntropy;
    mbedtls_ctr_drbg_context mCtrDrbg;
    mbedtls_x509_crt         mCACert;
    mbedtls_x509_crt         mCertChain;
    mbedtls_pk_context       mPrivKeyCtx;
    mbedtls_net_context      mListenFd;
    mbedtls_net_context      mClientFd;
    std::promise<void>       mListen;
};

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(tlschannel, nullptr, nullptr, nullptr, nullptr, nullptr);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(tlschannel, test_TLSChannelConnect)
{
    psa_status_t status = psa_crypto_init();
    zassert_equal(status, PSA_SUCCESS, "psa_crypto_init failed");

    CertLoaderMock  certLoader;
    CertHandlerMock certHandler;
    ClientChannel   vChannel;

    TLSChannel channel;

    auto err = channel.Init(certHandler, certLoader, vChannel);
    zassert_equal(err, aos::ErrorEnum::eNone, "test failed");

    err = channel.SetTLSConfig("client");
    zassert_equal(err, aos::ErrorEnum::eNone, "test failed");

    std::promise<void> listen;
    auto               waitListen = listen.get_future();
    Server             server(std::move(listen));

    auto resServer = std::async(std::launch::async, &Server::Run, &server);
    if (waitListen.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
        zassert_unreachable("test failed");
    }

    auto resClient = std::async(std::launch::async, &TLSChannel::Connect, &channel);

    auto errClient = resClient.get();
    auto errServer = resServer.get();

    zassert_equal(errClient, aos::ErrorEnum::eNone, "test failed");
    zassert_equal(errServer, aos::ErrorEnum::eNone, "test failed");
}
