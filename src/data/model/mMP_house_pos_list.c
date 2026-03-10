#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
u8 mMP_house_pos_list[0x978];
#else
u8 mMP_house_pos_list[] = {
#include "assets/mMP_house_pos_list.inc"
};
#endif
