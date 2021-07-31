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

#include "PjonHlBus.hpp"

namespace PjonHL
{

// TODO: The following functions are very hard to "inline" into a header only
//       library, as we need to use them as unique callbacks for PJON.
//       Either PJON provides member function callbacks or we cannot make this
//       header only library.

// -----------------------------------------------------------------------------
std::function<void ( uint8_t code, uint16_t data, void *custom_pointer) > & getErrorFunction()
{
    static std::function<void ( uint8_t code, uint16_t data, void *custom_pointer) > function;
    return function;
}

// -----------------------------------------------------------------------------
void globalErrorFunction(
        uint8_t code,
        uint16_t data,
        void *custom_pointer
        )
{
    getErrorFunction()(code,data,custom_pointer);
}

// -----------------------------------------------------------------------------
std::function<void (uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)> & getReceiverFunction()
{
    static std::function<void (uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)> function;
    return function;
}

// -----------------------------------------------------------------------------
void globalReceiverFunction(
        uint8_t *payload,
        uint16_t length,
        const PJON_Packet_Info &packet_info
        )
{
    getReceiverFunction()(payload, length, packet_info);
}

// -----------------------------------------------------------------------------
std::string PjonErrorToString(uint8_t f_errorCode, uint8_t f_data)
{
    switch(f_errorCode)
    {
        case PJON_PACKETS_BUFFER_FULL:
            return "PJON Packet buffer full. Max number of packets in buffer is " + std::to_string(f_data);
        case PJON_CONTENT_TOO_LONG:
            return "PJON Packet content too long. Max Packet size is " + std::to_string(PJON_PACKET_MAX_LENGTH) + " bytes, but given packet has " + std::to_string(f_data) + " bytes.";
        case PJON_CONNECTION_LOST:
            return "PJON Remote device did not ACK. Connection may be lost.";
        default:
            return "Unknown PJON Error code " + std::to_string(f_errorCode);
    }
}
}
