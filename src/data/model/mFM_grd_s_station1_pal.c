#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
u16 mFM_grd_s_station1_pal[0x20 / sizeof(u16)];
#else
u16 mFM_grd_s_station1_pal[] = {
#include "assets/mFM_grd_s_station1_pal.inc"
};
#endif
