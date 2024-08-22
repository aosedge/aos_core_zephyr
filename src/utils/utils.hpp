/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_UTILS_HPP
#define _TEST_UTILS_HPP

#include <aos/common/tools/error.hpp>

namespace aos::zephyr::utils {

/**
 * Creates Aos string from fixed size C string.
 *
 * @tparam cSize max C string size.
 * @param cStr C string.
 * @return String&.
 */
template <size_t cSize>
constexpr String StringFromCStr(char (&cStr)[cSize])
{
    auto str = String(cStr, cSize - 1);

    cStr[cSize - 1] = '\0';
    str.Resize(strlen(cStr));

    return str;
}

/**
 * Creates Aos string from fixed size C string.
 *
 * @tparam cSize max C string size.
 * @param cStr C string.
 * @return String&.
 */
template <size_t cSize>
constexpr const String StringFromCStr(const char (&cStr)[cSize])
{
    auto str = String(cStr, cSize - 1);
    str.Resize(strlen(cStr));

    return str;
}

/**
 * Creates Aos array from fixed size C array.
 *
 * @tparam T C array type.
 * @tparam cSize max C array size.
 * @param arr C array.
 * @return Array<T>.
 */
template <typename T, size_t cSize>
Array<T> ToArray(T (&arr)[cSize])
{
    return Array<T>(arr, cSize);
}

/**
 * Creates Aos array from fixed size C array.
 *
 * @tparam T C array type.
 * @tparam cSize max C array size.
 * @param arr C array.
 * @return Array<T>.
 */
template <typename T, size_t cSize>
const Array<T> ToArray(const T (&arr)[cSize])
{
    return Array<T>(arr, cSize);
}

/**
 * Converts Aos error to string.
 *
 * @param err aos error.
 * @return const char*
 */
const char* ErrorToCStr(const Error& err);

} // namespace aos::zephyr::utils

#endif
