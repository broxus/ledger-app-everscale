cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(FuzzTxParser
        VERSION 1.0
        DESCRIPTION "Fuzzing of transaction parser"
        LANGUAGES C)

find_package(OpenSSL REQUIRED)

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Fuzzer needs to be built with Clang")
endif()

if (NOT DEFINED BOLOS_SDK)
  message(FATAL_ERROR "BOLOS_SDK environment variable not found.")
endif()

if (NOT DEFINED TARGET_DEVICE)
  message(FATAL_ERROR "TARGET_DEVICE environment variable not found.")
endif()

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

# compatible with ClusterFuzzLite
if (NOT DEFINED ENV{LIB_FUZZING_ENGINE})
	set(COMPILATION_FLAGS_ "-g -Wall -fsanitize=fuzzer,address,undefined")
else()
	set(COMPILATION_FLAGS_ "$ENV{LIB_FUZZING_ENGINE} $ENV{CXXFLAGS}")
endif()

add_compile_options(-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(IO_HID_EP_LENGTH=64 USB_SEGMENT_SIZE=64 BLE_SEGMENT_SIZE=32)

string(REPLACE " " ";" COMPILATION_FLAGS ${COMPILATION_FLAGS_})

include(extra/TxParser.cmake)

add_executable(fuzzer fuzzer.c os_mocks.c)

target_include_directories(fuzzer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_compile_options(fuzzer PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzzer PUBLIC ${COMPILATION_FLAGS})
target_link_libraries(fuzzer PUBLIC txparser OpenSSL::SSL)
