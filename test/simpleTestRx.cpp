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
#include "../PjonHlBus.hpp"

#include <PJON.h>
#include "strategies/ThroughSerial/ThroughSerial.h"
#include <iostream>

int main()
{

  ////ThroughSerial serialStrategy;
  //LocalUDP udpStrategy;
  //udpStrategy.set_port(5555);

  ////PjonHL::Bus<ThroughSerial> hlBus(42, serialStrategy);
  //PjonHL::Bus hlBus(43, udpStrategy);

  ////auto connection = hlBus.createConnection(PjonHL::Bus<ThroughSerial>::Address(43), PjonHL::Bus<ThroughSerial>::Address(42));
  //auto connection = hlBus.createConnection(Address(42));
  //
  //std::cout << "dispatch...\n";
  //while(1)
  //{
  //  auto packet = connection->receive(100);
  //  if(packet.isValid())
  //  {
  //    std::cout << "received: " << reinterpret_cast<char*>(packet.unwrap().data()) << std::endl;
  //  }
  //}
  return 0;
}
