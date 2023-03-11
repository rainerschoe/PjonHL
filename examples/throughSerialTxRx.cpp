// Include only ThroughSerial
#define PJON_INCLUDE_TS true

// reduce initial startup delay to 0 (usually not needed for Linux systems if
// application is started by user on commandline)
#define TS_INITIAL_DELAY 0


// Include PJON, Strategies and PjonHL::Bus
#include <PJON.h>
#include "strategies/ThroughSerial/ThroughSerial.h"
#include "PjonHlBus.hpp"

// Iostream for printing to stdout
#include <iostream>

int main()
{
    // configure PJON strategy:
    std::cout << "Preparing ThroughSerial..." << std::endl;
    ThroughSerial serialStrategy;
    int fd = -1;
    fd = serialOpen("/dev/ttyUSB0", 250000);
    if(fd < 0)
    {
        std::cout << " serial open failed" << std::endl;
        return -1;
    }
    serialStrategy.set_serial(fd);
    serialStrategy.set_baud_rate(250000);
    std::cout << " done" << std::endl;

    // Initialize PJON_HL:
    std::cout << "Initializing PjonHL..." << std::endl;
    PjonHL::Bus<ThroughSerial> hlBus(PjonHL::Address{"0.0.0.0/71"}, serialStrategy);
    std::cout << " done" << std::endl;

    // create a connection:
    // It is possible to use convenient String representation of a PJON addr:
    // DeviceId@BusId:Port  only deviceId is mandatory
    PjonHL::Address targetAddr("42");
    auto connection = hlBus.createConnection(targetAddr);
    // connection can now be used to send/transmit packets from and to the target
    // address.

    // Send a packet (send will not block):
    std::cout << "Dispatching packet to " << targetAddr.toString() << "..." << std::endl;
    std::vector<uint8_t> identification_request{0xab, 0xcd, 0xef};
    std::future<PjonHL::Result> resultFut = connection->send(std::move(identification_request), 1000);
    std::cout << " done" << std::endl;

    // Check if send succeeded (this will now wait until packet is sent or failed):
    std::cout << "Check if send was successful..." << std::endl;
    auto result = resultFut.get();
    if(result.isGood())
    {
        std::cout << " success :)\n";
    }
    else
    {
        std::cout << " failure :( " << result.getErrorMessage() << "\n";
    }

    // now receive data on this connection (wait up to 1s for data to arrive):
    std::cout << "Receive data from " << targetAddr.toString() << "..." << std::endl;
    auto received = connection->receive(1000);
    if(received.isValid())
    {
        std::vector<uint8_t> & data = received.unwrap().payload;
        std::cout << " Received " << data.size() << "bytes:" << std::endl;
        for(uint8_t byte : data)
        {
            std::cout << "  " << (uint16_t)byte << std::endl;
        }
    }
    else
    {
        std::cout << " No data received within 1 second :(\n";
    }

    return 0;
}
