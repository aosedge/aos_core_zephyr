/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TLSCHANNEL_HPP_
#define TLSCHANNEL_HPP_

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ssl.h>

#include <aos/common/crypto.hpp>
#include <aos/common/cryptoutils.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/iam/certhandler.hpp>

#include "channel.hpp"
#include "commchannel.hpp"

class TLSChannel : public Channel {
public:
    /**
     * Destructor.
     */
    ~TLSChannel();

    /**
     * Initializes secure channel.
     *
     * @param certHandler certificate handler.
     * @param certLoader certificate loader.
     * @param vChannel virtual channel.
     */
    aos::Error Init(aos::iam::certhandler::CertHandlerItf& certHandler, aos::cryptoutils::CertLoaderItf& certLoader,
        CommunicationItf& communication, int port);

    /**
     * Set TLS config.
     *
     * @param certType certificate type.
     * @return aos::Error
     */
    aos::Error SetTLSConfig(const aos::String& certType);

    /**
     * Connects to communication channel.
     *
     * @return aos::Error.
     */
    aos::Error Connect() override;

    /**
     * Closes current connection.
     */
    aos::Error Close() override;

    /**
     * Reads data from channel to array.
     *
     * @param data buffer where data is placed to.
     * @param size specifies how many bytes to read.
     * @return int num read bytes.
     */
    int Read(void* data, size_t size) override;

    /**
     * Writes data from array to channel.
     *
     * @param data data buffer.
     * @param size specifies how many bytes to write.
     * @return int num written bytes.
     */
    int Write(const void* data, size_t size) override;

private:
    static constexpr auto cPers = "tls_vchannel_client";

    static int TLSWrite(void* ctx, const unsigned char* buf, size_t len);
    static int TLSRead(void* ctx, unsigned char* buf, size_t len);

    aos::Error                              TLSConnect();
    aos::RetWithError<mbedtls_svc_key_id_t> SetupOpaqueKey(mbedtls_pk_context& pk);
    void                                    Cleanup();
    aos::Error                              SetupSSLConfig(const aos::String& certType);

    mbedtls_entropy_context                                mEntropy {};
    mbedtls_ctr_drbg_context                               mCtrDrbg {};
    mbedtls_x509_crt                                       mCertChain {};
    mbedtls_x509_crt                                       mCACert {};
    mbedtls_pk_context                                     mPrivKeyCtx {};
    mbedtls_ssl_config                                     mConf {};
    mbedtls_ssl_context                                    mSSL {};
    mbedtls_svc_key_id_t                                   mKeyID {};
    aos::SharedPtr<aos::crypto::PrivateKeyItf>             mPrivKey {};
    aos::iam::certhandler::CertHandlerItf*                 mCertHandler {};
    aos::cryptoutils::CertLoaderItf*                       mCertLoader {};
    aos::StaticString<aos::iam::certhandler::cCertTypeLen> mCertType;
    aos::Mutex                                             mMutex;
};

#endif /* TLSCHANNEL_HPP_ */
