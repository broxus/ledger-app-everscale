#ifdef FUZZ_WITH_PROTOBUF
#include <bits/types/struct_timeval.h>
#include <src/libfuzzer/libfuzzer_macro.h>
#include <sys/time.h>

#include "proto/pcap.pb.h"
#endif

#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "globals.h"
#include "message.h"
#include "os.h"
#include "byte_stream.h"

#ifdef FUZZ_WITH_PROTOBUF
#else

BocContext_t boc_context;
DataContext_t data_context;

void hex_to_bytes(const char* hex, uint8_t* bytes)
{
    size_t len = strlen(hex);
    size_t final_len = len / 2;
    for (size_t i=0, j=0; j<final_len; i+=2, j++)
        bytes[j] = (hex[i] % 32 + 9) % 25 * 16 + (hex[i+1] % 32 + 9) % 25;
}

extern int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len) {
    if (len < 1) return -1;

    /*auto boc = "0101040104a0a0a01a4ba92d8ae00301e02800004000";
    auto buffer_len = strlen(boc) / 2;

    uint8_t buffer[buffer_len];
    hex_to_bytes(boc, buffer);*/

    data_context.sign_tr_context.current_wallet_type = buf[0] % 9;

    BEGIN_TRY {
        TRY {
            ByteStream_t src;
            ByteStream_init(&src, buf+1, len-1);

            uint8_t wc = DEFAULT_WORKCHAIN_ID;

            uint8_t address[ADDRESS_LENGTH] = {
                0x30, 0x99, 0xf1, 0x4e, 0xcc, 0xaa, 0x05, 0x42,
                0xd2, 0xd6, 0x0e, 0x92, 0xeb, 0x66, 0x49, 0x5f,
                0x6e, 0xcf, 0x01, 0xa1, 0x14, 0xe1, 0x2f, 0x9d,
                0xb8, 0xd9, 0xcb, 0x82, 0x7a, 0x87, 0xbf, 0x84};

            uint8_t prepend_address[ADDRESS_LENGTH];
            memcpy(prepend_address, address, ADDRESS_LENGTH);

            int flow = prepare_to_sign(&src, wc, address, prepend_address);
        }
        CATCH_OTHER(e) {}
        FINALLY {}
    }
    END_TRY;

    return 0;
}

#endif

/*#ifdef IS_AFL

__AFL_FUZZ_INIT();

int main(int argc, char *argv[]) {
    printf("Hello Geek!\n");
    fflush(stdout);

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif

    uint8_t *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN;
        LLVMFuzzerTestOneInput(buf, len);
    }

    return 0;
}

#endif*/
