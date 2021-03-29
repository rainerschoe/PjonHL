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

#include "Address.hpp"

// Good case tests:

TEST_CASE( "DevId", "" ) {
    PjonHL::Address addr("42");
    REQUIRE( addr.id == 42 );
    REQUIRE( addr.port == 0 );
    REQUIRE( addr.busId[0] == 0 );
    REQUIRE( addr.busId[1] == 0 );
    REQUIRE( addr.busId[2] == 0 );
    REQUIRE( addr.busId[3] == 0 );
}

TEST_CASE( "DevId + Port", "" ) {
    PjonHL::Address addr("42:876");
    REQUIRE( addr.id == 42 );
    REQUIRE( addr.port == 876 );
    REQUIRE( addr.busId[0] == 0 );
    REQUIRE( addr.busId[1] == 0 );
    REQUIRE( addr.busId[2] == 0 );
    REQUIRE( addr.busId[3] == 0 );
}

TEST_CASE( "Bus + DevId", "" ) {
    PjonHL::Address addr("1.5.6.38/42");
    REQUIRE( addr.id == 42 );
    REQUIRE( addr.port == 0 );
    REQUIRE( addr.busId[0] == 1 );
    REQUIRE( addr.busId[1] == 5 );
    REQUIRE( addr.busId[2] == 6 );
    REQUIRE( addr.busId[3] == 38 );
}

TEST_CASE( "Bus + DevId + Port", "" ) {
    PjonHL::Address addr("1.5.6.38/42:873");
    REQUIRE( addr.id == 42 );
    REQUIRE( addr.port == 873 );
    REQUIRE( addr.busId[0] == 1 );
    REQUIRE( addr.busId[1] == 5 );
    REQUIRE( addr.busId[2] == 6 );
    REQUIRE( addr.busId[3] == 38 );
}

TEST_CASE( "toString default", "" ) {
    PjonHL::Address addr;
    REQUIRE("0.0.0.0/0:0" == addr.toString());
}

TEST_CASE( "toString devId", "" ) {
    PjonHL::Address addr("56");
    REQUIRE("0.0.0.0/56:0" == addr.toString());
}

TEST_CASE( "toString devIdPort", "" ) {
    PjonHL::Address addr("56:8765");
    REQUIRE("0.0.0.0/56:8765" == addr.toString());
}

TEST_CASE( "toString busDevIdPort", "" ) {
    PjonHL::Address addr("1.2.3.4/56:8765");
    REQUIRE("1.2.3.4/56:8765" == addr.toString());
}

// Bad case tests:

TEST_CASE( "DevId Range", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("256")
    );
}

TEST_CASE( "Port Range", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("42:65536")
    );
}

TEST_CASE( "Bus Range1", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("0.0.0.256/42")
    );
}

TEST_CASE( "Bus Range2", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("0.0.256.0/42")
    );
}

TEST_CASE( "Bus Range3", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("0.256.0.0/42")
    );
}

TEST_CASE( "Bus Range4", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("256.0.0.0/42")
    );
}

TEST_CASE( "Format Error no devid", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("256.0.0.0:42")
    );
}

TEST_CASE( "Format Error slash instead of :", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("42/234")
    );
}

TEST_CASE( "Format Error empty string", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("")
    );
}

TEST_CASE( "Format Error too small bus 3", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("0.0.0/45")
    );
}

TEST_CASE( "Format Error too small bus 2", "" ) {
    REQUIRE_THROWS(
    PjonHL::Address("0.0/45")
    );
}
