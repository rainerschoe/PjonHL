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

namespace PjonHL
{
template<class Strategy>
Connection<Strategy>::Connection(Address f_remoteAddress, Address f_remoteMask, Address f_localAddress, Address f_localMask, Bus<Strategy> & f_pjonHL) :
    m_remoteAddress(f_remoteAddress),
    m_remoteMask(f_remoteMask),
    m_localAddress(f_localAddress),
    m_localMask(f_localMask),
    m_pjonHL(f_pjonHL),
    m_active(true)
{
}

template<class Strategy>
std::future<Result> Connection<Strategy>::send(const std::vector<uint8_t> && f_payload, uint32_t f_timeout_milliseconds, bool f_enableRetransmit)
{
    std::lock_guard<std::mutex> guard(m_activityMutex);

    if(not m_active)
    {
        std::promise<Result> promise;
        promise.set_value(Result("Connection not active (is Bus instance still alive?)"));
        return promise.get_future();
    }
    // TODO: I hope m_localAddress means to PJON what I think it means?
    return m_pjonHL.send(m_localAddress, m_remoteAddress, f_payload, f_timeout_milliseconds, f_enableRetransmit);
}

template<class Strategy>
Expect< std::vector<uint8_t> > Connection<Strategy>::receive(uint32_t f_timeout_milliseconds)
{
    std::unique_lock<std::mutex> guardActivity(m_activityMutex);
    if(not m_active)
    {
        return Expect< std::vector<uint8_t> >();
    }

    std::unique_lock<std::mutex> guardRxQueue(m_rxQueueMutex);

    bool dataAvailable = m_rxQueueCondition.wait_for(
            guardRxQueue,
            std::chrono::milliseconds(f_timeout_milliseconds),
            [this]{return m_rxQueue.size()>0;}
            );

    if(dataAvailable)
    {
        std::vector<uint8_t> rxPayload = std::move(m_rxQueue.front());
        m_rxQueue.pop();
        return Expect< std::vector<uint8_t> >{std::move(rxPayload)};
    }

    return Expect<std::vector<uint8_t>>{};
}

template<class Strategy>
void Connection<Strategy>::addReceivedPacket(std::vector<uint8_t> && f_packet, Address f_remoteAddress)
{
    // NOTE: not locking m_activityMutex here
    //       - to avoid problems with condition variable.
    //       - To allow safe destruction if connection (if we would lock
    //         activity here we can deadlock if destructor of Handle runs and
    //         Bus is trying to deliver packet to this connection at same time)
    //       This function is hence not thread safe with respect to destruction
    //       of Bus.
    //       This works however, as only Busis calling it and it makes sure to
    //       de-register before.

    // TODO: handle remote address here
    std::unique_lock<std::mutex> guardRxQueue(m_rxQueueMutex);
    m_rxQueue.push(f_packet);
    m_rxQueueCondition.notify_all();
}

template<class Strategy>
void Connection<Strategy>::setInactive()
{
    std::lock_guard<std::mutex> guard(m_activityMutex);
    m_active = false;
}

}
