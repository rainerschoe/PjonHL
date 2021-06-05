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

#include "catch2/catch.hpp"

#include "PjonHlBus.hpp"
#include "PJONDefines.h"
#include <algorithm>
#include <inttypes.h>
#include <vector>
#include <queue>
#include <thread>

// Mock Strategy
class Strategy
{
};

class PJONShadow
{
    struct RxPacket
    {
        uint8_t *payload;
        uint16_t length;
        PJON_Packet_Info packet_info;
    };
    public:

    void set_error(PJON_Error e) {
      _error = e;
    };

    void set_receiver(PJON_Receiver r) {
      _receiver = r;
    };

    void begin()
    {
    }

    uint16_t update() {
        if(m_numErrorQueued>0)
        {
            m_numErrorQueued--;
            _error(0,0,0);
        }
        return 0;
    }
    uint16_t receive() {
        std::lock_guard<std::mutex> guard(m_rxPacketQueueMutex);
        if(m_rxPacketQueue.size() > 0)
        {
            RxPacket & packet = m_rxPacketQueue.front();
            _receiver(packet.payload, packet.length, packet.packet_info);
            m_rxPacketQueue.pop();
        }
        return 0;
    }

    uint16_t send(
      const PJON_Packet_Info &info,
      const void *payload,
      uint16_t length
    )
    {
        sendCount++;
        packets[0].state = m_nextSendResult?0:1;
        if(m_nextSendResult == false)
        {
            m_numErrorQueued++;
        }
        m_nextSendResult = false;;
        return 0;
    };

    PJON_Error _error;
    PJON_Receiver _receiver;
    Strategy strategy;
    PJON_Packet * packets;

    // test functionality:
    size_t sendCount = 0;
    void reset()
    {
        sendCount = 0;
        _error = static_cast<PJON_Error>(nullptr);
        _receiver = static_cast<PJON_Receiver>(nullptr);
        strategy = Strategy();
        packets = nullptr;
        while(not m_rxPacketQueue.empty())
        {
            m_rxPacketQueue.pop();
        }
        m_numErrorQueued = 0;
        m_nextSendResult = false;
    }

    void setNextSendResult(bool result)
    {
        m_nextSendResult = result;
    }
    bool m_nextSendResult = false;

    size_t m_numErrorQueued = 0;

    std::queue<RxPacket> m_rxPacketQueue;

    void enqueuePacketForRx(
            uint8_t *payload,
            uint16_t length,
            const PJON_Packet_Info &packet_info
            )
    {
        std::lock_guard<std::mutex> guard(m_rxPacketQueueMutex);
        RxPacket packet;
        packet.payload = payload;
        packet.length = length;
        packet.packet_info = packet_info;
        m_rxPacketQueue.push(packet);
    }
    std::mutex m_rxPacketQueueMutex;
};

PJONShadow & shadow()
{
    static PJONShadow shadow;
    return shadow;
}

// Mock PJON class. Forwards all calls to a singleton shadow class, so that we
// can monitor what PjonHL is doing with the PJON backend
template<class Strategy>
class PJON
{
    public:
    PJON(const uint8_t *b_id, uint8_t device_id)
    {
        shadow().packets = packets;
        shadow().strategy = strategy;
    }

    void set_error(PJON_Error e) {
        shadow().set_error(e);
    };

    void set_receiver(PJON_Receiver r) {
        shadow().set_receiver(r);
    };

    void begin()
    {
        shadow().begin();
    }

    uint16_t update() {
        return shadow().update();
    }
    uint16_t receive() {
        return shadow().receive();
    }

    uint16_t send(
      const PJON_Packet_Info &info,
      const void *payload,
      uint16_t length
    )
    {
        return shadow().send(info, payload, length);
    };

    void set_acknowledge(bool)
    {
    }
    void set_crc_32(bool)
    {
    }
    void set_communication_mode(bool)
    {
    }
    void set_shared_network(bool)
    {
    }

    PJON_Error _error;
    PJON_Receiver _receiver;
    Strategy strategy;
    PJON_Packet packets[PJON_MAX_PACKETS];
};

TEST_CASE( "destruct bus before connection", "" ) {
    shadow().reset();
    std::unique_ptr<PjonHL::Bus<Strategy>::ConnectionHandle> connection;
    {
        PjonHL::Bus<Strategy> bus(PjonHL::Address{}, Strategy{});
        connection = std::make_unique<PjonHL::Bus<Strategy>::ConnectionHandle>(bus.createConnection(PjonHL::Address{}));
    }
    // now use connection:
    auto future = (*connection)->send(std::vector<uint8_t>{0x00});

    REQUIRE(future.valid() == true);
    REQUIRE(future.get() == false);
    REQUIRE(shadow().sendCount == 0);
}

TEST_CASE( "Send Succeed", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{}, Strategy{});
    auto connection = bus.createConnection(PjonHL::Address{});
    shadow().setNextSendResult(true);
    auto future = connection->send(std::vector<uint8_t>{0x00});
    REQUIRE(future.valid() == true);
    REQUIRE(future.get() == true);
    REQUIRE(1 == shadow().sendCount);
}

TEST_CASE( "Send Fail", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{}, Strategy{});
    auto connection = bus.createConnection(PjonHL::Address{});
    shadow().setNextSendResult(false);
    auto future = connection->send(std::vector<uint8_t>{0x00});
    REQUIRE(future.valid() == true);
    REQUIRE(future.get() == false);
    REQUIRE(1 == shadow().sendCount);
}

TEST_CASE( "Rx good case", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createConnection(PjonHL::Address{42});


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 36;
    info.tx.id = 42;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == true);
    auto data = expectedData.unwrap();

    REQUIRE(data[0] == 0xab);
    REQUIRE(data[1] == 0xcd);
    REQUIRE(data[2] == 0xef);
}

TEST_CASE( "Rx good case 2 connections", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection1 = bus.createConnection(PjonHL::Address{42});
    auto connection2 = bus.createConnection(PjonHL::Address{42});


    auto receiveFuture1 = std::async(
            std::launch::async,
            [connection{std::move(connection1)}]()
            {
                return connection->receive(100);
            }
            );
    auto receiveFuture2 = std::async(
            std::launch::async,
            [connection{std::move(connection2)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 36;
    info.tx.id = 42;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData1 = receiveFuture1.get();

    REQUIRE(expectedData1.isValid() == true);
    auto data1 = expectedData1.unwrap();

    REQUIRE(data1[0] == 0xab);
    REQUIRE(data1[1] == 0xcd);
    REQUIRE(data1[2] == 0xef);

    auto expectedData2 = receiveFuture2.get();

    REQUIRE(expectedData2.isValid() == true);
    auto data2 = expectedData2.unwrap();

    REQUIRE(data2[0] == 0xab);
    REQUIRE(data2[1] == 0xcd);
    REQUIRE(data2[2] == 0xef);
}

TEST_CASE( "Rx wrong target addr", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createConnection(PjonHL::Address{42});


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 37;
    info.tx.id = 42;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == false);
}

TEST_CASE( "Rx timeout", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createConnection(PjonHL::Address{42});


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(20);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 36;
    info.tx.id = 42;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == false);
}

TEST_CASE( "Rx wrong target (Rx) addr", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createConnection(PjonHL::Address{42});


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 37;
    info.tx.id = 42;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == false);
}

TEST_CASE( "Rx wrong source (Tx) addr", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createConnection(PjonHL::Address{42});


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 36;
    info.tx.id = 43;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == false);
}

TEST_CASE( "Rx listening on all remote addr", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createConnection(PjonHL::Address{0}, PjonHL::Address{0});


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 36;
    info.tx.id = 44;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == true);
    auto data = expectedData.unwrap();

    REQUIRE(data[0] == 0xab);
    REQUIRE(data[1] == 0xcd);
    REQUIRE(data[2] == 0xef);
}

TEST_CASE( "Rx listening on different local addr", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createDetachedConnection(PjonHL::Address{42}, PjonHL::Address{37}, PjonHL::Address{"255.255.255.255/255"});


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 37;
    info.tx.id = 42;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == true);
    auto data = expectedData.unwrap();

    REQUIRE(data[0] == 0xab);
    REQUIRE(data[1] == 0xcd);
    REQUIRE(data[2] == 0xef);
}

TEST_CASE( "Rx listening on different local addr but wrong received", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createDetachedConnection(PjonHL::Address{42}, PjonHL::Address{37}, PjonHL::Address{"255.255.255.255/255"});


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 38;
    info.tx.id = 42;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == false);
}

TEST_CASE( "Rx listening on all local addr", "" ) {
    shadow().reset();
    PjonHL::Bus<Strategy> bus(PjonHL::Address{36}, Strategy{});
    auto connection = bus.createDetachedConnection(
            PjonHL::Address{42}, // remote addr
            PjonHL::Address{37}, // local addr
            PjonHL::Address{"255.255.255.255/255"}, // remote mask
            PjonHL::Address{"0.0.0.0/0"} // local mask
            );


    auto receiveFuture = std::async(
            std::launch::async,
            [connection{std::move(connection)}]()
            {
                return connection->receive(100);
            }
            );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::vector<uint8_t> payload{0xab, 0xcd, 0xef};
    PJON_Packet_Info info;
    info.rx.id = 38;
    info.tx.id = 42;
    shadow().enqueuePacketForRx(payload.data(), payload.size(), info);

    auto expectedData = receiveFuture.get();

    REQUIRE(expectedData.isValid() == true);
    auto data = expectedData.unwrap();

    REQUIRE(data[0] == 0xab);
    REQUIRE(data[1] == 0xcd);
    REQUIRE(data[2] == 0xef);
}

