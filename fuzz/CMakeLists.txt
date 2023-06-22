cmake_minimum_required(VERSION 3.10)

project(NeoFuzzer VERSION 0.1.9 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# BOLOS SDK
set(BOLOS_SDK $ENV{BOLOS_SDK})
add_compile_definitions(IO_HID_EP_LENGTH=64)

find_package(OpenSSL REQUIRED)

# add_compile_definitions(HAVE_HASH HAVE_SHA256)
include_directories(.
    ../src
    "${BOLOS_SDK}/include"
    "${BOLOS_SDK}/lib_cxng/include"
)

add_compile_options(-g -O0 -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)

# Build with code coverage generation
if(CODE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "(Apple)?[Cc]lang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
        add_compile_options(-fprofile-arcs -ftest-coverage)
        link_libraries(gcov)
    else()
        message(FATAL_ERROR "Unsupported compiler used with code coverage generation")
    endif()
endif()

# Fuzzer target
set(APP_SRC_DIR "../src")

set(APP_SOURCES
    ${APP_SRC_DIR}/byte_stream.c
    ${APP_SRC_DIR}/message.c
    ${APP_SRC_DIR}/slice_data.c
    ${APP_SRC_DIR}/contract.c
    ${APP_SRC_DIR}/cell.c
    ${APP_SRC_DIR}/hashmap_label.c
    ${APP_SRC_DIR}/utils.c
)

add_executable(fuzzer
    fuzzer.c
    os_mocks.c
    ${APP_SOURCES}
)

target_include_directories(fuzzer PUBLIC ../src)
target_compile_options(fuzzer PUBLIC -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined)
target_link_options(fuzzer PUBLIC -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined)
target_link_libraries(fuzzer OpenSSL::SSL)
