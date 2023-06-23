#include "os.h"
#include "cx.h"
#include "utils.h"
#include "menu.h"

#include <stdlib.h>

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

void get_public_key(uint32_t account_number, uint8_t* publicKeyArray) {
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;

    get_private_key(account_number, &privateKey);
    BEGIN_TRY {
        TRY {
            cx_ecfp_generate_pair(CX_CURVE_Ed25519, &publicKey, &privateKey, 1);
        }
        CATCH_OTHER(e) {
            explicit_bzero(&privateKey, sizeof(privateKey));
            THROW(e);
        }
        FINALLY {
            explicit_bzero(&privateKey, sizeof(privateKey));
        }
    }
    END_TRY;

    for (int i = 0; i < 32; i++) {
        publicKeyArray[i] = publicKey.W[64 - i];
    }
    if ((publicKey.W[32] & 1) != 0) {
        publicKeyArray[31] |= 0x80;
    }
}

static const uint32_t HARDENED_OFFSET = 0x80000000;

void get_private_key(uint32_t account_number, cx_ecfp_private_key_t *privateKey) {
    const uint32_t derivePath[BIP32_PATH] = {
            44 | HARDENED_OFFSET,
            396 | HARDENED_OFFSET,
            account_number | HARDENED_OFFSET,
            0 | HARDENED_OFFSET,
            0 | HARDENED_OFFSET
    };

    uint8_t privateKeyData[32];
    BEGIN_TRY {
        TRY {
            os_perso_derive_node_bip32_seed_key(HDW_ED25519_SLIP10,
                                                CX_CURVE_Ed25519,
                                                derivePath,
                                                BIP32_PATH,
                                                privateKeyData,
                                                NULL,
                                                NULL,
                                                0);
            cx_ecfp_init_private_key(CX_CURVE_Ed25519,
                                     privateKeyData,
                                     32,
                                     privateKey);
        }
        CATCH_OTHER(e) {
            explicit_bzero(&privateKeyData, sizeof(privateKeyData));
            THROW(e);
        }
        FINALLY {
            explicit_bzero(&privateKeyData, sizeof(privateKeyData));
        }
    }
    END_TRY;
}

void send_response(uint8_t tx, bool approve) {
    G_io_apdu_buffer[tx++] = approve? 0x90 : 0x69;
    G_io_apdu_buffer[tx++] = approve? 0x00 : 0x85;
    reset_app_context();
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
}

unsigned int ui_prepro(const bagl_element_t *element) {
    unsigned int display = 1;
    if (element->component.userid > 0) {
        display = (ux_step == element->component.userid - 1);
        if (display) {
            if (element->component.userid == 1) {
                UX_CALLBACK_SET_INTERVAL(2000);
            }
            else {
                UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            }
        }
    }
    return display;
}

#endif

void writeUint32BE(uint32_t val, uint8_t *bytes) {
    bytes[0] = (val >> 24) & 0xFF;
    bytes[1] = (val >> 16) & 0xFF;
    bytes[2] = (val >> 8) & 0xFF;
    bytes[3] = val & 0xFF;
}

void writeUint64BE(uint64_t val, uint8_t *bytes) {
    bytes[0] = (val >> 56) & 0xFF;
    bytes[1] = (val >> 48) & 0xFF;
    bytes[2] = (val >> 40) & 0xFF;
    bytes[3] = (val >> 32) & 0xFF;
    bytes[4] = (val >> 24) & 0xFF;
    bytes[5] = (val >> 16) & 0xFF;
    bytes[6] = (val >> 8) & 0xFF;
    bytes[7] = val & 0xFF;
}

uint16_t readUint16BE(uint8_t *buffer) {
    return (buffer[0] << 8) | (buffer[1]);
}

uint32_t readUint32BE(uint8_t *buffer) {
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | (buffer[3]);
}

uint64_t readUint64BE(uint8_t *buffer) {
    uint32_t i1 = buffer[3] + (buffer[2] << 8u) + (buffer[1] << 16u) + (buffer[0] << 24u);
    uint32_t i2 = buffer[7] + (buffer[6] << 8u) + (buffer[5] << 16u) + (buffer[4] << 24u);
    return i2 | ((uint64_t) i1 << 32u);
}

uint8_t leading_zeros(uint16_t value) {
    uint8_t lz = 0;
    uint16_t msb = 0x8000;
    for (uint8_t i = 0; i < 16; ++i) {
        if ((value << i) & msb) {
            break;
        }
        ++lz;
    }

    return lz;
}

uint16_t format_hex(const uint8_t *in, size_t in_len, char *out, size_t out_len) {
    if (out_len < 2 * in_len + 1) {
        return -1;
    }

    const char hex[] = "0123456789abcdef";
    size_t i = 0;
    int written = 0;

    while (i < in_len && (i * 2 + (2 + 1)) <= out_len) {
        uint8_t high_nibble = (in[i] & 0xF0) >> 4;
        *out = hex[high_nibble];
        out++;

        uint8_t low_nibble = in[i] & 0x0F;
        *out = hex[low_nibble];
        out++;

        i++;
        written += 2;
    }

    *out = '\0';

    return written + 1;
}

#define SCRATCH_SIZE 37
uint8_t convert_hex_amount_to_displayable(const uint8_t* amount, uint8_t decimals, uint8_t amount_length, char* out) {
    uint8_t LOOP1 = SCRATCH_SIZE - decimals;
    uint8_t LOOP2 = decimals;
    uint16_t scratch[SCRATCH_SIZE];
    uint8_t offset = 0;
    uint8_t nonZero = 0;
    uint8_t i;
    uint8_t targetOffset = 0;
    uint8_t workOffset;
    uint8_t j;
    uint8_t nscratch = SCRATCH_SIZE;
    uint8_t smin = nscratch - 2;
    uint8_t comma = 0;

    for (i = 0; i < SCRATCH_SIZE; i++) {
        scratch[i] = 0;
    }
    for (i = 0; i < amount_length; i++) {
        for (j = 0; j < 8; j++) {
            uint8_t k;
            uint16_t shifted_in =
                    (((amount[i] & 0xff) & ((1 << (7 - j)))) != 0) ? (short)1
                                                                   : (short)0;
            for (k = smin; k < nscratch; k++) {
                scratch[k] += ((scratch[k] >= 5) ? 3 : 0);
            }
            if (scratch[smin] >= 8) {
                VALIDATE(smin > 1, ERR_INVALID_DATA);
                smin -= 1;
            }
            for (k = smin; k < nscratch - 1; k++) {
                scratch[k] =
                        ((scratch[k] << 1) & 0xF) | ((scratch[k + 1] >= 8) ? 1 : 0);
            }
            scratch[nscratch - 1] = ((scratch[nscratch - 1] << 1) & 0x0F) |
                                    (shifted_in == 1 ? 1 : 0);
        }
    }

    for (i = 0; i < LOOP1; i++) {
        if (!nonZero && (scratch[offset] == 0)) {
            offset++;
        } else {
            nonZero = 1;
            out[targetOffset++] = scratch[offset++] + '0';
        }
    }
    if (targetOffset == 0) {
        out[targetOffset++] = '0';
    }
    workOffset = offset;
    for (i = 0; i < LOOP2; i++) {
        unsigned char allZero = 1;
        unsigned char k;
        for (k = i; k < LOOP2; k++) {
            if (scratch[workOffset + k] != 0) {
                allZero = 0;
                break;
            }
        }
        if (allZero) {
            break;
        }
        if (!comma) {
            out[targetOffset++] = '.';
            comma = 1;
        }
        out[targetOffset++] = scratch[offset++] + '0';
    }
    return targetOffset;
}
