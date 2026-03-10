#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
u8 inv_mwin_ono2_tex[0x200] ATTRIBUTE_ALIGN(32);
#else
u8 inv_mwin_ono2_tex[] ATTRIBUTE_ALIGN(32) = {
#include "assets/inv_mwin_ono2_tex.inc"
};
#endif

#ifdef TARGET_PC
u8 inv_mwin_ono3_tex[0x200];
#else
u8 inv_mwin_ono3_tex[] = {
#include "assets/inv_mwin_ono3_tex.inc"
};
#endif
