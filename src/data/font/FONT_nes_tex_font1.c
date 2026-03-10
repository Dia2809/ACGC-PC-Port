#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"


#ifdef TARGET_PC
u8 FONT_nes_tex_font1[0x6000];
#else
u8 FONT_nes_tex_font1[] = { 
#include "assets/FONT_nes_tex_font1.inc"
};
#endif

