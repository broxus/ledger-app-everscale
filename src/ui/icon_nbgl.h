#pragma once

#define VENOM_VARIANT     1
#define EVERSCALE_VARIANT 2

#if (VARIANT_ID == VENOM_VARIANT)
#define ICON_APP C_app_venom_64px
#else
#define ICON_APP C_app_everscale_64px
#endif