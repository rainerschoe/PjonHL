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
#include <future>
#include <vector>
#include "Expect.hpp"
#include "Address.hpp"
#include "PjonHlBus.hpp"

namespace PjonHL
{

template<class Strategy>
class Bus;

template<class Strategy>
class Connection
{
    public:
        /// Schedules transmission of a packet to the remote side of the connection.
        /// Thread safe with respect to other public member functions.
        /// @param f_payload data moved in to be transmitted
        /// @param f_timeout_milliseconds time until which retransmissions are
        ///          attempted before transmission is aborted with an error.
        /// @param f_enableRetransmit not yet implemented, has no effect
        ///        TODO: remove or implement
        /// @returns A future which may be used to check if packet was sent
        ///          successfully or not. A call to .get() will block until the
        ///          result is known for sure (I.e. packet could be sent or
        ///          failed to send).
        std::future<bool> send(
                const std::vector<uint8_t> && f_payload,
                uint32_t f_timeout_milliseconds = 1000,
                bool f_enableRetransmit=true
                );

        /// Receives a packet from the remote side of the connection.
        /// Thread safe with respect to other public member functions.
        /// @param f_timeout_milliseconds Time to block and wait for data to
        ///         become available.
        /// @return Expected packet. Valid and unwrappable if packet was received.
        /// NOTE: Currently there is no way to determine the remote or local
        ///       address of a received packet (relevant if implementing routing)
        ///       This is a known limitation and will be addressed in the future.
        Expect< std::vector<uint8_t> > receive(uint32_t f_timeout_milliseconds = 0);

    private:
        Connection(Address f_remoteAddress, Address f_remoteMask, Address f_localAddress, Address f_localMask, Bus<Strategy> & f_pjonHL);
        void addReceivedPacket(std::vector<uint8_t> && f_packet, Address f_remoteAddress);
        void setInactive();

        std::mutex m_rxQueueMutex;
        std::condition_variable m_rxQueueCondition;
        std::queue< std::vector<uint8_t> > m_rxQueue;

        const Address m_remoteAddress;
        const Address m_remoteMask;
        const Address m_localAddress;
        const Address m_localMask;
        Bus<Strategy> & m_pjonHL;
        std::mutex m_activityMutex;
        bool m_active = true;
        friend Bus<Strategy>;
};
}

#include "Connection.inl"
