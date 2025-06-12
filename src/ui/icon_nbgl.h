#pragma once

#define EVERSCALE_VARIANT 1
#define VENOM_VARIANT     2
#define TYCHO_VARIANT     3

#if (VARIANT_ID == VENOM_VARIANT)
#define ICON_APP C_app_venom_64px
#elif (VARIANT_ID == TYCHO_VARIANT)
#define ICON_APP C_app_tycho_64px
#else
#define ICON_APP C_app_everscale_64px
#endif