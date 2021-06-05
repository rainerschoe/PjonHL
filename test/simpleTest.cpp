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

#define PJON_INCLUDE_TS true  

#define TS_RESPONSE_TIME_OUT 35000
/* Maximum accepted timeframe between transmission and synchronous
   acknowledgement. This timeframe is affected by latency and CRC computation.
   Could be necessary to higher this value if devices are separated by long
   physical distance and or if transmitting long packets. */

#define PJON_INCLUDE_TS true  // Include only ThroughSerial
#define PJON_INCLUDE_LUDP true
#define TS_INITIAL_DELAY 0
#include "../PjonHlBus.hpp"
//#include "Actions.hpp"

#include <PJON.h>
#include "strategies/ThroughSerial/ThroughSerial.h"
#include <iostream>
#include "cstdio"

std::string vectorToString(const std::vector<uint8_t> & vec)
{
    std::string result;
    for(auto c : vec)
    {
        result += static_cast<char>(c);
    }
    return result;
}

std::string vectorToHexString(const std::vector<uint8_t> & vec)
{
    std::string result;
    for(auto c : vec)
    {
        char buf[3];
        snprintf(buf,3,"%02x", c);
        result += buf;
        result += " ";
    }
    return result;

}

struct __attribute__ ((packed)) IdentificationMessage
{
  uint8_t msgType=0x20;
  uint8_t deviceId;
  uint8_t busId[4];
  char deviceName[24];
};

int main()
{

  // configure PJON strategy:
  ThroughSerial serialStrategy;
  int fd = -1;
  std::cout << "open...\n";
  fd = serialOpen("/dev/ttyUSB0", 250000);
  serialStrategy.set_serial(fd);
  serialStrategy.set_baud_rate(250000);

  // Initialize PJON_HL:
  std::cout << "init...\n";
  PjonHL::Bus<ThroughSerial> hlBus(0x11, serialStrategy);

  // create a connection:
  // It is possible to use convenient String representation of a PJON addr:
  // DeviceId@BusId:Port  only deviceId is mandatory
  auto connection = hlBus.createConnection(PjonHL::Address("42@0.0.0.0"));
  // connection can now be used to send/transmit packets from and to the target
  // address.
  
  // Send a packet (send will not block):
  std::cout << "dispatch...\n";
  std::vector<uint8_t> identification_request;
  identification_request.push_back(0x00);
  std::future<bool> success = connection->send(std::move(identification_request), 1000);
  std::cout << "dispatched...\n";

  // Check if send succeeded (this will now wait until packet is sent or failed):
  if(success.get())
  {
    std::cout << "success :)\n";
  }
  else
  {
    std::cout << "failure :(\n";
  }

  // now receive data on this connection (wait up to 6s for data to arrive):
  Expect< std::vector<uint8_t> > received = connection->receive(6000);
  if(received.isValid())
  {
      std::vector<uint8_t> & data = received.unwrap();
      for(uint8_t byte : data)
      {
        std::cout << (uint16_t)byte << std::endl;
      }
  }
  return 0;
}
