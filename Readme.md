# PjonHL
PjonHL is a highlevel wrapper around [PJON](https://github.com/gioblu/PJON).

It provides a thread-aware and thread-safe C++ (>=14) interface. 

One of the use-cases of PjonHL is to allow easy implementation of linux based
_PJON gateways_ or _Graphical User Interfaces_ for example on RapsberryPi.

The target platforms for PjonHL are systems which provide
- Threading
- No constrained resources (Memory)

This document intends to give a simplified overview. For detailed documentation
please have a look into the corresponding `.hpp` header files. A good start is
[PjonHlBus.hpp](./PjonHlBus.hpp) and [Connection.hpp](./Connection.hpp).

## Building and Linking
PjonHL needs to be built and linked to your project.

### Using CMake
The library comes with CMake build configuration (see [CMakeLists.txt](./CMakeLists.txt)).
You may use this (e.g. via
[FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html))
and link against the `PjonHL` target in your projects.

To build the example and tests, create a build folder, run cmake and then build:
```
> mkdir build
> cd build
> cmake ..
> make
```

If the CMake target `libPjon` is not defined (by your project), CMake will
automatically download Pjon as a dependency during build configuration for you.

Unit tests are built if CMake is configured to do so (`enable_testing()` or
pass `-DBUILD_TESTS=ON` to cmake).
This uses [Catch2](https://github.com/catchorg/Catch2) as a unit test framework,
which will also be automatically downloaded during build configuration.  
To run the unit tests, execute `ctest` in the build folder.

### Building manually
If you do not use CMake, you still can build the project, you merely need to
compile the `*.cpp` files in the PjonHL directory and link against them. When
compiling you need to have the Pjon source directory as an include path.

## Concept
PjonHL consists of two main classes:

### Bus:
Represents a single PJON bus and internally handles the PJON event-loop
(receive + update) in a thread. To create a Bus, you need to pass a PJON strategy:

```C++
ThroughSerial serialStrategy;

// [...] initialize strategy

// Construct a PjonHL bus:
PjonHL::Bus<ThroughSerial> bus("0.0.0.0/42", serialStrategy);
```

The Bus then provides methods to create "Connections":

```C++
ConnectionHandle connection = hlBus.createConnection(PjonHL::Address("0.0.0.0/53"));
```

The `Bus` instance is then responsible for feeding the associated Connections
with data, as well as transmitting data sent over a Connection.

NOTE: A Bus instance is not to be used to directly send/receive packets. Connections should be used.

### Connection:
A `Connection` is created by a bus and accessed through a ConnectionHandle.

It holds two Addresses:
   1. Address of a "remote" bus participant (or a broadcast address)
   2. Address of the "local" bus participant (or a broadcast address)

It allows sending and receiving of packets to a remote counterpart:
```C++
std::vector<uint8_t> data;
std::future<bool> txSuccess = connection->send(std::move(data), 1000);

Expect< std::vector<uint8_t> > received = connection->receive(1000);
```

As well as checking if sending or receiving succeeded:
```C++
if(txSuccess.get())
{
    // [...] handle success/failure
}
if(received.isValid())
{
    // [...] handle received packet
}

```

### Helper Classes
In addition to the two main classes the following useful helper classes are
introduced:

#### Address:
Represents a PJON Address and provides conversion to and from a convenient
string representations.

E.g. "42" or "42:3456" or full form: "0.0.0.0/42:3456"
```C++
PjonHL::Address addr("0.0.0.0/42:3456");
std::cout << "Address is " << addr.to_string();
```

#### Expect:
Represents an expected/optional value.
It provides a method to check if expected value is present.

In PjonHL `Expect< std::vector<uint8_t> >` is used as a return type for
receive() calls, which may or may not receive data.
```C++
Expect< std::vector<uint8_t> > receivedPacket = connection.receive(1000);
if(receivedPacket.isValid())
{
    std::vector<uint8_t> packet = receivedPacket.unwrap();
}
```

## Class relationship:
```
----------------------      ---------------------------------------------------
| Connection         |      | Bus                                             |
----------------------      ---------------------------------------------------
| - send(vector)     |------| - Bus (localAddr, Strategy)                     |
| - Expect receive() |*    1| - ConnectionHandle createConnection (remoteAddr)|
----------------------      ---------------------------------------------------
```

## Example
Pseudo code below shows the general Idea.
Full example see [here](./examples/throughSerialTxRx.cpp).
```C++
ThroughSerial serialStrategy;

// [...] initialize strategy

// Construct a PjonHL bus:
PjonHL::Bus<ThroughSerial> bus("0.0.0.0/42", serialStrategy);

// Create a Connection on the bus:
ConnectionHandle connection = hlBus.createConnection(PjonHL::Address("0.0.0.0/53"));

std::vector<uint8_t> data;

// [...] fill data vector

// Send a packet to the remote side using the conneciton:
std::future<bool> success = connection->send(std::move(data), 1000);

// [...] Now could do some other processing in parallel like sending some other
//       packets, as send() does not block.

// Now wait and check if send succeeded:
if(success.get())
{
    // [...] handle success/failure
}

// Try to receive data from the connection:
Expect< std::vector<uint8_t> > received = connection->receive(1000);
if(received.isValid())
{
    // [...] handle received packet
}

```
