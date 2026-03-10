#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
u8 log_win_nintendo2_tex[0x400];
#else
u8 log_win_nintendo2_tex[] = {
#include "assets/log_win_nintendo2_tex.inc"
};
#endif
