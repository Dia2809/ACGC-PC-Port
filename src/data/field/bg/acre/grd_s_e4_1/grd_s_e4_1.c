#include "libforest/gbi_extensions.h"

extern u8 cliff_pal_dummy[];
extern u8 earth_pal_dummy[];
extern u8 grass_tex_dummy[];
extern u8 cliff_tex_dummy[];

#ifdef TARGET_PC
static Vtx grd_s_e4_1_v[0x160 / sizeof(Vtx)];
#else
static Vtx grd_s_e4_1_v[] = {
#include "assets/field/bg/grd_s_e4_1_v.inc"
};
#endif

extern Gfx grd_s_e4_1_modelT[] = {
    gsSPEndDisplayList(),
};

extern Gfx grd_s_e4_1_model[] = {
    gsSPTexture(0, 0, 0, 0, G_ON),
    gsDPSetRenderMode(G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2),
    gsDPSetCombineLERP(TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0, PRIMITIVE, 0, COMBINED, 0, 0, 0, 0, COMBINED),
    gsDPLoadTLUT_Dolphin(15, 16, 1, earth_pal_dummy),
    gsDPLoadTextureBlock_4b_Dolphin(grass_tex_dummy, G_IM_FMT_CI, 32, 32, 15, GX_REPEAT, GX_REPEAT, 0, 0),
    gsDPSetPrimColor(0, 128, 255, 255, 255, 255),
    gsSPLoadGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BACK | G_FOG | G_LIGHTING | G_SHADING_SMOOTH),
    gsSPVertex(&grd_s_e4_1_v[0], 7, 0),
    gsSPNTrianglesInit_5b(
        5, // tri count
        0, 1, 2, // tri0
        1, 3, 2, // tri1
        3, 4, 2 // tri2
    ),
    gsSPNTriangles_5b(
        3, 5, 4, // tri0
        5, 6, 4, // tri1
        0, 0, 0, // tri2
        0, 0, 0 // tri3
    ),
    gsDPLoadTLUT_Dolphin(15, 16, 1, cliff_pal_dummy),
    gsDPLoadTextureBlock_4b_Dolphin(cliff_tex_dummy, G_IM_FMT_CI, 64, 64, 15, GX_REPEAT, GX_REPEAT, 0, 0),
    gsSPVertex(&grd_s_e4_1_v[7], 15, 0),
    gsSPNTrianglesInit_5b(
        15, // tri count
        0, 1, 2, // tri0
        0, 3, 1, // tri1
        3, 4, 1 // tri2
    ),
    gsSPNTriangles_5b(
        3, 5, 4, // tri0
        5, 6, 4, // tri1
        1, 7, 8, // tri2
        1, 8, 2 // tri3
    ),
    gsSPNTriangles_5b(
        6, 9, 10, // tri0
        6, 10, 4, // tri1
        1, 4, 10, // tri2
        1, 10, 7 // tri3
    ),
    gsSPNTriangles_5b(
        0, 11, 3, // tri0
        3, 12, 5, // tri1
        3, 11, 12, // tri2
        11, 13, 14 // tri3
    ),
    gsSPEndDisplayList(),
};

#ifdef TARGET_PC
extern void pc_load_asset(const char*, void*, unsigned int, unsigned int, int, int);
void _pc_load_src_data_field_bg_acre_grd_s_e4_1_grd_s_e4_1_c(void) {
    pc_load_asset("assets/field/bg/grd_s_e4_1_v.bin", grd_s_e4_1_v, 0x160, 0x9D42A8, 0, 2);
}
#endif
