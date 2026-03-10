#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"


#ifdef TARGET_PC
Vtx obj_zassou_c_v[0x40 / sizeof(Vtx)];
#else
Vtx obj_zassou_c_v[] = { 
#include "assets/obj_zassou_c_v.inc"
};
#endif

Gfx obj_zassou_cT_gfx_model[] = { 
gsSPVertex(obj_zassou_c_v, 4, 0),
gsSPNTrianglesInit_5b(2, 0, 1, 2, 0, 2, 3, 0, 0, 0),
gsSPEndDisplayList()
};

