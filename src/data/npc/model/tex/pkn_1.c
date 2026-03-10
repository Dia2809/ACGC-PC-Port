#ifdef TARGET_PC
unsigned short pkn_1_pal[0x20 / sizeof(unsigned short)] __attribute__((aligned(32)));
#else
extern unsigned short pkn_1_pal[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pkn_1_pal.inc"
};
#endif

#ifdef TARGET_PC
unsigned char pkn_1_tmem_txt[0x680] __attribute__((aligned(32)));
#else
extern unsigned char pkn_1_tmem_txt[] __attribute__((aligned(32))) = {
#include "assets/npc/tex/pkn_1_tmem_txt.inc"
};
#endif

