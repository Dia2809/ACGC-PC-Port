#include "m_field_make.h"

#ifdef TARGET_PC
u16 mFM_beach_pal[12][16];
#else
extern u16 mFM_beach_pal[mFM_FIELD_PAL_NUM][16] = {
  {
    #include "assets/mFM_beach_pal_0.inc"
  },
  {
    #include "assets/mFM_beach_pal_1.inc"
  },
  {
    #include "assets/mFM_beach_pal_2.inc"
  },
  {
    #include "assets/mFM_beach_pal_3.inc"
  },
  {
    #include "assets/mFM_beach_pal_4.inc"
  },
  {
    #include "assets/mFM_beach_pal_5.inc"
  },
  {
    #include "assets/mFM_beach_pal_6.inc"
  },
  {
    #include "assets/mFM_beach_pal_7.inc"
  },
  {
    #include "assets/mFM_beach_pal_8.inc"
  },
  {
    #include "assets/mFM_beach_pal_9.inc"
  },
  {
    #include "assets/mFM_beach_pal_10.inc"
  },
  {
    #include "assets/mFM_beach_pal_11.inc"
  }
};
#endif

#ifdef TARGET_PC
extern void pc_load_asset(const char*, void*, unsigned int, unsigned int, int, int);
void _pc_load_src_data_field_bg_beach_pal_c(void) {
    pc_load_asset("assets/mFM_beach_pal.bin", mFM_beach_pal, 0x180, 0x8E9AE8, 0, 1);
}
#endif
