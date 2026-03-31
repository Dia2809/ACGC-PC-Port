#include "libforest/emu64/emu64.hpp"

#include "boot.h"
#include "terminal.h"
#include "MSL_C/w_math.h"

#ifdef TARGET_PC
/* Executable image range from pc_main.c — BSS/data can collide with N64 segments */
extern "C" uintptr_t pc_image_base;
extern "C" uintptr_t pc_image_end;

#if UINTPTR_MAX > 0xFFFFFFFFu
#include "pc_gbi_ptr.h"

/* === 64-bit seg2k0 ===
 * On 64-bit, real PC pointers are always above 0x0FFFFFFF. N64 segment
 * addresses are always 0x03XXXXXX-0x0FXXXXXX. However, truncated 64-bit
 * pointers (low 32 bits of a real pointer stored in a u32 Gfx field) can
 * fall ANYWHERE in the 32-bit range, including the segment address range.
 *
 * Strategy: first try segment resolution. If the segment base is 0 (unused
 * segment), try recovering the full pointer from the truncated value using
 * the known image/arena base addresses. If all else fails and the address is
 * not mapped in this process (e.g. a 32-bit armhf arena ptr on an arm64
 * host), return a dummy buffer rather than crash. */
uintptr_t emu64::seg2k0(uintptr_t segadr) {
    /* 1. Basic Sanity Checks */
    if (segadr > 0xFFFFFFFFu) return segadr; // Already 64-bit
    if (segadr == 0) return 0;

    /* 2. Try N64 segment resolution (The 0x03-0x0F range) */
    if (segadr >= 0x03000000u && segadr <= 0x0FFFFFFFu) {
        uintptr_t seg = (segadr >> 24) & 0xF;
        uintptr_t offset = segadr & 0xFFFFFF;

        if (this->segments[seg] != 0) {
            uintptr_t resolved = this->segments[seg] + offset;

            /* Verify the resolved pointer is actually mapped before using it.
               A stale armhf segment base stored in segments[] would produce
               an unmapped address here; fall through to recovery in that case. */
            unsigned char vec;
            uintptr_t page_addr = resolved & ~(uintptr_t)0xFFF;
            if (mincore((void*)page_addr, 1, &vec) == 0) {
                this->resolved_addresses++;
                return resolved;
            }
            /* Resolved address is not mapped — fall through */
        }
    }

    /* 3. Recovery Logic: reconstruct a full 64-bit pointer from a truncated
       32-bit value using the known image/arena base addresses. */
    uintptr_t recovered = pc_gbi_recover_ptr((unsigned int)segadr);
    if (recovered != segadr) {
        return recovered;
    }

    /* 4. Last resort: check whether segadr itself is a valid mapped address
       (e.g. a 32-bit host pointer below 4 GB that recovery didn't recognise). */
    {
        unsigned char vec;
        if (mincore((void*)(segadr & ~(uintptr_t)0xFFF), 1, &vec) == 0) {
            return segadr;
        }
    }

    /* 5. Completely unresolvable (e.g. an armhf arena address like 0x8B027800
       running inside an arm64 process). Return a zero-filled dummy buffer so
       the caller receives valid memory and doesn't SEGV; the affected geometry
       or texture will render incorrectly but the game keeps running. */
    static unsigned char seg2k0_fallback[64] __attribute__((aligned(64)));
    return (uintptr_t)seg2k0_fallback;
}
#else
/* === 32-bit seg2k0 (Windows/ARMHF) ===
 * On 32-bit, PC pointers can collide with N64 segment addresses. */

/* Arena range from pc_os.c — heap pointers in arena also bypass segment resolution */
extern "C" unsigned char* pc_arena_base;
extern "C" unsigned char* pc_arena_end;

#define SEG2K0_PAGE_CACHE_SIZE 32
static struct { u32 page; u8 committed; } seg2k0_page_cache[SEG2K0_PAGE_CACHE_SIZE];
static int seg2k0_cache_next = 0;

static int seg2k0_is_committed(u32 addr) {
    u32 page = addr & ~0xFFF;
    for (int i = 0; i < SEG2K0_PAGE_CACHE_SIZE; i++) {
        if (seg2k0_page_cache[i].page == page) {
            return seg2k0_page_cache[i].committed;
        }
    }
    int committed = 0;
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((void*)(uintptr_t)addr, &mbi, sizeof(mbi)) > 0 && mbi.State == MEM_COMMIT) {
        committed = 1;
    }
#else
    unsigned char vec = 0;
    // Fix: cast to uintptr_t to ensure proper pointer size on all ARM variants
    committed = (mincore((void*)(uintptr_t)page, 1, &vec) == 0);
#endif
    seg2k0_page_cache[seg2k0_cache_next].page = page;
    seg2k0_page_cache[seg2k0_cache_next].committed = committed;
    seg2k0_cache_next = (seg2k0_cache_next + 1) % SEG2K0_PAGE_CACHE_SIZE;
    return committed;
}

uintptr_t emu64::seg2k0(uintptr_t segadr) {
    if ((segadr >> 28) != 0 || segadr < 0x03000000) {
        return segadr;
    }

    if (segadr >= pc_image_base && segadr < pc_image_end) {
        return segadr;
    }

    if (pc_arena_base && segadr >= (uintptr_t)pc_arena_base &&
        segadr < (uintptr_t)pc_arena_end) {
        return segadr;
    }

    u32 seg = (segadr >> 24) & 0xF;
    u32 offset = segadr & 0xFFFFFF;

    if (this->segments[seg] == 0) {
        return segadr;
    }

    uintptr_t resolved = this->segments[seg] + offset;

    if (offset > 0x80000 && seg2k0_is_committed((u32)segadr)) {
        return segadr;
    }

    if (seg2k0_is_committed((u32)resolved)) {
        this->resolved_addresses++;
        return resolved;
    }

    if (seg2k0_is_committed((u32)segadr)) {
        return segadr;
    }

    this->resolved_addresses++;
    return resolved;
}
#endif /* UINTPTR_MAX */
#else
u32 emu64::seg2k0(u32 segadr) {
    u32 k0;
    if ((segadr >> 28) == 0) {
        if (segadr < 0x03000000) {
            this->Printf0(VT_COL(RED, WHITE) "segadr=%08x" VT_RST "\n", segadr);
            this->panic("segadr is over 0x03000000.", __FILE__, 20);
            k0 = segadr + 0x80000000;
        } else {
            k0 = (u32)this->segments[(segadr >> 24) & 0xF] + (segadr & 0xFFFFFF);
        }
        this->resolved_addresses++;
    } else {
        k0 = segadr;
    }

    if ((k0 >> 31) == 0 || k0 < 0x80000000 || k0 >= 0x83000000) {
        this->Printf0("異常なアドレスです。%08x -> %08x\n", segadr, k0);
        this->panic("異常なアドレスです。", __FILE__, 77);
        this->abnormal_addresses++;
    }
    return k0;
}
#endif

// --- Math and Matrix functions follow ---

void guMtxXFM1F_dol(MtxP mtx, float x, float y, float z, float* ox, float* oy, float* oz) {
    *ox = mtx[0][0] * x + mtx[0][1] * y + mtx[0][2] * z + mtx[0][3];
    *oy = mtx[1][0] * x + mtx[1][1] * y + mtx[1][2] * z + mtx[1][3];
    *oz = mtx[2][0] * x + mtx[2][1] * y + mtx[2][2] * z + mtx[2][3];
}

void guMtxXFM1F_dol7(MtxP mtx, float x, float y, float z, float* ox, float* oy, float* oz) {
    GC_Mtx inv;
    PSMTXInverse(mtx, inv);
    *ox = inv[0][0] * x + inv[0][1] * y + inv[0][2] * z + inv[0][3];
    *oy = inv[1][0] * x + inv[1][1] * y + inv[1][2] * z + inv[1][3];
    *oz = inv[2][0] * x + inv[2][1] * y + inv[2][2] * z + inv[2][3];
}

void guMtxXFM1F_dol2(MtxP mtx, GXProjectionType type, float x, float y, float z, float* ox, float* oy, float* oz) {
    if (type == GX_PERSPECTIVE) {
        f32 s = -1.0f / z;
        *ox = mtx[0][0] * x * s - mtx[0][2];
        *oy = mtx[1][1] * y * s - mtx[1][2];
        *oz = mtx[2][3] * s - mtx[2][2];
    } else {
        *ox = mtx[0][0] * x + mtx[0][3];
        *oy = mtx[1][1] * y + mtx[1][3];
        *oz = mtx[2][2] * z + mtx[2][3];
    }
}

void guMtxXFM1F_dol2w(MtxP mtx, GXProjectionType type, float x, float y, float z, float* ox, float* oy, float* oz, float* ow) {
    if (type == GX_PERSPECTIVE) {
        *ox = mtx[0][0] * x + mtx[0][2] * z;
        *oy = mtx[1][1] * y + mtx[1][2] * z;
        *oz = mtx[2][3] + mtx[2][2] * z;
        *ow = -z;
    } else {
        *ox = mtx[0][0] * x + mtx[0][3];
        *oy = mtx[1][1] * y + mtx[1][3];
        *oz = mtx[2][2] * z + mtx[2][3];
        *ow = 1.0f;
    }
}

float guMtxXFM1F_dol3(MtxP mtx, GXProjectionType type, float z) {
    if (type == GX_PERSPECTIVE) {
        return -mtx[2][3] / (z + mtx[2][2]);
    } else {
        return (z - mtx[2][3]) / mtx[2][2];
    }
}

void guMtxXFM1F_dol6w(MtxP mtx, GXProjectionType type, float x, float y, float z, float w, float* ox, float* oy, float* oz, float* ow) {
    if (type == GX_PERSPECTIVE) {
        float xScale = mtx[0][0], yScale = mtx[1][1], zScale = mtx[2][2];
        float xRatioScaling = mtx[0][2], yRatioScaling = mtx[1][2], zSkew = mtx[2][3];
        *ox = (yScale * zSkew * (x + xRatioScaling * w)) / (xScale * (yScale * zSkew));
        *oy = (xScale * zSkew * (y + yRatioScaling * w)) / (xScale * (yScale * zSkew));
        *oz = -w;
        *ow = (xScale * yScale * (z + zScale * w)) / (xScale * (yScale * zSkew));
    } else {
        float xScale = mtx[0][0], xSkew = mtx[0][3], yScale = mtx[1][1], ySkew = mtx[1][3], zScale = mtx[2][2], zSkew = mtx[2][3];
        float n = 1.0f / (xScale * yScale * zScale);
        *ox = n * (yScale * zScale * (x - xSkew));
        *oy = n * (zScale * xScale * (y - ySkew));
        *oz = n * (xScale * yScale * (z - zSkew));
        *ow = 1.0f;
    }
}

void guMtxXFM1F_dol6w1(MtxP mtx, GXProjectionType type, float x, float y, float z, float w, float* ox, float* oy, float* oz) {
    if (type == GX_PERSPECTIVE) {
        float xScale = mtx[0][0], yScale = mtx[1][1], zScale = mtx[2][2], xRatioScaling = mtx[0][2], yRatioScaling = mtx[1][2], zSkew = mtx[2][3];
        float temp_f7 = 1.0f / (xScale * yScale * (z + (zScale * w)));
        *ox = temp_f7 * (yScale * zSkew * (x + (xRatioScaling * w)));
        *oy = temp_f7 * (xScale * zSkew * (y + (yRatioScaling * w)));
        *oz = temp_f7 * (yScale * zSkew * xScale * -w);
    } else {
        *ox = (x - mtx[0][3]) / mtx[0][0];
        *oy = (y - mtx[1][3]) / mtx[1][1];
        *oz = (z - mtx[2][3]) / mtx[2][2];
    }
}

void guMtxNormalize(GC_Mtx mtx) {
    for (int i = 0; i < 3; i++) {
        float magnitude = sqrtf(mtx[i][0] * mtx[i][0] + mtx[i][1] * mtx[i][1] + mtx[i][2] * mtx[i][2]);
        mtx[i][0] *= 1.0f / magnitude;
        mtx[i][1] *= 1.0f / magnitude;
        mtx[i][2] *= 1.0f / magnitude;
    }
}

void N64Mtx_to_DOLMtx(const Mtx* n64, MtxP gc) {
    s16* fixed = ((s16*)n64) + 0;
    u16* frac = ((u16*)n64) + 16;
    for (int i = 0; i < 4; i++) {
#ifdef TARGET_PC
        gc[0][i] = (float)fixed[1] + (float)frac[1] * (1.0f / 65536.0f);
        gc[1][i] = (float)fixed[0] + (float)frac[0] * (1.0f / 65536.0f);
        gc[2][i] = (float)fixed[3] + (float)frac[3] * (1.0f / 65536.0f);
#else
        gc[0][i] = (float)fixed[0] + (float)frac[0] * (1.0f / 65536.0f);
        gc[1][i] = (float)fixed[1] + (float)frac[1] * (1.0f / 65536.0f);
        gc[2][i] = (float)fixed[2] + (float)frac[2] * (1.0f / 65536.0f);
#endif
        fixed += 4;
        frac += 4;
    }
}