#ifdef TARGET_PC
unsigned short pgn_3_pal[0x20 / sizeof(unsigned short)] __attribute__((aligned(32)));
#else
extern unsigned short pgn_3_pal[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_pal.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_eye1_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_eye1_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_eye1_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_eye2_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_eye2_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_eye2_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_eye3_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_eye3_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_eye3_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_eye4_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_eye4_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_eye4_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_eye5_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_eye5_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_eye5_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_eye6_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_eye6_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_eye6_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_eye7_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_eye7_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_eye7_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_eye8_TA_tex_txt[0x100] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_eye8_TA_tex_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_eye8_TA_tex_txt.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pgn_3_tmem_txt[0x580] __attribute__((aligned(32)));
#else
extern unsigned char pgn_3_tmem_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pgn_3_tmem_txt.inc"
};
#endif

