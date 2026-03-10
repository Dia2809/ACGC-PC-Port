#ifdef TARGET_PC
unsigned short brd_9_pal[0x20 / sizeof(unsigned short)] __attribute__((aligned(32)));
#else
extern unsigned short brd_9_pal[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_pal.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_eye1_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_eye1_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_eye1_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_eye2_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_eye2_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_eye2_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_eye3_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_eye3_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_eye3_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_eye4_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_eye4_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_eye4_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_eye5_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_eye5_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_eye5_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_eye6_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_eye6_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_eye6_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_eye7_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_eye7_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_eye7_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_eye8_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_eye8_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_eye8_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char brd_9_tmem_txt[0x500] __attribute__((aligned(32)));
#else
extern unsigned char brd_9_tmem_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/brd_9_tmem_txt.inc"
};
#endif

