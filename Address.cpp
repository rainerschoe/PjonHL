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

#include "Address.hpp"
#include <regex>
#include <stdexcept>

namespace PjonHL
{

// -----------------------------------------------------------------------------
Address::Address()
{
}

// -----------------------------------------------------------------------------
Address::Address(int f_id) :
    id(static_cast<uint8_t>(f_id))
{
}

// -----------------------------------------------------------------------------
Address::Address(uint8_t f_id) :
    id(f_id)
{
}

// -----------------------------------------------------------------------------
Address::Address(const std::string & f_addrString) : Address(f_addrString.c_str())
{
}

// -----------------------------------------------------------------------------
Address::Address(const char* f_addrString)
{
    static const std::regex regex_bus_device_port("(\\d+\\.\\d+\\.\\d+\\.\\d+)\\/(\\d+):(\\d+)");
    static const std::regex regex_bus_device("(\\d+\\.\\d+\\.\\d+\\.\\d+)\\/(\\d+)");
    static const std::regex regex_device_port("(\\d+):(\\d+)");
    static const std::regex regex_device("\\d+");
    
    static const std::regex regex_bus("(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)");

    // default values:
    std::string busStr = "0.0.0.0";
    std::string devStr = "0";
    std::string portStr = "0";

    std::cmatch matches;
    if(std::regex_match(f_addrString, matches, regex_bus_device_port))
    {
        if(matches.size() < 4)
        {
            throw(std::runtime_error("Invalid Address: Expected Bus/Device:Port. " + std::string(f_addrString)));
        }
        busStr = matches[1];
        devStr = matches[2];
        portStr = matches[3];
    }
    else if(std::regex_match(f_addrString, matches, regex_bus_device))
    {
        if(matches.size() < 3)
        {
            throw(std::runtime_error("Invalid Address: Expected Bus/Device. " + std::string(f_addrString)));
        }
        busStr = matches[1];
        devStr = matches[2];
    }
    else if(std::regex_match(f_addrString, matches, regex_device_port))
    {
        if(matches.size() < 3)
        {
            throw(std::runtime_error("Invalid Address: Expected Device:Port. " + std::string(f_addrString)));
        }
        devStr = matches[1];
        portStr = matches[2];
    }
    else if(std::regex_match(f_addrString, matches, regex_device))
    {
        if(matches.size() < 1)
        {
            throw(std::runtime_error("Invalid Address: Expected Device. " + std::string(f_addrString)));
        }
        devStr = matches[0];
    }
    else
    {
            throw(std::runtime_error("Invalid Address format" + std::string(f_addrString)));
    }


    // parse bus:
    std::smatch bus_matches;
    if(not std::regex_match(busStr, bus_matches, regex_bus))
    {
        throw(std::runtime_error("Invalid Address: Error parsing BusId. " + std::string(f_addrString)));
    }
    if(bus_matches.size() < 5)
    {
        throw(std::runtime_error("Invalid Address: Error parsing BusId. " + std::string(f_addrString)));
    }
    for(size_t i = 0; i<4; i++)
    {
        size_t pos = 0;
        long unsigned int value = stoul(bus_matches[i+1], &pos);
        if(pos == 0)
        {
            throw(std::runtime_error("Invalid Address: Error parsing BusId. " + std::string(f_addrString)));
        }
        if(value > 0xff)
        {
            throw(std::runtime_error("Invalid Address: BusId out of range. " + std::string(f_addrString)));
        }
        busId[i] = value;
    }

    // parse device id:
    size_t pos = 0;
    long unsigned int value = stoul(devStr, &pos);
    if(pos == 0)
    {
        throw(std::runtime_error("Invalid Address: Error parsing DeviceId. " + std::string(f_addrString)));
    }
    if(value > 0xff)
    {
        throw(std::runtime_error("Invalid Address: DeviceId out of range. " + std::string(f_addrString)));
    }
    id = value;

    // parse port:
    pos = 0;
    value = stoul(portStr, &pos);
    if(pos == 0)
    {
        throw(std::runtime_error("Invalid Address: Error parsing Port. " + std::string(f_addrString)));
    }
    if(value > 0xffff)
    {
        throw(std::runtime_error("Invalid Address: Port out of range. " + std::string(f_addrString)));
    }
    port = value;
}

// -----------------------------------------------------------------------------
bool Address::matches(Address f_other, Address f_mask) const
{
    bool match = true;
    match = match and maskMatch(id, f_other.id, f_mask.id);
    match = match and maskMatch(port, f_other.port, f_mask.port);
    for(int i = 0; i<4; i++)
    {
        match = match and maskMatch(busId[i], f_other.busId[i], f_mask.busId[i]);
    }
    return match;
}

// -----------------------------------------------------------------------------
std::string Address::toString()
{
    std::string result;
    result += std::to_string(busId[0]) + ".";
    result += std::to_string(busId[1]) + ".";
    result += std::to_string(busId[2]) + ".";
    result += std::to_string(busId[3]) + "/";
    result += std::to_string(static_cast<uint16_t>(id));
    result += ":";
    result += std::to_string(port);
    return result;
}

// -----------------------------------------------------------------------------
Address Address::createAllOneAddress()
{
    Address mask;
    mask.id = 0xff;
    mask.port = 0xffff;
    mask.busId[0] = 0xff;
    mask.busId[1] = 0xff;
    mask.busId[2] = 0xff;
    mask.busId[3] = 0xff;
    return mask;
}

}
