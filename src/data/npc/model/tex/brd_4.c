#ifdef TARGET_PC
unsigned short brd_4_pal[0x20 / sizeof(unsigned short)] __attribute__((aligned(32)));
#else
extern unsigned short brd_4_pal[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_pal.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_eye1_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_eye1_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_eye1_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_eye2_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_eye2_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_eye2_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_eye3_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_eye3_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_eye3_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_eye4_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_eye4_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_eye4_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_eye5_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_eye5_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_eye5_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_eye6_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_eye6_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_eye6_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_eye7_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_eye7_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_eye7_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_eye8_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_eye8_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_eye8_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_4_tmem_txt[0x500] __attribute__((aligned(32)));
#else
extern unsigned char brd_4_tmem_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_4_tmem_txt.inc"
};
#endif

