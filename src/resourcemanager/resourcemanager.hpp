/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEMANAGER_HPP_
#define RESOURCEMANAGER_HPP_

#include <aos/common/tools/error.hpp>
#include <aos/common/tools/noncopyable.hpp>

/**
 * Resource manager interface.
 */

class ResourceManagerItf {
public:
    /**
     * Destructor
     */
    virtual ~ResourceManagerItf() = default;

    /**
     * Get current unit config version.
     *
     * @param[out] version param to store version.
     * @return aos::Error.
     */
    virtual aos::Error GetUnitConfigInfo(aos::String& version) const = 0;

    /**
     * Check new unit configuration.
     *
     * @param version unit config version
     * @param unitConfig string with unit configuration.
     * @return aos::Error.
     */
    virtual aos::Error CheckUnitConfig(const aos::String& version, const aos::String& unitConfig) const = 0;

    /**
     * Update unit configuration.
     *
     * @param version unit config version.
     * @param unitConfig string with unit configuration.
     * @return aos::Error.
     */
    virtual aos::Error UpdateUnitConfig(const aos::String& version, const aos::String& unitConfig) = 0;
};

/**
 * Resource manager instance.
 */

class ResourceManager : public ResourceManagerItf, private aos::NonCopyable {
public:
    /**
     * Get current unit config version.
     *
     * @param[out] version param to store version.
     * @return aos::Error.
     */
    aos::Error GetUnitConfigInfo(aos::String& version) const override;

    /**
     * Check new unit configuration.
     *
     * @param version unit config version
     * @param unitConfig string with unit configuration.
     * @return aos::Error.
     */
    aos::Error CheckUnitConfig(const aos::String& version, const aos::String& unitConfig) const override;

    /**
     * Update unit configuration.
     *
     * @param version unit config version.
     * @param unitConfig string with unit configuration.
     * @return aos::Error.
     */
    aos::Error UpdateUnitConfig(const aos::String& version, const aos::String& unitConfig) override;

private:
    static constexpr auto cUnitConfigFilePath = CONFIG_AOS_UNIT_CONFIG_FILE;
};

#endif
