#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
u8 museum_fish_actable[0x30];
#else
u8 museum_fish_actable[] = {
#include "assets/museum_fish_actable.inc"
};
#endif
