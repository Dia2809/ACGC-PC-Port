#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
u16 cal_win_tuki10_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki10_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki10_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki11_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki11_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki11_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki12_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki12_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki12_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki2_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki2_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki2_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki3_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki3_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki3_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki4_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki4_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki4_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki5_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki5_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki5_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki6_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki6_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki6_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki7_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki7_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki7_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki8_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki8_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki8_pal.inc"
};
#endif

#ifdef TARGET_PC
u16 cal_win_tuki9_pal[0x20 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 cal_win_tuki9_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki9_pal.inc"
};
#endif

#ifdef TARGET_PC
u8 cal_win_tuki2_tex[0x200] ATTRIBUTE_ALIGN(32);
#else
u8 cal_win_tuki2_tex[] ATTRIBUTE_ALIGN(32) = {
#include "assets/cal_win_tuki2_tex.inc"
};
#endif
