#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"

#ifdef TARGET_PC
u16 mFM_obj_tree_01_pal[0x1C0 / sizeof(u16)] ATTRIBUTE_ALIGN(32);
#else
u16 mFM_obj_tree_01_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/mFM_obj_tree_01_pal.inc"
};
#endif
