/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_ZEPHYR_NETWORKMANAGER_HPP_
#define AOS_ZEPHYR_NETWORKMANAGER_HPP_

#include "aos/sm/networkmanager.hpp"

namespace aos::zephyr::networkmanager {

/**
 * Network manager.
 */
class NetworkManager : public sm::networkmanager::NetworkManagerItf {
public:
    /**
     * Returns instance's network namespace path.
     *
     * @param instanceID instance ID.
     * @return RetWithError<StaticString<cFilePathLen>>.
     */
    RetWithError<StaticString<cFilePathLen>> GetNetnsPath(const String& instanceID) const override
    {
        (void)instanceID;

        return {{}, ErrorEnum::eNone};
    }

    /**
     * Updates networks.
     *
     * @param networks network parameters.
     * @return Error.
     */
    Error UpdateNetworks(const Array<aos::NetworkParameters>& networks) override
    {
        (void)networks;

        return ErrorEnum::eNone;
    }

    /**
     * Adds instance to network.
     *
     * @param instanceID instance ID.
     * @param networkID network ID.
     * @param instanceNetworkParameters instance network parameters.
     * @return Error.
     */
    Error AddInstanceToNetwork(const String& instanceID, const String& networkID,
        const sm::networkmanager::InstanceNetworkParameters& instanceNetworkParameters) override
    {
        (void)instanceID;
        (void)networkID;
        (void)instanceNetworkParameters;

        return ErrorEnum::eNone;
    }

    /**
     * Removes instance from network.
     *
     * @param instanceID instance ID.
     * @param networkID network ID.
     * @return Error.
     */
    Error RemoveInstanceFromNetwork(const String& instanceID, const String& networkID) override
    {
        (void)instanceID;
        (void)networkID;

        return ErrorEnum::eNone;
    }

    /**
     * Returns instance's IP address.
     *
     * @param instanceID instance ID.
     * @param networkID network ID.
     * @param[out] ip instance's IP address.
     * @return Error.
     */
    Error GetInstanceIP(const String& instanceID, const String& networkID, String& ip) const override
    {
        (void)instanceID;
        (void)networkID;
        (void)ip;

        return ErrorEnum::eNone;
    }

    /**
     * Returns instance's traffic.
     *
     * @param instanceID instance ID.
     * @param[out] inputTraffic instance's input traffic.
     * @param[out] outputTraffic instance's output traffic.
     * @return Error.
     */
    Error GetInstanceTraffic(const String& instanceID, uint64_t& inputTraffic, uint64_t& outputTraffic) const override
    {
        (void)instanceID;
        (void)inputTraffic;
        (void)outputTraffic;

        return ErrorEnum::eNone;
    }

    /**
     * Gets system traffic.
     *
     * @param[out] inputTraffic system input traffic.
     * @param[out] outputTraffic system output traffic.
     * @return Error.
     */
    Error GetSystemTraffic(uint64_t& inputTraffic, uint64_t& outputTraffic) const override
    {
        (void)inputTraffic;
        (void)outputTraffic;

        return ErrorEnum::eNone;
    }

    /**
     * Sets the traffic period.
     *
     * @param period traffic period.
     * @return Error
     */
    Error SetTrafficPeriod(sm::networkmanager::TrafficPeriod period) override
    {
        (void)period;

        return ErrorEnum::eNone;
    }
};

} // namespace aos::zephyr::networkmanager

#endif
