#include "libc64/math64.h"
#ifndef TARGET_PC
#include "MSL_C/w_math.h"
#endif

#ifdef TARGET_PC
/* PC: provide implementations using standard math.h (included via math64.h) */
f32 fatan2(f32 x, f32 y) {
    return atan2f(x, y);
}

f32 game_fsqrt(f32 x) {
    return sqrtf(x);
}

f32 facos(f32 x) {
    return acosf(x);
}
#else
f32 fatan2(f32 x, f32 y) {
    return atan2(x, y);
}

f32 game_fsqrt(f32 x) {
    return sqrtf(x);
}

f32 facos(f32 x) {
    return acos(x);
}
#endif
