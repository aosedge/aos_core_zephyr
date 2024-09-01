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

namespace aos::zephyr::communication {

class TLSChannel : public ChannelItf {
public:
    /**
     * Destructor.
     */
    ~TLSChannel();

    /**
     * Initializes secure channel.
     *
     * @param name TLS channel name.
     * @param certHandler certificate handler.
     * @param certLoader certificate loader.
     * @param channel virtual channel.
     */
    Error Init(const String& name, iam::certhandler::CertHandlerItf& certHandler,
        cryptoutils::CertLoaderItf& certLoader, ChannelItf& channel);

    /**
     * Set TLS config.
     *
     * @param certType certificate type.
     * @return Error
     */
    Error SetTLSConfig(const String& certType);

    /**
     * Connects to communication channel.
     *
     * @return Error.
     */
    Error Connect() override;

    /**
     * Closes current connection.
     */
    Error Close() override;

    /**
     * Returns if channel is connected.
     *
     * @return bool.
     */
    bool IsConnected() const override;

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
    static constexpr auto cPers    = "tls_vchannel_client";
    static constexpr auto cNameLen = 64;

    static int TLSWrite(void* ctx, const unsigned char* buf, size_t len);
    static int TLSRead(void* ctx, unsigned char* buf, size_t len);

    Error                              TLSConnect();
    RetWithError<mbedtls_svc_key_id_t> SetupOpaqueKey(mbedtls_pk_context& pk);
    void                               Cleanup();
    Error                              SetupSSLConfig(const String& certType);

    StaticString<cNameLen>                       mName;
    mbedtls_entropy_context                      mEntropy {};
    mbedtls_ctr_drbg_context                     mCtrDrbg {};
    mbedtls_x509_crt                             mCertChain {};
    mbedtls_x509_crt                             mCACert {};
    mbedtls_pk_context                           mPrivKeyCtx {};
    mbedtls_ssl_config                           mConf {};
    mbedtls_ssl_context                          mSSL {};
    mbedtls_svc_key_id_t                         mKeyID {};
    SharedPtr<crypto::PrivateKeyItf>             mPrivKey {};
    ChannelItf*                                  mChannel {};
    iam::certhandler::CertHandlerItf*            mCertHandler {};
    cryptoutils::CertLoaderItf*                  mCertLoader {};
    StaticString<iam::certhandler::cCertTypeLen> mCertType;
};

} // namespace aos::zephyr::communication

#endif /* TLSCHANNEL_HPP_ */
