#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <aos/common/tools/error.hpp>
#include <aosprotocol.h>

#include "utils/checksum.hpp"

class TransportIntf {
public:
    virtual ~TransportIntf() = default;

    virtual aos::Error Connect()                    = 0;
    virtual aos::Error Read(void* buffer, int len)  = 0;
    virtual aos::Error Write(void* buffer, int len) = 0;
};

class ChanIntf {
public:
    virtual ~ChanIntf() = default;

    virtual aos::Error Connect()                             = 0;
    virtual aos::Error Read(void* buffer, int len)           = 0;
    virtual aos::Error Write(void* buffer, int len)          = 0;
    virtual aos::Error WaitRead(void** buffer, size_t* size) = 0;
    virtual aos::Error ReadDone()                            = 0;
    virtual aos::Error Close()                               = 0;
};

class CommunicationItf : public TransportIntf { };

class AosProtocol {
public:
    AosProtocolHeader& PrepareHeader(uint32_t port, const aos::Array<uint8_t>& data)
    {
        mHeader.mPort     = port;
        mHeader.mDataSize = size;

        auto [checksum, err] = CalculateSha256(data);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(mHeader.mChecksum), aos::cSHA256Size) = checksum;

        return mHeader;
    }

private:
    AosProtocolHeader mHeader {};
};

#endif // TYPES_HPP_