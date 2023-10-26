/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>

#include "checksum.hpp"

aos::RetWithError<aos::StaticBuffer<aos::cSHA256Size>> CalculateSha256(const void* data, size_t size)
{
    tc_sha256_state_struct              s;
    aos::StaticBuffer<aos::cSHA256Size> digest;

    auto ret = tc_sha256_init(&s);
    if (TC_CRYPTO_SUCCESS != ret) {
        return {digest, AOS_ERROR_WRAP(ret)};
    }

    ret = tc_sha256_update(&s, static_cast<const uint8_t*>(data), size);
    if (TC_CRYPTO_SUCCESS != ret) {
        return {digest, AOS_ERROR_WRAP(ret)};
    }

    ret = tc_sha256_final(static_cast<uint8_t*>(digest.Get()), &s);
    if (TC_CRYPTO_SUCCESS != ret) {
        return {digest, AOS_ERROR_WRAP(ret)};
    }

    return digest;
}
