#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"


#ifdef TARGET_PC
u8 FONT_nes_tex_jyouge[0x80];
#else
u8 FONT_nes_tex_jyouge[] = { 
#include "assets/FONT_nes_tex_jyouge.inc"
};
#endif

