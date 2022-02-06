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
      # TODO: using my private fork for now, which includes fix for
      #       https://github.com/gioblu/PJON/issues/406
      #       Once this is merged and release, can use official upstream again
      GIT_REPOSITORY https://github.com/rainerschoe/PJON.git
      #GIT_REPOSITORY https://github.com/gioblu/PJON
      #GIT_TAG        f3611f40
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

# TODO: build example in own CMake file (maybe use FetchContent)
set(TARGET_NAME "throughSerialTxRx")
add_executable(${TARGET_NAME}
    examples/throughSerialTxRx.cpp
    )
target_link_libraries(${TARGET_NAME} PRIVATE PjonHL)

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

    if(ENABLE_COVERAGE)                                    
      FetchContent_Declare(cmake-codecov                                      
        GIT_REPOSITORY https://github.com/callalilychen/CMake-codecov.git            
        GIT_TAG        9d810304424e3bf0b0d8d051c59786765f534568                   
      )                                                    
      FetchContent_Populate(cmake-codecov)                 
      set(CMAKE_MODULE_PATH "${cmake-codecov_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
      find_package(codecov)                                
      add_coverage(throughSerialTxRx)                         
      add_coverage(${PROJECT_NAME})                         
      add_coverage(${TARGET_NAME})                         
      list(APPEND LCOV_REMOVE_PATTERNS "'/usr/*'")         
      coverage_evaluate()                                  
    endif()
endif()

