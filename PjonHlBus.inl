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

#include <functional>
#include <inttypes.h>
#include <mutex>

namespace PjonHL
{

// ############################################################################
// ## Implementation of Bus and support classes below:
// ############################################################################

// -----------------------------------------------------------------------------
std::function<void ( uint8_t code, uint16_t data, void *custom_pointer) > & getErrorFunction();

void globalErrorFunction(
        uint8_t code,
        uint16_t data,
        void *custom_pointer
        );
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
std::function<void (uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)> & getReceiverFunction();

void globalReceiverFunction(
        uint8_t *payload,
        uint16_t length,
        const PJON_Packet_Info &packet_info
        );

// -----------------------------------------------------------------------------

template<class Strategy>
Bus<Strategy>::~Bus()
{
    std::lock_guard<std::mutex> guard(m_connections_mutex);
    for(Connection<Strategy>* connection: m_connections)
    {
        connection->setInactive();
    }

    m_eventLoopRunning = false;

    m_eventLoopThread.join();
    getErrorFunction() = std::function<void ( uint8_t code, uint16_t data, void *custom_pointer) >();
}

template<class Strategy>
Bus<Strategy>::Bus(Address f_localAddress, Strategy f_strategy, BusConfig f_config) :
    m_pjon(f_localAddress.busId.data(), f_localAddress.id)
{
    m_localAddress = f_localAddress;
    m_pjon.strategy = f_strategy;

    // load config:
    m_pjon.set_acknowledge(f_config.ackType == BusConfig::AckType::AckEnabled);
    m_pjon.set_crc_32(f_config.crcType == BusConfig::CrcType::Crc32);
    m_pjon.set_communication_mode(f_config.communicationMode == BusConfig::CommunicationMode::HalfDuplex);
    m_pjon.set_shared_network(f_config.busTopology == BusConfig::BusTopology::Shared);

    // TODO: Currently PJON does not provide any interface to set a "callable" object
    //       Or a member function as error / receiver function.
    //       Only Raw function pointers are supported.
    //       This limits PjonHl to one possible instance and requires the hack below
    //       to forward calls into the PjonHl instance.
    //       Maybe PJON can be extended to allow a more generic way of storing
    //       error and receiver functions.

    // safety check: currently only one instance is allowed:
    if(getErrorFunction())
    {
        // already function is set.
        std::cout << "Error: Currently only one instance of Bus is allowed due to limitation in PJON backend." << std::endl;
        exit(1);

    }

    // set up global functions to forward calls into our member functions:

    getErrorFunction() = [this](
            uint8_t code,
            uint16_t data,
            void *custom_pointer
            )
        {
            pjonErrorHandler(code,data,custom_pointer);
        };
    m_pjon.set_error(&globalErrorFunction);

    getReceiverFunction() = [this](
            uint8_t *payload,
            uint16_t length,
            const PJON_Packet_Info &packet_info
            )
        {
            pjonReceiveFunction(payload, length, packet_info);
        };
    m_pjon.set_receiver(&globalReceiverFunction);

    // start up pjon and our event loop thread:
    m_pjon.begin();
    m_eventLoopThread = std::thread([this]{pjonEventLoop();});
}

template<class Strategy>
typename Bus<Strategy>::ConnectionHandle Bus<Strategy>::createDetachedConnection(Address f_remoteAddress, Address f_localAddress, Address f_remoteMask, Address f_localMask)
{
    std::lock_guard<std::mutex> guard(m_connections_mutex);
    ConnectionHandle connection = ConnectionHandle(
            new Connection<Strategy>(f_remoteAddress, f_remoteMask, f_localAddress, f_localMask, *this),
            [this](Connection<Strategy> * f_connection)
            {
                {
                    // lock the connection, to ensure Bus is not changing
                    // connection while we dereference it:
                    std::lock_guard<std::mutex> connectionLockGuard(f_connection->m_activityMutex);
                    if(f_connection->m_active)
                    {
                        // only delete reference in Bus parent if still active
                        // (i.e. Bus is still alive)
                        std::lock_guard<std::mutex> guard(m_connections_mutex);
                        m_connections.remove(f_connection);
                    }
                }
                delete f_connection;
            }
            );
    m_connections.push_back(connection.get());

    return connection;
}

template<class Strategy>
typename Bus<Strategy>::ConnectionHandle Bus<Strategy>::createConnection(Address f_remoteAddress, Address f_remoteMask)
{
    return createDetachedConnection(f_remoteAddress, m_localAddress, f_remoteMask, Address::createAllOneAddress());
}

template<class Strategy>
std::future<Result> Bus<Strategy>::send(Address f_localAddress, Address f_remoteAddress, const std::vector<uint8_t> & f_payload, uint32_t f_timeout_milliseconds, bool f_enableRetransmit)
{
    TxRequest request;
    request.m_payload = f_payload;
    request.m_localAddress = f_localAddress;
    request.m_remoteAddress = f_remoteAddress;
    request.m_timeoutMilliseconds = f_timeout_milliseconds;
    request.m_retransmitEnabled = f_enableRetransmit;

    auto future = request.m_successPromise.get_future();
    std::lock_guard<std::recursive_mutex> guard(m_txQueueMutex);
    m_txQueue.push(std::move(request));

    return future;
}

template<class Strategy>
void Bus<Strategy>::pjonErrorHandler(uint8_t code, uint16_t data, void *custom_pointer)
{
    // TODO: currently we discard `code` here and just propagate a boolean to
    //       the future. Maybe later we should give more elaborate information
    //       to the user through the future (maybe a result class with some
    //       human readable error information paired with a machine readable
    //       success or failure information)
    {
        std::lock_guard<std::recursive_mutex> guard(m_txQueueMutex);
        if(m_txQueue.size()>0 and (m_txQueue.front().m_dispatched == true))
        {
            if(m_txQueue.front().m_pjonPacketBufferIndex == data)
            {
                m_txQueue.front().m_successPromise.set_value(Result(PjonErrorToString(code, data)));
                m_txQueue.pop();
            }
        }
    }
}

template<class Strategy>
void Bus<Strategy>::pjonReceiveFunction(
        uint8_t *payload,
        uint16_t length,
        const PJON_Packet_Info &packet_info
        )
{
    Address remoteAddr;
    remoteAddr.id = packet_info.tx.id;
#if(PJON_INCLUDE_PORT)
    remoteAddr.port = packet_info.port;
#else
    remoteAddr.port = 0;
#endif
    remoteAddr.busId[0] = packet_info.tx.bus_id[0];
    remoteAddr.busId[1] = packet_info.tx.bus_id[1];
    remoteAddr.busId[2] = packet_info.tx.bus_id[2];
    remoteAddr.busId[3] = packet_info.tx.bus_id[3];

    Address targetAddr;
    targetAddr.id = packet_info.rx.id;
#if(PJON_INCLUDE_PORT)
    targetAddr.port = packet_info.port;
#else
    targetAddr.port = 0;
#endif
    targetAddr.busId[0] = packet_info.rx.bus_id[0];
    targetAddr.busId[1] = packet_info.rx.bus_id[1];
    targetAddr.busId[2] = packet_info.rx.bus_id[2];
    targetAddr.busId[3] = packet_info.rx.bus_id[3];

    std::lock_guard<std::mutex> connections_guard(m_connections_mutex);
    for(auto connection : m_connections)
    {
        // if more than one connection is interested in a packet, the packet
        // gets placed in the rx queue of both connections.
        if(
            connection->m_remoteAddress.matches(remoteAddr, connection->m_remoteMask)
            and
            connection->m_localAddress.matches(targetAddr, connection->m_localMask)
          )
        {
            connection->addReceivedPacket(std::vector<uint8_t>(payload, payload + length), remoteAddr);
        }
    }

    m_lastRxTxActivity = std::chrono::steady_clock::now();
}

template<class Strategy>
void Bus<Strategy>::pjonEventLoop()
{
    while(m_eventLoopRunning)
    {
        // first do tx queue dispatch if required:
        {
            std::lock_guard<std::recursive_mutex> guard(m_txQueueMutex);
            if((m_txQueue.size()>0) and (m_txQueue.front().m_dispatched == false))
            {
                // have a new packet to transmit and all previous packets are sent
                // or failed
                // NOTE:  dispatch might call error handler.
                //        this will then lead to recursive mutex lock.
                //        This is why m_txQueue is a recursive mutex
                dispatchTxRequest(m_txQueue.front());
                if(not m_txQueue.front().m_dispatched)
                {
                    // dispatch failed (most likely packet size too big)
                    // -> communicate to user and drop packet:
                    // TODO: more concrete error message?
                    m_txQueue.front().m_successPromise.set_value(Result("Dispatching failed, Most likely packet size too big."));
                    m_txQueue.pop();
                }

                // NOTE: For now we only support one packet being in transit
                //       at a time.
                //       To be more efficient we could only guarantee sequential
                //       delivery of packets part of one connection. then we can
                //       have more packets in pjon list at a time.
            }
        }

        // now give PJON a change to handle its internal state machine and
        // transmit packets:
        m_pjon.update();

        // After each update we might have sent packets.
        // Check if the packet we currently are interested in sending (if any)
        // was sent:

        // TODO: this is accessing rather internal members of PJON, namely
        // the m_pjon.packets[].state variable in PJON's internal packet queue.
        // However I do not see any other way of retrieving success/failure
        // information because:
        // - error callback only notifies on error, not on success
        // - calling blocking_send_packet() does not work,
        //    1. as we would block receiving packets during the attempts 
        //    2. if we solve 1 by running rx in separate thread we introduce
        //       race, as rx does call update()
        // - cannot use returned value by update, as it might be influenced by
        //   async ACKs which were dispatched by receive()
        // To fix this, I need to discuss with PJON authors what the recommended
        // and stable strategy is to find out if a packet was sent or not.

        {
            std::lock_guard<std::recursive_mutex> guard(m_txQueueMutex);
            if(m_txQueue.size()>0 and (m_txQueue.front().m_dispatched == true))
            {
                if(m_pjon.packets[m_txQueue.front().m_pjonPacketBufferIndex].state == 0)
                {
                    // we now know we have success, as if we would have failure, 
                    // error callback would have been called and the packet would
                    // already be popped with promise set to false
                    m_txQueue.front().m_successPromise.set_value(Result());

                    // packet can be popped from queue:
                    m_txQueue.pop();
                }
            }
        }

        // handle second part of PJON state machine, receiving packets:
        for(int i = 0; i<100; i++)
        {
            // calling this multiple times, as pjon internally might receive
            // packets event-loop based byte-by byte.
            // This would then effectively limit to one byte per sleep period.
            // I circumvent this by hoping that a pjon packet rarely has
            // more than 100 bytes required to receive
            m_pjon.receive();
        }

        // FIXME: this causes "busy-waiting" loop, and causes high CPU load.
        //        as a simple workaround we introduce a delay here to free up
        //        CPU time.
        //        A better solution would require three parts:
        //        - Use a condition variable to exit sleeping as soon as user
        //          wants to transmit
        //        - find a way to determine (e.g. return value from receive() )
        //          if more receive() calls will be required in the future.
        //          e.g. if we are in the middle of receiving a packet.
        //          If so skip sleeping to cause no unwanted delays.
        //        - sleep on condition variable for ~10..500ms depending on
        //          desired rx latency
        //        Other solutions e.g. using poll() for unix serial device read
        //        may work more efficiently and provide very low latency, but are
        //        specific to used strategy :-(
        if(std::chrono::steady_clock::now() - m_lastRxTxActivity.load() > std::chrono::milliseconds(200))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}

template<class Strategy>
void Bus<Strategy>::dispatchTxRequest(TxRequest & f_request)
{
    PJON_Packet_Info info;

    info.tx.id = f_request.m_localAddress.id;
    info.rx.id = f_request.m_remoteAddress.id;
    info.header = PJON_NO_HEADER; // TODO: what does this mean?
    PJONTools::copy_id(info.rx.bus_id, f_request.m_remoteAddress.busId.data(), 4);
    PJONTools::copy_id(info.tx.bus_id, f_request.m_localAddress.busId.data(), 4);
#if(PJON_INCLUDE_PACKET_ID)
    info.id = 0; // packet id
    // TODO: support packet ID (maybe can be done transparent to user via connections)
#endif
#if(PJON_INCLUDE_PORT)
    info.port = f_request.m_remoteAddress.port;
#endif

    uint16_t bufferIndex = m_pjon.send(
            info,
            f_request.m_payload.data(),
            f_request.m_payload.size()
            );

    m_lastRxTxActivity = std::chrono::steady_clock::now();

    if(bufferIndex != PJON_FAIL)
    {
        f_request.m_pjonPacketBufferIndex = bufferIndex;
        f_request.m_dispatched = true;
    }
    else
    {
        f_request.m_pjonPacketBufferIndex = 0;
        f_request.m_dispatched = false;
    }
}

}
