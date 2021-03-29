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
#include <mutex>
#include <string>
#include <thread>
#include <future>
#include <vector>
#include <queue>
#include <list>
#include <iostream>
#include <functional>
#include <cstdlib>

#include "Expect.hpp"
#include "Address.hpp"
#include "Connection.hpp"

template<class Strategy>
class PJON;

namespace PjonHL
{
template<class Strategy>
class Connection;

template<class Strategy>
class Bus
{
    public:
        ~Bus();

        /// Constructs an instance of Bus.
        /// @param f_localAddress address which will be used in outgoing packets,
        ///         if no local address was explicitly specified for the connection.
        ///         It will also be used to filter incoming packets.
        /// @param f_strategy the underlying physical bus strategy to use.
        ///         e.g. an instance of ThroughSerial
        Bus(Address f_localAddress, Strategy f_strategy);

        /// Handle which will be returned by createConnection() calls.
        /// Holds ownership of a connection and can be used to send/receive packets.
        using ConnectionHandle = std::unique_ptr<Connection<Strategy>, std::function<void(Connection<Strategy>*)> >;

        /// Creates a connection which can be used to send/receive packets to/from
        /// a remote counterpart.
        /// - Packets will be sent from the local Address passed in the
        ///   constructor to the given f_remoteAddress.
        /// - Packets are received from this connection only, if both
        ///     1. The sender address matches the given f_remoteAddress masked by
        ///        f_remoteMask.
        ///     2. The target address exactly matches the local address given in
        ///        the constructor
        /// @param f_remoteAddress Address of the remote counterpart.
        /// @param f_remoteMask Incoming packets have to match f_remoteAddress
        ///         combined with f_remoteMask
        ConnectionHandle createConnection(
                Address f_remoteAddress,
                Address f_remoteMask = Address::createAllOneAddress()
                );

        /// Creates a connection which can be used to send/receive packets to/from
        /// a remote counterpart. The local address passed in the constructor
        /// of bus has no effect for this connection.
        /// It is useful to react to broadcasts or forward packets (routing).
        /// - Packets will be sent from f_localAddress to the given
        ///   f_remoteAddress.
        /// - Packets are received from this connection only, if both
        ///     1. The sender address matches the given f_remoteAddress masked by
        ///        f_remoteMask.
        ///     2. The target address matches matches the given f_localAddress
        ///        masked by f_localMask.
        ConnectionHandle createDetachedConnection(
                Address f_remoteAddress,
                Address f_localAddress,
                Address f_remoteMask = Address::createAllOneAddress(),
                Address f_localMask = Address::createAllOneAddress()
                );

        /// Sends a packet without a connection. Should not be used in normal
        /// operation.
        /// Prefer using connections for sending/receiving instead of this
        /// send() function.
        std::future<bool> send(
                Address f_localAddress,
                Address f_remoteAddress,
                const std::vector<uint8_t> & f_payload,
                uint32_t f_timeout_milliseconds,
                bool f_enableRetransmit = true
                );

    private:
        struct TxRequest
        {
            std::promise<bool> m_successPromise;
            std::vector<uint8_t> m_payload;
            Address m_localAddress;
            Address m_remoteAddress;
            uint32_t m_timeoutMilliseconds;
            bool m_retransmitEnabled;
            size_t m_pjonPacketBufferIndex;
            bool m_dispatched = false;
        };

        void pjonErrorHandler(uint8_t code, uint16_t data, void *custom_pointer);

        void pjonReceiveFunction(
                uint8_t *payload,
                uint16_t length,
                const PJON_Packet_Info &packet_info
                );

        void pjonEventLoop();

        void dispatchTxRequest(TxRequest & f_request);

        std::mutex m_txQueueMutex;
        std::queue< TxRequest > m_txQueue;

        Address m_localAddress;
        PJON<Strategy> m_pjon;

        std::mutex m_connections_mutex;
        std::list<Connection<Strategy>*> m_connections;

        std::thread m_eventLoopThread;

        std::atomic<bool> m_eventLoopRunning = true;
};
}

#include "Bus.inl"
