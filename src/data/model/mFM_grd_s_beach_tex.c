#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
u8 mFM_grd_s_beach_tex[0x400];
#else
u8 mFM_grd_s_beach_tex[] = {
#include "assets/mFM_grd_s_beach_tex.inc"
};
#endif
