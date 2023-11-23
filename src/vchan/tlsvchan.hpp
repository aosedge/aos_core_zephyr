/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TLSVCHAN_HPP_
#define TLSVCHAN_HPP_

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ssl.h>

#include "vchan.hpp"

class TLSVchan : public Vchan {
public:
    using FuncDbg = void (*)(void*, int, const char*, int, const char*);

    /**
     * Creates TLSVchan instance.
     */
    TLSVchan(mbedtls_ssl_config& conf, FuncDbg funcDbg = nullptr);

    /**
     * Destructor.
     */
    ~TLSVchan();

    /**
     * Initializes TLSVchan instance.
     *
     * @return Error.
     */
    virtual aos::Error Init() override;

    /**
     * Connects to Vchan.
     *
     * @param domdID domd ID.
     * @param vchanPath Vchan path.
     * @return Error.
     */
    aos::Error Connect(domid_t domdID, const aos::String& vchanPath) override;

    /**
     * Reads data from Vchan.
     *
     * @param buffer buffer to read.
     * @param size buffer size.
     * @return Error.
     */
    aos::Error Read(uint8_t* buffer, size_t size) override;

    /**
     * Writes data to Vchan.
     *
     * @param buffer buffer to write.
     * @param size buffer size.
     * @return Error.
     */
    aos::Error Write(const uint8_t* buffer, size_t size) override;

private:
    static int TlsWrite(void* ctx, const unsigned char* buf, size_t len);
    static int TlsRead(void* ctx, unsigned char* buf, size_t len);

    mbedtls_ssl_config*      mSSLconf {};
    mbedtls_ssl_context      mSSLctx {};
    mbedtls_entropy_context  mEntropy {};
    mbedtls_ctr_drbg_context mCtrDrbg {};
    FuncDbg                  mFuncDbg {};
};

#endif
