#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "evw_anime.h"
#include "c_keyframe.h"
#include "ac_npc.h"
#include "ef_effect_control.h"

#ifdef TARGET_PC
static Vtx act_m_maimai_v[0x100 / sizeof(Vtx)];
#else
static Vtx act_m_maimai_v[] = {
#include "assets/act_m_maimai2/act_m_maimai_v.inc"
};
#endif

#ifdef TARGET_PC
static u16 act_m_maimai_pal[0x20 / sizeof(u16)];
#else
static u16 act_m_maimai_pal[] = {
#include "assets/act_m_maimai2/act_m_maimai_pal.inc"
};
#endif

#ifdef TARGET_PC
static u8 act_m_maimai_tex[0x200];
#else
static u8 act_m_maimai_tex[] = {
#include "assets/act_m_maimai2/act_m_maimai_tex.inc"
};
#endif

Gfx act_m_mu_maimaiT_model[] = {
    gsSPTexture(65535, 65535, 0, G_TX_RENDERTILE, G_ON),
    gsDPPipeSync(),
    gsDPSetCombineLERP(TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, COMBINED, 0, SHADE, 0, 0, 0, 0, COMBINED),
    gsDPSetPrimColor(0, 255, 255, 255, 255, 255),
    gsDPSetRenderMode(G_RM_FOG_SHADE_A, G_RM_AA_ZB_TEX_EDGE2),
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_RGBA16),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, act_m_maimai_pal),
    gsDPTileSync(),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 496, G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK,
                G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD),
    gsDPLoadSync(),
    gsDPLoadTLUTCmd(G_TX_LOADTILE, 15),
    gsDPPipeSync(),
    gsDPSetTextureImage(G_IM_FMT_CI, G_IM_SIZ_16b, 1, act_m_maimai_tex),
    gsDPSetTile(G_IM_FMT_CI, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_MIRROR | G_TX_CLAMP, 5, G_TX_NOLOD,
                G_TX_MIRROR | G_TX_WRAP, 5, G_TX_NOLOD),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 255, 1024),
    gsDPPipeSync(),
    gsDPSetTile(G_IM_FMT_CI, G_IM_SIZ_4b, 2, 0, G_TX_RENDERTILE, 15, G_TX_MIRROR | G_TX_CLAMP, 5, G_TX_NOLOD,
                G_TX_MIRROR | G_TX_WRAP, 5, G_TX_NOLOD),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 124, 124),
    gsSPLoadGeometryMode(G_ZBUFFER | G_SHADE | G_FOG | G_LIGHTING | G_SHADING_SMOOTH | G_DECAL_LEQUAL),
    gsSPVertex(act_m_maimai_v, 16, 0),
    gsSP2Triangles(0, 1, 2, 0, 3, 4, 5, 0),
    gsSP2Triangles(3, 5, 6, 0, 3, 7, 1, 0),
    gsSP1Triangle(8, 2, 1, 0),
    gsSPLoadGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BACK | G_FOG | G_LIGHTING | G_SHADING_SMOOTH | G_DECAL_LEQUAL),
    gsSP2Triangles(9, 10, 11, 0, 9, 12, 10, 0),
    gsSP2Triangles(12, 13, 10, 0, 13, 14, 10, 0),
    gsSP2Triangles(14, 15, 10, 0, 15, 11, 10, 0),
    gsSPEndDisplayList(),
};

#ifdef TARGET_PC
extern void pc_load_asset(const char*, void*, unsigned int, unsigned int, int, int);
void _pc_load_src_data_model_act_m_maimai2_c(void) {
    pc_load_asset("assets/act_m_maimai2/act_m_maimai_v.bin", act_m_maimai_v, 0x100, 0xB4F7B0, 0, 2);
    pc_load_asset("assets/act_m_maimai2/act_m_maimai_pal.bin", act_m_maimai_pal, 0x20, 0xB4F8B0, 0, 1);
    pc_load_asset("assets/act_m_maimai2/act_m_maimai_tex.bin", act_m_maimai_tex, 0x200, 0xB4F8D0, 0, 0);
}
#endif
