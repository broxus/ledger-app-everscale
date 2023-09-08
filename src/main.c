/*******************************************************************************
*   Ledger Free TON App
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "utils.h"
#include "menu.h"
#include "apdu_constants.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

BocContext_t boc_context;
DataContext_t data_context;

void reset_app_context() {
    memset(&boc_context, 0, sizeof(boc_context));
    memset(&data_context, 0, sizeof(data_context));
}

void handleApdu(volatile unsigned int *flags, volatile unsigned int *tx, int rx) {
    unsigned short sw = 0;

    if (!flags || !tx) {
            THROW(0x6802);
        }
    if (rx < 0) {
            THROW(0x6813);
    }
    BEGIN_TRY {
        TRY {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(0x6E00);
            }
            // must at least hold the class and instruction
            if (rx <= OFFSET_INS) {
                THROW(0x6b00);
            }
            PRINTF("command: %d\n", G_io_apdu_buffer[OFFSET_INS]);
            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case INS_GET_APP_CONFIGURATION: {
                    if (G_io_apdu_buffer[OFFSET_P1] != 0 || G_io_apdu_buffer[OFFSET_P2] != 0) {
                        // invalid parameter?
                        THROW(0x6802);
                    }

                    handleGetAppConfiguration(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2],
                                              G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
                } break;

                case INS_GET_PUBLIC_KEY: {
                    if (G_io_apdu_buffer[OFFSET_LC] != rx - OFFSET_CDATA) {
                        // the length of the APDU should match what's in the 5-byte header.
                        // If not fail.  Don't want to buffer overrun or anything.
                        THROW(0x6985);
                    }

                    handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
                } break;

                case INS_SIGN: {
                    if (G_io_apdu_buffer[OFFSET_LC] != rx - OFFSET_CDATA) {
                        // the length of the APDU should match what's in the 5-byte header.
                        // If not fail.  Don't want to buffer overrun or anything.
                        THROW(0x6985);
                    }

                    handleSign(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
                } break;

                case INS_GET_ADDRESS: {
                    if (G_io_apdu_buffer[OFFSET_LC] != rx - OFFSET_CDATA) {
                        // the length of the APDU should match what's in the 5-byte header.
                        // If not fail.  Don't want to buffer overrun or anything.
                        THROW(0x6985);
                    }

                    handleGetAddress(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
                } break;

                case INS_SIGN_TRANSACTION: {
                    if (G_io_apdu_buffer[OFFSET_LC] != rx - OFFSET_CDATA) {
                        // the length of the APDU should match what's in the 5-byte header.
                        // If not fail.  Don't want to buffer overrun or anything.
                        THROW(0x6985);
                    }

                    /*uint8_t p1 = G_io_apdu_buffer[OFFSET_P1];
                    if (p1 == P1_NON_CONFIRM) {
                        // Don't allow blind signing.
                        THROW(0x6808);
                    } */

                    uint8_t p2 = G_io_apdu_buffer[OFFSET_P2];
                    bool more = (bool) (p2 & P2_MORE);

                    //P2_MORE is a signal for more apdu to receive in current chunk. P2_EXTEND is a signal for extended buffer and can't be in first chunk
                    // P2_EXTEND && !P2_MORE = last APDU message;
                    // P2_EXTEND && P2_MORE = !first !last APDU message
                    // P2_MORE && !P2_EXTEND = first chunk.

                    // P2_EXTEND is set to signal that this APDU buffer extends, rather
                    // than replaces, the current message buffer
                    bool first_data_chunk = !(p2 & P2_EXTEND);

                    const int result = handleSignTransaction(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, first_data_chunk, more);
                    if (result != 0){
                        reset_app_context();
                        THROW(result);
                    } else {
                        THROW(0x9000);
                    }
                } break;

                default:
                    THROW(0x6D00);
                    break;
            }
        }
        CATCH(EXCEPTION_IO_RESET) {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e) {
        switch (e & 0xF000) {
            case 0x6000:
                sw = e;
                reset_app_context();
                break;
            case 0x9000:
                // All is well
                sw = e;
                break;
            default:
                // Internal error
                sw = 0x6800 | (e & 0x7FF);
                reset_app_context();
                break;
            }
            // Unexpected exception => report
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY {
        }
    }
    END_TRY;
}

void app_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;


    reset_app_context();

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);

                handleApdu(&flags, &tx, rx);
            }
            CATCH(EXCEPTION_IO_RESET) {
              THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                    case 0x6000:
                        sw = e;
                        reset_app_context();
                        break;
                    case 0x9000:
                        // All is well
                        sw = e;
                        break;
                    default:
                        // Internal error
                        sw = 0x6800 | (e & 0x7FF);
                        reset_app_context();
                        break;
                }
                if (e != 0x9000) {
                    flags &= ~IO_ASYNCH_REPLY;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

//return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t*)element);
}

unsigned char io_event(unsigned char channel) {
    UNUSED(channel);

    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_STATUS_EVENT:
            if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID && !(U4BE(G_io_seproxyhal_spi_buffer, 3) & SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
                THROW(EXCEPTION_IO_RESET);
            }
            // no break is intentional
            __attribute__((fallthrough));
        default:
            UX_DEFAULT_EVENT();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            UX_DISPLAYED_EVENT({});
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer,
            {
#ifndef TARGET_NANOX
                if (UX_ALLOWED) {
                    if (ux_step_count) {
                    // prepare next screen
                    ux_step = (ux_step+1)%ux_step_count;
                    // redisplay screen
                    UX_REDISPLAY();
                    }
                }
#endif // TARGET_NANOX
            });
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}


unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;

        // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
        case CHANNEL_SPI:
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }
                return 0; // nothing received from the master so far (it's a tx
                        // transaction)
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                            sizeof(G_io_apdu_buffer), 0);
            }

        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}


void app_exit(void) {

    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {

        }
    }
    END_TRY_L(exit);
}

void nv_app_state_init() {

}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        UX_INIT();

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

                nv_app_state_init();

                USB_power(0);
                USB_power(1);

                ui_idle();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif // HAVE_BLE

                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                // reset IO and UX before continuing
                continue;
            }
            CATCH_ALL {
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();
    return 0;
}
