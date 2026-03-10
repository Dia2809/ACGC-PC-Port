#include "version.h"

#ifdef TARGET_PC
char __Creator__[0xC];
#else
char __Creator__[] = {
#include "assets/__Creator__.inc"
};
#endif

#ifdef TARGET_PC
char __DateTime__[0x12];
#else
char __DateTime__[] = {
#include "assets/__DateTime__.inc"
};
#endif


