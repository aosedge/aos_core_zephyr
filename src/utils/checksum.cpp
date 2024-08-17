/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mbedtls/sha256.h>

#include "checksum.hpp"

namespace aos::zephyr::utils {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

RetWithError<StaticArray<uint8_t, cSHA256Size>> CalculateSha256(const Array<uint8_t>& data)
{
    mbedtls_sha256_context            ctx;
    StaticArray<uint8_t, cSHA256Size> digest;

    digest.Resize(cSHA256Size);

    if (data.Size() == 0) {
        return digest;
    }

    mbedtls_sha256_init(&ctx);

    auto ret = mbedtls_sha256_starts(&ctx, 0);
    if (ret != 0) {
        mbedtls_sha256_free(&ctx);

        return {digest, AOS_ERROR_WRAP(ret)};
    }

    ret = mbedtls_sha256_update(&ctx, data.Get(), data.Size());
    if (ret != 0) {
        mbedtls_sha256_free(&ctx);

        return {digest, AOS_ERROR_WRAP(ret)};
    }

    ret = mbedtls_sha256_finish(&ctx, static_cast<uint8_t*>(digest.Get()));
    if (ret != 0) {
        mbedtls_sha256_free(&ctx);

        return {digest, AOS_ERROR_WRAP(ret)};
    }

    mbedtls_sha256_free(&ctx);

    return digest;
}

} // namespace aos::zephyr::utils
