#ifdef TARGET_PC
unsigned short wol_6_pal[0x20 / sizeof(unsigned short)] __attribute__((aligned(32)));
#else
extern unsigned short wol_6_pal[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_pal.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_eye1_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_eye1_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_eye1_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_eye2_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_eye2_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_eye2_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_eye3_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_eye3_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_eye3_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_eye4_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_eye4_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_eye4_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_eye5_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_eye5_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_eye5_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_eye6_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_eye6_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_eye6_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_eye7_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_eye7_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_eye7_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_eye8_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_eye8_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_eye8_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char wol_6_tmem_txt[0x400] __attribute__((aligned(32)));
#else
extern unsigned char wol_6_tmem_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/wol_6_tmem_txt.inc"
};
#endif

