// Copyright 2021 Rainer Schoenberger
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <inttypes.h>
#include <string>
#include <array>
#include <cstring>
#include "PJONDefines.h"
namespace PjonHL
{
struct Address
{
    /// Construct default address:
    /// deviceId will be 0
    /// busId will be 0.0.0.0
    /// port will be PJON_BROADCAST
    Address();

    /// Construct address with given device Id only.
    /// busId will be 0.0.0.0
    /// port will be PJON_BROADCAST
    Address(int f_id);

    /// Construct address with given device Id only.
    /// busId will be 0.0.0.0
    /// port will be PJON_BROADCAST
    Address(uint8_t f_id);

    /// Constructs address from given string representation.
    /// Valid String formats:
    ///   DeviceId
    ///   DeviceId:Port
    ///   BusId/DeviceId
    ///   BusId/DeviceId:Port
    ///
    ///   DeviceId = 0...255
    ///   BusId = B.B.B.B
    ///   B = 0...255
    ///   Port = 0...65536
    /// Examples:
    ///   "42"
    ///   "42:1337"
    ///   "0.0.0.0/42"
    ///   "0.0.0.0/42:1337
    /// 
    /// Throws std::runtime_error if given string is not a valid address.
    /// 
    /// @param f_addrString string to be parsed into an address
    Address(const std::string & f_addrString);

    /// See Address(const std::string & f_addrString). Uses C style string instead.
    Address(const char* f_addrString);

    /// Check if this address matches other address given a mask.
    /// @param f_other: other mask to check against
    /// @param f_mask: address to use as a bitwise mask (only bits equal to 1 are included in a match)
    /// @returns true if this address matched other address given the mask.
    bool matches(Address f_other, Address f_mask) const;

    /// Convert the address into a string representation.
    /// @returns string representation in same format as the string constructor
    ///          also expects for parsing.
    std::string toString();


    /// Constructs an Address with all fields set to maximum value / all ones
    /// in binary.
    /// @returns the Address as described.
    static Address createAllOneAddress();

    /// The device ID
    uint8_t id = 0;

    /// The busId
    std::array<uint8_t,4> busId = {0,0,0,0};

    /// Port
    uint16_t port = PJON_BROADCAST;

    // TODO: Support MAC

    private:
        template<class T>
        static bool maskMatch(T v1, T v2, T mask)
        {
            return ((v1 bitand mask) == (v2 bitand mask));
        }
};
}
