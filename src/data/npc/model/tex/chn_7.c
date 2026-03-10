#ifdef TARGET_PC
unsigned short chn_7_pal[0x20 / sizeof(unsigned short)] __attribute__((aligned(32)));
#else
extern unsigned short chn_7_pal[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_pal.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_eye1_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_eye1_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_eye1_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_eye2_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_eye2_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_eye2_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_eye3_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_eye3_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_eye3_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_eye4_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_eye4_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_eye4_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_eye5_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_eye5_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_eye5_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_eye6_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_eye6_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_eye6_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_eye7_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_eye7_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_eye7_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_eye8_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_eye8_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_eye8_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char chn_7_tmem_txt[0x4C0] __attribute__((aligned(32)));
#else
extern unsigned char chn_7_tmem_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/chn_7_tmem_txt.inc"
};
#endif

