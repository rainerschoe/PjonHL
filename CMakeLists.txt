cmake_minimum_required(VERSION 3.15)
include(FetchContent)

project(PjonHL)

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
endif()

# The following retrieves Pjon as a dependency.
# If you have Pjon already available as part of your project, please create
# the target "libPjon" with correct target_include_directorieswhich PjonHL is
# "linking" against.
if(NOT TARGET libPjon)
    FetchContent_Declare(
      Pjon
      GIT_REPOSITORY https://github.com/gioblu/PJON
      GIT_TAG        13.0
    )

    FetchContent_GetProperties(Pjon)
    if(NOT pjon_POPULATED)
        FetchContent_Populate(Pjon)
    endif()

    add_library(libPjon INTERFACE
    )
    target_include_directories(libPjon INTERFACE 
      ${pjon_SOURCE_DIR}/src
    )
endif()

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})

file(GLOB ${PROJECT_NAME}_SRC ${PROJECT_SOURCE_DIR}/*.c*)
add_library(${PROJECT_NAME}
  ${${PROJECT_NAME}_SRC}
)
target_compile_options(${PROJECT_NAME} PUBLIC -DLINUX2  -DLINUX)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} PUBLIC pthread libPjon)

# The following will build unit-tests and also pull in Catch2 as a dependency.
if(CMAKE_TESTING_ENABLED)
    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v2.13.4
    )
    FetchContent_MakeAvailable(Catch2)

    set(TARGET_NAME "PjonHLTests")
    add_executable(${TARGET_NAME}
        test/PjonHLTests.cpp
        test/AddressTest.cpp
        test/ExpectTest.cpp
        test/TestBus.cpp
        )
    target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME} PjonHL Catch2::Catch2)

    add_test(${TARGET_NAME} ${PROJECT_BINARY_DIR}/${TARGET_NAME})
    message("Building PjonHL unit-test. Executable=${PROJECT_BINARY_DIR}/${TARGET_NAME}")
endif()

# TODO: build example in own CMake file (maybe use FetchContent)
set(TARGET_NAME "throughSerialTxRx")
add_executable(${TARGET_NAME}
    examples/throughSerialTxRx.cpp
    )
target_link_libraries(${TARGET_NAME} PRIVATE PjonHL)