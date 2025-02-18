#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"

#define CLA                       0xE0
#define INS_GET_APP_CONFIGURATION 0x01
#define INS_GET_PUBLIC_KEY        0x02
#define INS_SIGN                  0x03
#define INS_GET_ADDRESS           0x04
#define INS_SIGN_TRANSACTION      0x05

#define OFFSET_CLA   0
#define OFFSET_INS   1
#define OFFSET_P1    2
#define OFFSET_P2    3
#define OFFSET_LC    4
#define OFFSET_CDATA 5

#define APPVERSION_LEN 3
