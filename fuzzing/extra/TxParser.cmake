# project information
project(TxParser
        VERSION 1.0
        DESCRIPTION "Transaction parser of Everscale app"
        LANGUAGES C)

# specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Wno-unused-function -DFUZZ -pedantic -g -O0"
)

add_library(txparser
    ${BOLOS_SDK}/lib_standard_app/format.c
    ${BOLOS_SDK}/lib_standard_app/buffer.c
    ${BOLOS_SDK}/lib_standard_app/read.c
    ${BOLOS_SDK}/lib_standard_app/write.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/byte_stream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cell.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/contract.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/hashmap_label.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/message.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/slice_data.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils.c
)

set_target_properties(txparser PROPERTIES SOVERSION 1)

target_include_directories(txparser PUBLIC
    ${BOLOS_SDK}/include
    ${BOLOS_SDK}/lib_cxng/include
    ${BOLOS_SDK}/lib_standard_app
    ${BOLOS_SDK}/target/${TARGET_DEVICE}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
)
