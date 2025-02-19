
// unsigned short sw = 0;

// if (!flags || !tx) {
//     THROW(0x6802);
// }

// if (rx < 0) {
//     THROW(0x6813);
// }

// BEGIN_TRY {
//     TRY {
//         if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
//             THROW(0x6E00);
//         }
//         // must at least hold the class and instruction
//         if (rx <= OFFSET_INS) {
//             THROW(0x6b00);
//         }
//         PRINTF("command: %d\n", G_io_apdu_buffer[OFFSET_INS]);
//         switch (G_io_apdu_buffer[OFFSET_INS]) {
//             case INS_GET_APP_CONFIGURATION: {
//                 if (G_io_apdu_buffer[OFFSET_P1] != 0 || G_io_apdu_buffer[OFFSET_P2] != 0) {
//                     THROW(0x6802);
//                 }

//                 handleGetAppConfiguration(G_io_apdu_buffer[OFFSET_P1],
//                                           G_io_apdu_buffer[OFFSET_P2],
//                                           G_io_apdu_buffer + OFFSET_CDATA,
//                                           G_io_apdu_buffer[OFFSET_LC],
//                                           flags,
//                                           tx);
//             } break;

//             case INS_GET_PUBLIC_KEY: {
//                 if (G_io_apdu_buffer[OFFSET_LC] != rx - OFFSET_CDATA) {
//                     // the length of the APDU should match what's in the 5-byte header.
//                     // If not fail.  Don't want to buffer overrun or anything.
//                     THROW(0x6985);
//                 }

//                 handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1],
//                                    G_io_apdu_buffer[OFFSET_P2],
//                                    G_io_apdu_buffer + OFFSET_CDATA,
//                                    G_io_apdu_buffer[OFFSET_LC],
//                                    flags,
//                                    tx);
//             } break;

//             case INS_GET_ADDRESS: {
//                 if (G_io_apdu_buffer[OFFSET_LC] != rx - OFFSET_CDATA) {
//                     // the length of the APDU should match what's in the 5-byte header.
//                     // If not fail.  Don't want to buffer overrun or anything.
//                     THROW(0x6985);
//                 }

//                 handleGetAddress(G_io_apdu_buffer[OFFSET_P1],
//                                  G_io_apdu_buffer[OFFSET_P2],
//                                  G_io_apdu_buffer + OFFSET_CDATA,
//                                  G_io_apdu_buffer[OFFSET_LC],
//                                  flags,
//                                  tx);
//             } break;

//             case INS_SIGN: {
//                 if (G_io_apdu_buffer[OFFSET_P1] != P1_CONFIRM || G_io_apdu_buffer[OFFSET_P2] !=
//                 0) {
//                     THROW(0x6802);
//                 }

//                 if (G_io_apdu_buffer[OFFSET_LC] != rx - OFFSET_CDATA) {
//                     // the length of the APDU should match what's in the 5-byte header.
//                     // If not fail.  Don't want to buffer overrun or anything.
//                     THROW(0x6985);
//                 }

//                 handleSign(G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags);
//             } break;

//             case INS_SIGN_TRANSACTION: {
//                 if (G_io_apdu_buffer[OFFSET_LC] != rx - OFFSET_CDATA) {
//                     // the length of the APDU should match what's in the 5-byte header.
//                     // If not fail.  Don't want to buffer overrun or anything.
//                     THROW(0x6985);
//                 }

//                 uint8_t p1 = G_io_apdu_buffer[OFFSET_P1];
//                 if (p1 == P1_NON_CONFIRM) {
//                     // Don't allow blind signing.
//                     THROW(0x6808);
//                 }

//                 uint8_t p2 = G_io_apdu_buffer[OFFSET_P2];
//                 bool more = (bool) (p2 & P2_MORE);

//                 // P2_MORE is a signal for more apdu to receive in current chunk;
//                 // P2_EXTEND is a signal for extended buffer and can't be in first chunk;
//                 // P2_MORE && !P2_EXTEND = first chunk;
//                 // P2_EXTEND && !P2_MORE = last chunk;
//                 // P2_EXTEND && P2_MORE = ordinary request without chunks;

//                 // P2_EXTEND is set to signal that this APDU buffer extends, rather
//                 // than replaces, the current message buffer
//                 bool first_data_chunk = !(p2 & P2_EXTEND);

//                 handleSignTransaction(G_io_apdu_buffer + OFFSET_CDATA,
//                                       G_io_apdu_buffer[OFFSET_LC],
//                                       flags,
//                                       first_data_chunk,
//                                       more);
//             } break;

//             default:
//                 THROW(0x6D00);
//                 break;
//         }
//     }
//     CATCH(EXCEPTION_IO_RESET) {
//         THROW(EXCEPTION_IO_RESET);
//     }
//     CATCH_OTHER(e) {
//         switch (e & 0xF000) {
//             case 0x6000:
//                 sw = e;
//                 break;
//             case 0x9000:
//                 // All is well
//                 sw = e;
//                 break;
//             default:
//                 // Internal error
//                 sw = 0x6800 | (e & 0x7FF);
//                 break;
//         }
//         // Unexpected exception => report
//         G_io_apdu_buffer[*tx] = sw >> 8;
//         G_io_apdu_buffer[*tx + 1] = sw;
//         *tx += 2;
//     }
//     FINALLY {
//     }
// }
// END_TRY;