#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
u16 inv_mwin_matutake2_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 inv_mwin_matutake2_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/inv_mwin_matutake2_pal.inc"
};
#endif

#ifdef TARGET_PC
u8 inv_mwin_matutake_tex[0x200];
#else
u8 inv_mwin_matutake_tex[] = {
#include "assets/inv_mwin_matutake_tex.inc"
};
#endif
