/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TLSVCHANNEL_HPP_
#define TLSVCHANNEL_HPP_

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ssl.h>

#include <aos/common/crypto.hpp>
#include <aos/common/cryptoutils.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/iam/certhandler.hpp>

#include "commchannel.hpp"

class TLSChannel : public CommChannelItf {
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
    void Init(aos::iam::certhandler::CertHandlerItf& certHandler, aos::cryptoutils::CertLoaderItf& certLoader,
        CommChannelItf& vChannel);

    /**
     * Set the secure channel.
     *
     * @param certType Certificate type.
     * @return aos::Error
     */
    aos::Error SetSecure(const aos::String& certType) override;

    /**
     * Connects to communication channel.
     *
     * @param secure specifies if secure connection is required.
     * @return aos::Error.
     */
    aos::Error Connect() override;

    /**
     * Closes current connection.
     */
    aos::Error Close() override;

    /**
     * Returns if channel is connected.
     *
     * @return bool.
     */
    bool IsConnected() const override;

    /**
     * Reads data from channel to array.
     *
     * @param data array where data is placed to.
     * @param size specifies how many bytes to read.
     * @return aos::Error.
     */
    aos::Error Read(aos::Array<uint8_t>& data, size_t size) override;

    /**
     * Writes data from array to channel.
     *
     * @param data array where data is taken from.
     * @param size specifies how many bytes to write.
     * @return aos::Error.
     */
    aos::Error Write(const aos::Array<uint8_t>& data) override;

private:
    static constexpr const char* cPers = "tls_vchannel_client";

    static int TLSWrite(void* ctx, const unsigned char* buf, size_t len);
    static int TLSRead(void* ctx, unsigned char* buf, size_t len);

    aos::Error                              TLSConnect();
    aos::RetWithError<mbedtls_svc_key_id_t> SetupOpaqueKey(mbedtls_pk_context& pk);
    void                                    Cleanup();
    aos::Error                              SetupSSLConfig(const aos::String& certType);

    mbedtls_entropy_context                    mEntropy {};
    mbedtls_ctr_drbg_context                   mCtrDrbg {};
    mbedtls_x509_crt                           mCertChain {};
    mbedtls_x509_crt                           mCACert {};
    mbedtls_pk_context                         mPrivKeyCtx {};
    mbedtls_ssl_config                         mConf {};
    mbedtls_ssl_context                        mSsl {};
    mbedtls_svc_key_id_t                       mKeyID {};
    aos::SharedPtr<aos::crypto::PrivateKeyItf> mPrivKey {};
    CommChannelItf*                            mChannel {};
    aos::iam::certhandler::CertHandlerItf*     mCertHandler {};
    aos::cryptoutils::CertLoaderItf*           mCertLoader {};
    bool                                       mSecure {};
};

#endif /* TLSVCHANNEL_HPP_ */
