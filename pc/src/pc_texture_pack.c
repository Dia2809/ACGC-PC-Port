/* pc_texture_pack.c - Dolphin-compatible HD texture pack loader
 *
 * Filename: tex1_{W}x{H}_{hash}[_{tlut_hash}]_{fmt}.dds
 * Hashes match Dolphin's XXHash64 (seed=0). CI textures use min/max palette
 * index scan for TLUT hash (only used entries, in BE byte order).
 * DDS: BC7, BC1/DXT1, BC3/DXT5, or uncompressed RGBA. */
#include "pc_texture_pack.h"
#include "pc_gx_internal.h"
#include "pc_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- XXHash64 (seed=0, matches Dolphin's GetHash64) --- */

typedef unsigned long long xxh_u64;
typedef unsigned int xxh_u32;

#define XXH_PRIME64_1 0x9E3779B185EBCA87ULL
#define XXH_PRIME64_2 0xC2B2AE3D27D4EB4FULL
#define XXH_PRIME64_3 0x165667B19E3779F9ULL
#define XXH_PRIME64_4 0x85EBCA77C2B2AE63ULL
#define XXH_PRIME64_5 0x27D4EB2F165667C5ULL

static inline xxh_u64 xxh_read64(const void* p) {
    xxh_u64 val;
    memcpy(&val, p, 8);
    return val; /* LE platform = native read */
}

static inline xxh_u32 xxh_read32(const void* p) {
    xxh_u32 val;
    memcpy(&val, p, 4);
    return val;
}

static inline xxh_u64 xxh_rotl64(xxh_u64 x, int r) {
    return (x << r) | (x >> (64 - r));
}

static inline xxh_u64 xxh_round(xxh_u64 acc, xxh_u64 input) {
    acc += input * XXH_PRIME64_2;
    acc = xxh_rotl64(acc, 31);
    acc *= XXH_PRIME64_1;
    return acc;
}

static inline xxh_u64 xxh_merge_round(xxh_u64 acc, xxh_u64 val) {
    val = xxh_round(0, val);
    acc ^= val;
    acc = acc * XXH_PRIME64_1 + XXH_PRIME64_4;
    return acc;
}

static xxh_u64 xxhash64(const void* input, int len) {
    const unsigned char* p = (const unsigned char*)input;
    const unsigned char* end = p + len;
    xxh_u64 h64;

    if (len >= 32) {
        const unsigned char* limit = end - 32;
        xxh_u64 v1 = 0 + XXH_PRIME64_1 + XXH_PRIME64_2; /* seed=0 */
        xxh_u64 v2 = 0 + XXH_PRIME64_2;
        xxh_u64 v3 = 0;
        xxh_u64 v4 = 0 - XXH_PRIME64_1;

        do {
            v1 = xxh_round(v1, xxh_read64(p));      p += 8;
            v2 = xxh_round(v2, xxh_read64(p));      p += 8;
            v3 = xxh_round(v3, xxh_read64(p));      p += 8;
            v4 = xxh_round(v4, xxh_read64(p));      p += 8;
        } while (p <= limit);

        h64 = xxh_rotl64(v1, 1) + xxh_rotl64(v2, 7) +
              xxh_rotl64(v3, 12) + xxh_rotl64(v4, 18);
        h64 = xxh_merge_round(h64, v1);
        h64 = xxh_merge_round(h64, v2);
        h64 = xxh_merge_round(h64, v3);
        h64 = xxh_merge_round(h64, v4);
    } else {
        h64 = 0 + XXH_PRIME64_5; /* seed + PRIME5 */
    }

    h64 += (xxh_u64)len;

    /* Remaining 8-byte chunks */
    while (p + 8 <= end) {
        xxh_u64 k1 = xxh_round(0, xxh_read64(p));
        h64 ^= k1;
        h64 = xxh_rotl64(h64, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        p += 8;
    }

    /* Remaining 4-byte chunk */
    if (p + 4 <= end) {
        h64 ^= (xxh_u64)xxh_read32(p) * XXH_PRIME64_1;
        h64 = xxh_rotl64(h64, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        p += 4;
    }

    /* Remaining bytes */
    while (p < end) {
        h64 ^= (xxh_u64)(*p) * XXH_PRIME64_5;
        h64 = xxh_rotl64(h64, 11) * XXH_PRIME64_1;
        p++;
    }

    /* Avalanche */
    h64 ^= h64 >> 33;
    h64 *= XXH_PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= XXH_PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}

/* --- DDS format constants --- */

#define DDS_MAGIC          0x20534444  /* "DDS " */
#define DDS_HEADER_SIZE    124
#define DDS_DX10_SIZE      20
#define DDPF_FOURCC        0x04

#define DXGI_FORMAT_R8G8B8A8_UNORM   28
#define DXGI_FORMAT_B8G8R8A8_UNORM   87
#define DXGI_FORMAT_BC1_UNORM        71
#define DXGI_FORMAT_BC3_UNORM        77
#define DXGI_FORMAT_BC7_UNORM        98

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif
#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#define GL_COMPRESSED_RGBA_BPTC_UNORM     0x8E8C
#endif

static int g_has_bc7 = 0;
static int g_has_s3tc = 0;

/* ---- Software decompressors: BC1/BC3/BC7 fallback when GPU lacks extensions ---- */

static unsigned long long sw_bc_bits(const unsigned char *src, int *bp, int n) {
    unsigned long long v = 0;
    for (int i = 0; i < n; i++) {
        v |= (unsigned long long)((src[*bp >> 3] >> (*bp & 7)) & 1) << i;
        ++*bp;
    }
    return v;
}

static unsigned char sw_expand8(unsigned char v, int n) {
    if (n >= 8) return v;
    return (unsigned char)((v << (8 - n)) | (v >> (2 * n - 8)));
}

static void sw_bc1_block(const unsigned char *s, unsigned char *d, int stride) {
    unsigned short c0 = (unsigned short)(s[0] | (s[1] << 8));
    unsigned short c1 = (unsigned short)(s[2] | (s[3] << 8));
    unsigned char r[4], g[4], b[4], a[4];
    r[0] = sw_expand8((c0 >> 11) & 31, 5); g[0] = sw_expand8((c0 >> 5) & 63, 6); b[0] = sw_expand8(c0 & 31, 5);
    r[1] = sw_expand8((c1 >> 11) & 31, 5); g[1] = sw_expand8((c1 >> 5) & 63, 6); b[1] = sw_expand8(c1 & 31, 5);
    if (c0 > c1) {
        r[2]=(unsigned char)((2*r[0]+r[1]+1)/3); g[2]=(unsigned char)((2*g[0]+g[1]+1)/3); b[2]=(unsigned char)((2*b[0]+b[1]+1)/3);
        r[3]=(unsigned char)((r[0]+2*r[1]+1)/3); g[3]=(unsigned char)((g[0]+2*g[1]+1)/3); b[3]=(unsigned char)((b[0]+2*b[1]+1)/3);
        a[0]=a[1]=a[2]=a[3]=255;
    } else {
        r[2]=(unsigned char)((r[0]+r[1]+1)/2); g[2]=(unsigned char)((g[0]+g[1]+1)/2); b[2]=(unsigned char)((b[0]+b[1]+1)/2);
        r[3]=g[3]=b[3]=0; a[0]=a[1]=a[2]=255; a[3]=0;
    }
    unsigned int bits = (unsigned int)(s[4]|(s[5]<<8)|(s[6]<<16)|(s[7]<<24));
    for (int row=0;row<4;row++) for (int col=0;col<4;col++) {
        int idx=(bits>>(2*(row*4+col)))&3;
        unsigned char *p=d+row*stride+col*4;
        p[0]=r[idx]; p[1]=g[idx]; p[2]=b[idx]; p[3]=a[idx];
    }
}

static void sw_bc3_block(const unsigned char *s, unsigned char *d, int stride) {
    unsigned char a0=s[0], a1=s[1], av[8];
    av[0]=a0; av[1]=a1;
    if (a0 > a1) {
        for (int i=1;i<=6;i++) av[i+1]=(unsigned char)(((7-i)*(int)a0+i*(int)a1+3)/7);
    } else {
        for (int i=1;i<=4;i++) av[i+1]=(unsigned char)(((5-i)*(int)a0+i*(int)a1+2)/5);
        av[6]=0; av[7]=255;
    }
    unsigned long long ab = (unsigned long long)s[2] | ((unsigned long long)s[3]<<8) |
                            ((unsigned long long)s[4]<<16) | ((unsigned long long)s[5]<<24) |
                            ((unsigned long long)s[6]<<32) | ((unsigned long long)s[7]<<40);
    sw_bc1_block(s+8, d, stride);
    for (int row=0;row<4;row++) for (int col=0;col<4;col++)
        d[row*stride+col*4+3] = av[(ab>>(3*(row*4+col)))&7];
}

/* BC7 partition/anchor tables (from DirectX BC7 spec / OpenGL ARB_texture_compression_bptc) */
static const unsigned short s_bc7_p2[64] = { /* 2-subset shapes, bit i = subset of pixel i */
    0xCCCC,0x8888,0xEEEE,0xECC8,0xC880,0xFEEC,0xFEC8,0xEC80,
    0xC800,0xFFEC,0xFE80,0xE800,0xFFE8,0xFF00,0xFFF0,0xF000,
    0xF710,0x008E,0x7100,0x08CE,0x008C,0x7310,0x3100,0x8CCE,
    0x088C,0x3110,0x6666,0x366C,0x17E8,0x0FF0,0x718E,0x399C,
    0xAAAA,0xF0F0,0x5A5A,0x33CC,0x3C3C,0x55AA,0x9696,0xA55A,
    0x73CE,0x13C8,0x324C,0x3BCE,0x69A0,0x4380,0x0EC8,0x194C,
    0x384C,0xE74C,0x738C,0x3990,0x1340,0xE380,0x1390,0xF340,
    0xABCC,0x4BBC,0x1DDC,0x0E74,0xBBCC,0x1EE4,0xCBC4,0x2C78
};
static const unsigned int s_bc7_p3[64] = { /* 3-subset shapes, 2 bits per pixel (pixel i = bits 2i+1:2i) */
    0xAA685050,0x6A5A5040,0x5A5A4200,0x5450A0A8,0xA5A50000,0xA0A05050,0x5555A0A0,0x5A5A5050,
    0xAA550000,0xAA555500,0xAAAA5500,0x90909090,0x94949494,0xA4A4A4A4,0xA9A59450,0x2A0A4250,
    0xA5945040,0x0A425054,0xA5A5A500,0x55A0A0A0,0xA8A85454,0x6A6A4040,0xA4A45000,0x1A1A0500,
    0x0050A4A4,0xAAA59090,0x14696914,0x69691400,0xA08585A0,0xAA821414,0x50A4A450,0x6A5A0200,
    0xA9A58000,0x5090A0A8,0xA8A09050,0x24242424,0x00AA5500,0x24924924,0x24499224,0x50A50A50,
    0x500AA550,0xAAAA4444,0x66660000,0xA5A0A5A0,0x50A050A0,0x69286928,0x44AAAA44,0x66666600,
    0xAA444444,0x54A854A8,0x95809580,0x96A0A096,0xA850A850,0xAA485500,0x1A1A1A9A,0x4624A054,
    0x9686A0A0,0x9A1A8500,0x1AA04250,0xAA910000,0xA4A04250,0x14681968,0x00142868,0xA8A808A0
};
/* Anchor pixel indices (highest index bit implicit 0) for each partition shape */
static const unsigned char s_bc7_a2[64] = { /* 2-subset: anchor for subset 1 */
     2, 3, 1, 3, 7, 2, 3, 7,11, 2, 7,11, 3, 8, 4,12,
     4, 1, 8, 1, 2, 4, 8, 1, 2, 4, 1, 2, 3, 4, 1, 2,
     1, 4, 1, 2, 2, 1, 1, 1, 1, 3, 2, 1, 5, 7, 3, 2,
     2, 2, 2, 4, 6, 7, 4, 6, 2, 2, 2, 2, 2, 2, 2, 3
};
static const unsigned char s_bc7_a3a[64] = { /* 3-subset: anchor for subset 1 */
     3, 3,15,15, 8, 3,15,15, 8, 8, 6, 6, 6, 5, 3, 3,
     3, 3, 8,15, 3, 3, 6,10, 5, 8, 8, 6, 8, 5,15,15,
     8,15, 3, 5, 6,10, 8,15,15, 3,15, 5,15,15,15,15,
     3,15, 5, 5, 5, 8, 5,10, 5,10, 8,13,15,12, 3, 3
};
static const unsigned char s_bc7_a3b[64] = { /* 3-subset: anchor for subset 2 */
    15, 8, 8, 3,15,15, 3, 8,15,15,15,15,15,15,15, 8,
    15, 8,15, 3,15, 8,15, 8, 3,15, 6,10,15,15,10, 8,
    15, 3,15,10,10, 8, 9,10, 6,15, 8,15, 3, 6, 6, 8,
    15, 3,15,15,15,15,15,15,15,15,15,15, 3,15,15, 8
};
static const int s_bc7_iw[5][16] = {
    {0},{0},{0,21,43,64},{0,9,18,27,37,46,55,64},
    {0,4,9,13,17,21,26,30,34,38,43,47,51,55,60,64}
};

static void sw_bc7_block(const unsigned char *src, unsigned char *dst, int stride) {
    /* {ns, pb, rb, isb, cb, ab, epb, spb, ib, ib2} */
    static const int MD[8][10] = {
        {3,4,0,0,4,0,1,0,3,0},{2,6,0,0,6,0,0,1,3,0},
        {3,6,0,0,5,0,0,0,2,0},{2,6,0,0,7,0,1,0,2,0},
        {1,0,2,1,5,6,0,0,2,3},{1,0,2,0,7,8,0,0,2,2},
        {1,0,0,0,7,7,1,0,4,0},{2,6,0,0,5,5,1,0,2,0}
    };
    int bp = 0;
    int mode = -1;
    for (int m=0; m<=7; m++) if (sw_bc_bits(src,&bp,1)) { mode=m; break; }
    if (mode < 0) {
        for (int i=0;i<4;i++) for (int j=0;j<4;j++) {
            unsigned char *p=dst+i*stride+j*4; p[0]=p[1]=p[2]=0; p[3]=255;
        }
        return;
    }
    const int *m = MD[mode];
    int ns=m[0], pb=m[1], rb=m[2], isb=m[3], cb=m[4], ab=m[5], epb=m[6], spb=m[7], ib=m[8], ib2=m[9];
    int part = pb  ? (int)sw_bc_bits(src,&bp,pb)  : 0;
    int rot  = rb  ? (int)sw_bc_bits(src,&bp,rb)  : 0;
    int isel = isb ? (int)sw_bc_bits(src,&bp,isb) : 0;
    int ne = ns * 2;
    unsigned char ep[6][4];
    for (int i=0;i<ne;i++) { ep[i][0]=ep[i][1]=ep[i][2]=0; ep[i][3]=255; }
    for (int c=0;c<3;c++) for (int e=0;e<ne;e++) ep[e][c]=(unsigned char)sw_bc_bits(src,&bp,cb);
    if (ab) for (int e=0;e<ne;e++) ep[e][3]=(unsigned char)sw_bc_bits(src,&bp,ab);
    if (epb) {
        for (int e=0;e<ne;e++) {
            int p=(int)sw_bc_bits(src,&bp,1);
            for (int c=0;c<(ab?4:3);c++) ep[e][c]=(unsigned char)((ep[e][c]<<1)|p);
        }
    } else if (spb) {
        for (int s=0;s<ns;s++) {
            int p=(int)sw_bc_bits(src,&bp,1);
            for (int e=s*2;e<s*2+2;e++) for (int c=0;c<3;c++) ep[e][c]=(unsigned char)((ep[e][c]<<1)|p);
        }
    }
    int cbt = cb + (epb||spb ? 1 : 0);
    int abt = ab + (epb ? 1 : 0);
    for (int e=0;e<ne;e++) {
        for (int c=0;c<3;c++) ep[e][c]=sw_expand8(ep[e][c],cbt);
        if (ab) ep[e][3]=sw_expand8(ep[e][3],abt); /* else stays 255 */
    }
    /* Determine anchor pixels (one per subset, MSB of index implicit 0) */
    int anc[3] = {0, 16, 16};
    if (ns >= 2) anc[1] = (ns==2) ? s_bc7_a2[part] : s_bc7_a3a[part];
    if (ns == 3) anc[2] = s_bc7_a3b[part];
    /* Read primary indices */
    unsigned char idx[16], idx2[16];
    for (int pix=0;pix<16;pix++) {
        int is_anc=(pix==anc[0]||pix==anc[1]||pix==anc[2]);
        idx[pix]=(unsigned char)sw_bc_bits(src,&bp,ib-(is_anc?1:0));
    }
    /* Read secondary indices (modes 4 and 5 only) */
    for (int pix=0;pix<16;pix++) {
        idx2[pix]=0;
        if (ib2) idx2[pix]=(unsigned char)sw_bc_bits(src,&bp,ib2-(pix==0?1:0));
    }
    /* Interpolate pixels */
    for (int row=0;row<4;row++) for (int col=0;col<4;col++) {
        int pix = row*4+col;
        int sub = 0;
        if (ns==2) sub=(s_bc7_p2[part]>>pix)&1;
        else if (ns==3) sub=(s_bc7_p3[part]>>(pix*2))&3;
        int e0=sub*2, e1=sub*2+1;
        int ci, ai, cib, aib;
        if (ib2==0)          { ci=idx[pix]; cib=ib;  ai=idx[pix];  aib=ib;  }
        else if (mode==4&&isel){ ci=idx2[pix];cib=ib2; ai=idx[pix]; aib=ib; }
        else                  { ci=idx[pix]; cib=ib;  ai=idx2[pix]; aib=ib2; }
        unsigned char out[4];
        for (int c=0;c<3;c++)
            out[c]=(unsigned char)(((64-s_bc7_iw[cib][ci])*ep[e0][c]+s_bc7_iw[cib][ci]*ep[e1][c]+32)>>6);
        out[3]=(unsigned char)(((64-s_bc7_iw[aib][ai])*ep[e0][3]+s_bc7_iw[aib][ai]*ep[e1][3]+32)>>6);
        if      (rot==1){unsigned char t=out[0];out[0]=out[3];out[3]=t;}
        else if (rot==2){unsigned char t=out[1];out[1]=out[3];out[3]=t;}
        else if (rot==3){unsigned char t=out[2];out[2]=out[3];out[3]=t;}
        unsigned char *p=dst+row*stride+col*4;
        p[0]=out[0]; p[1]=out[1]; p[2]=out[2]; p[3]=out[3];
    }
}

static void sw_decompress_bc1(const unsigned char *src, unsigned char *dst, int w, int h) {
    int bx=(w+3)/4, by=(h+3)/4;
    for (int y=0;y<by;y++) for (int x=0;x<bx;x++) {
        unsigned char tmp[64]; sw_bc1_block(src,tmp,16);
        int px=x*4, py=y*4;
        for (int r=0;r<4&&(py+r)<h;r++) {
            int cols=(px+4<=w)?4:(w-px);
            memcpy(dst+((py+r)*w+px)*4, tmp+r*16, cols*4);
        }
        src+=8;
    }
}
static void sw_decompress_bc3(const unsigned char *src, unsigned char *dst, int w, int h) {
    int bx=(w+3)/4, by=(h+3)/4;
    for (int y=0;y<by;y++) for (int x=0;x<bx;x++) {
        unsigned char tmp[64]; sw_bc3_block(src,tmp,16);
        int px=x*4, py=y*4;
        for (int r=0;r<4&&(py+r)<h;r++) {
            int cols=(px+4<=w)?4:(w-px);
            memcpy(dst+((py+r)*w+px)*4, tmp+r*16, cols*4);
        }
        src+=16;
    }
}
static void sw_decompress_bc7(const unsigned char *src, unsigned char *dst, int w, int h) {
    int bx=(w+3)/4, by=(h+3)/4;
    for (int y=0;y<by;y++) for (int x=0;x<bx;x++) {
        unsigned char tmp[64]; sw_bc7_block(src,tmp,16);
        int px=x*4, py=y*4;
        for (int r=0;r<4&&(py+r)<h;r++) {
            int cols=(px+4<=w)?4:(w-px);
            memcpy(dst+((py+r)*w+px)*4, tmp+r*16, cols*4);
        }
        src+=16;
    }
}

static int g_stat_lookups = 0;
static int g_stat_hits = 0;
static int g_stat_loaded = 0;
static int g_stat_cache_hits = 0;
static int g_stat_neg_hits = 0;

/* --- Texture pack file lookup table --- */
#define TEXPACK_MAP_BITS  15
#define TEXPACK_MAP_SIZE  (1 << TEXPACK_MAP_BITS)  /* 32768 */
#define TEXPACK_MAP_MASK  (TEXPACK_MAP_SIZE - 1)

typedef struct {
    xxh_u64 data_hash;
    xxh_u64 tlut_hash;
    xxh_u32 gc_fmt;
    xxh_u32 orig_w, orig_h;
    char    filepath[260];
    int     occupied;
} TexPackEntry;

static TexPackEntry* g_texpack_map = NULL;
static int g_texpack_count = 0;
static int g_texpack_active = 0;

/* Wildcard TLUT entries: tex1_WxH_DATAHASH_$_FMT.dds (matches any palette) */
#define TEXPACK_WC_BITS  14
#define TEXPACK_WC_SIZE  (1 << TEXPACK_WC_BITS)
#define TEXPACK_WC_MASK  (TEXPACK_WC_SIZE - 1)

typedef struct {
    xxh_u64 data_hash;
    xxh_u32 gc_fmt;
    xxh_u32 orig_w, orig_h;
    char    filepath[260];
    int     occupied;
} TexPackWildcardEntry;

static TexPackWildcardEntry* g_texpack_wc_map = NULL;

/* --- Hash map operations --- */

static xxh_u32 texpack_slot(xxh_u64 data_hash, xxh_u64 tlut_hash, xxh_u32 fmt,
                            xxh_u32 w, xxh_u32 h) {
    xxh_u64 combined = data_hash ^ (tlut_hash * 0x9E3779B97F4A7C15ULL);
    combined ^= ((xxh_u64)fmt * 0x517CC1B727220A95ULL);
    combined ^= ((xxh_u64)w * 0x6C62272E07BB0142ULL);
    combined ^= ((xxh_u64)h * 0x165667B19E3779F9ULL);
    return (xxh_u32)(combined & TEXPACK_MAP_MASK);
}

static xxh_u32 texpack_wc_slot(xxh_u64 data_hash, xxh_u32 fmt, xxh_u32 w, xxh_u32 h) {
    xxh_u64 combined = data_hash;
    combined ^= ((xxh_u64)fmt * 0x517CC1B727220A95ULL);
    combined ^= ((xxh_u64)w * 0x6C62272E07BB0142ULL);
    combined ^= ((xxh_u64)h * 0x165667B19E3779F9ULL);
    return (xxh_u32)(combined & TEXPACK_WC_MASK);
}

static void texpack_insert(xxh_u64 data_hash, xxh_u64 tlut_hash, xxh_u32 fmt,
                           xxh_u32 w, xxh_u32 h, const char* filepath) {
    if (!g_texpack_map) return;

    xxh_u32 slot = texpack_slot(data_hash, tlut_hash, fmt, w, h);
    for (int i = 0; i < TEXPACK_MAP_SIZE; i++) {
        xxh_u32 idx = (slot + i) & TEXPACK_MAP_MASK;
        if (!g_texpack_map[idx].occupied) {
            g_texpack_map[idx].data_hash = data_hash;
            g_texpack_map[idx].tlut_hash = tlut_hash;
            g_texpack_map[idx].gc_fmt = fmt;
            g_texpack_map[idx].orig_w = w;
            g_texpack_map[idx].orig_h = h;
            strncpy(g_texpack_map[idx].filepath, filepath, 259);
            g_texpack_map[idx].filepath[259] = '\0';
            g_texpack_map[idx].occupied = 1;
            g_texpack_count++;
            break;
        }
    }

}

static TexPackEntry* texpack_find(xxh_u64 data_hash, xxh_u64 tlut_hash, xxh_u32 fmt,
                                  xxh_u32 w, xxh_u32 h) {
    if (!g_texpack_map || g_texpack_count == 0) return NULL;
    xxh_u32 slot = texpack_slot(data_hash, tlut_hash, fmt, w, h);
    for (int i = 0; i < TEXPACK_MAP_SIZE; i++) {
        xxh_u32 idx = (slot + i) & TEXPACK_MAP_MASK;
        if (!g_texpack_map[idx].occupied) return NULL;
        if (g_texpack_map[idx].data_hash == data_hash &&
            g_texpack_map[idx].tlut_hash == tlut_hash &&
            g_texpack_map[idx].gc_fmt == fmt &&
            g_texpack_map[idx].orig_w == w &&
            g_texpack_map[idx].orig_h == h) {
            return &g_texpack_map[idx];
        }
    }
    return NULL;
}

static void texpack_insert_wildcard(xxh_u64 data_hash, xxh_u32 fmt,
                                    xxh_u32 w, xxh_u32 h, const char* filepath) {
    if (!g_texpack_wc_map) return;
    xxh_u32 slot = texpack_wc_slot(data_hash, fmt, w, h);
    for (int i = 0; i < TEXPACK_WC_SIZE; i++) {
        xxh_u32 idx = (slot + i) & TEXPACK_WC_MASK;
        if (!g_texpack_wc_map[idx].occupied) {
            g_texpack_wc_map[idx].data_hash = data_hash;
            g_texpack_wc_map[idx].gc_fmt = fmt;
            g_texpack_wc_map[idx].orig_w = w;
            g_texpack_wc_map[idx].orig_h = h;
            strncpy(g_texpack_wc_map[idx].filepath, filepath, 259);
            g_texpack_wc_map[idx].filepath[259] = '\0';
            g_texpack_wc_map[idx].occupied = 1;
            g_texpack_count++;
            break;
        }
    }
}

static TexPackWildcardEntry* texpack_find_wildcard(xxh_u64 data_hash, xxh_u32 fmt,
                                                   xxh_u32 w, xxh_u32 h) {
    if (!g_texpack_wc_map || g_texpack_count == 0) return NULL;
    xxh_u32 slot = texpack_wc_slot(data_hash, fmt, w, h);
    for (int i = 0; i < TEXPACK_WC_SIZE; i++) {
        xxh_u32 idx = (slot + i) & TEXPACK_WC_MASK;
        if (!g_texpack_wc_map[idx].occupied) return NULL;
        if (g_texpack_wc_map[idx].data_hash == data_hash &&
            g_texpack_wc_map[idx].gc_fmt == fmt &&
            g_texpack_wc_map[idx].orig_w == w &&
            g_texpack_wc_map[idx].orig_h == h) {
            return &g_texpack_wc_map[idx];
        }
    }
    return NULL;
}

/* --- Filename parser --- */

static int parse_hex64(const char* s, xxh_u64* out) {
    *out = 0;
    for (int i = 0; i < 16; i++) {
        char c = s[i];
        xxh_u64 nibble;
        if (c >= '0' && c <= '9')      nibble = c - '0';
        else if (c >= 'a' && c <= 'f')  nibble = 10 + c - 'a';
        else if (c >= 'A' && c <= 'F')  nibble = 10 + c - 'A';
        else return 0;
        *out = (*out << 4) | nibble;
    }
    return 1;
}

static int parse_texpack_filename(const char* name, xxh_u32* w, xxh_u32* h,
                                  xxh_u64* data_hash, xxh_u64* tlut_hash, int* tlut_wildcard,
                                  xxh_u32* fmt) {
    if (strncmp(name, "tex1_", 5) != 0) return 0;
    const char* p = name + 5;

    char* end_w;
    unsigned long pw = strtoul(p, &end_w, 10);
    if (*end_w != 'x') return 0;
    char* end_h;
    unsigned long ph = strtoul(end_w + 1, &end_h, 10);
    if (*end_h != '_') return 0;
    *w = (xxh_u32)pw;
    *h = (xxh_u32)ph;
    p = end_h + 1;

    const char* parts[4];
    int part_count = 0;
    parts[part_count++] = p;
    while (*p && part_count < 4) {
        if (*p == '_') {
            parts[part_count++] = p + 1;
        }
        if (*p == '.') break;
        p++;
    }

    if (part_count == 2) {
        if (!parse_hex64(parts[0], data_hash)) return 0;
        *tlut_hash = 0;
        *tlut_wildcard = 0;
        *fmt = (xxh_u32)strtoul(parts[1], NULL, 10);
    } else if (part_count == 3) {
        if (!parse_hex64(parts[0], data_hash)) return 0;
        if (parts[1][0] == '$') {
            *tlut_hash = 0;
            *tlut_wildcard = 1;
        } else {
            if (!parse_hex64(parts[1], tlut_hash)) return 0;
            *tlut_wildcard = 0;
        }
        *fmt = (xxh_u32)strtoul(parts[2], NULL, 10);
    } else {
        return 0;
    }

    return 1;
}

/* --- Directory scanner --- */

#ifdef _WIN32
#include <windows.h>
#undef near
#undef far

static void scan_directory(const char* dir_path) {
    char search_path[300];
    snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search_path, &fd);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        if (fd.cFileName[0] == '.') continue;

        char full_path[300];
        snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scan_directory(full_path);
        } else {
            int len = (int)strlen(fd.cFileName);
            if (len > 4 && strcmp(fd.cFileName + len - 4, ".dds") == 0) {
                xxh_u32 w, h_val, fmt;
                xxh_u64 data_hash, tlut_hash;
                int tlut_wildcard = 0;
                if (parse_texpack_filename(fd.cFileName, &w, &h_val, &data_hash, &tlut_hash,
                                           &tlut_wildcard, &fmt)) {
                    if (tlut_wildcard) {
                        texpack_insert_wildcard(data_hash, fmt, w, h_val, full_path);
                    } else {
                        texpack_insert(data_hash, tlut_hash, fmt, w, h_val, full_path);
                    }
                }
            }
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
}

#else
#include <dirent.h>
#include <sys/stat.h>

static void scan_directory(const char* dir_path) {
    DIR* d = opendir(dir_path);
    if (!d) return;

    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char full_path[300];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_directory(full_path);
        } else {
            int len = (int)strlen(ent->d_name);
            if (len > 4 && strcmp(ent->d_name + len - 4, ".dds") == 0) {
                xxh_u32 w, h_val, fmt;
                xxh_u64 data_hash, tlut_hash;
                int tlut_wildcard = 0;
                if (parse_texpack_filename(ent->d_name, &w, &h_val, &data_hash, &tlut_hash,
                                           &tlut_wildcard, &fmt)) {
                    if (tlut_wildcard) {
                        texpack_insert_wildcard(data_hash, fmt, w, h_val, full_path);
                    } else {
                        texpack_insert(data_hash, tlut_hash, fmt, w, h_val, full_path);
                    }
                }
            }
        }
    }
    closedir(d);
}
#endif

/* --- DDS file loader --- */

static GLuint load_dds_file(const char* filepath, int* out_w, int* out_h) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return 0;

    unsigned char header[148];
    if (fread(header, 1, 128, f) != 128) { fclose(f); return 0; }

    xxh_u32 magic;
    memcpy(&magic, header, 4);
    if (magic != DDS_MAGIC) { fclose(f); return 0; }

    xxh_u32 dds_height, dds_width;
    memcpy(&dds_height, header + 12, 4);
    memcpy(&dds_width, header + 16, 4);

    xxh_u32 pf_flags, pf_fourcc;
    memcpy(&pf_flags, header + 80, 4);
    memcpy(&pf_fourcc, header + 84, 4);

    xxh_u32 dxgi_format = 0;
    GLenum gl_internal = 0;
    int compressed = 0;
    int block_size = 0;
    int sw_decomp = 0; /* 1=BC1, 3=BC3, 7=BC7 software decompression needed */

    if ((pf_flags & DDPF_FOURCC) && pf_fourcc == 0x30315844) {
        /* "DX10" FourCC — read extended header */
        if (fread(header + 128, 1, 20, f) != 20) { fclose(f); return 0; }
        memcpy(&dxgi_format, header + 128, 4);

        switch (dxgi_format) {
            case DXGI_FORMAT_BC7_UNORM:
                block_size = 16; compressed = 1;
                if (g_has_bc7) gl_internal = GL_COMPRESSED_RGBA_BPTC_UNORM;
                else { sw_decomp = 7; gl_internal = GL_RGBA; }
                break;
            case DXGI_FORMAT_BC1_UNORM:
                block_size = 8; compressed = 1;
                if (g_has_s3tc) gl_internal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                else { sw_decomp = 1; gl_internal = GL_RGBA; }
                break;
            case DXGI_FORMAT_BC3_UNORM:
                block_size = 16; compressed = 1;
                if (g_has_s3tc) gl_internal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                else { sw_decomp = 3; gl_internal = GL_RGBA; }
                break;
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
                gl_internal = GL_RGBA;
                compressed = 0;
                break;
            default:
                fclose(f);
                return 0;
        }
    } else if ((pf_flags & DDPF_FOURCC)) {
        /* Legacy FourCC (DXT1, DXT5) */
        if (pf_fourcc == 0x31545844) { /* "DXT1" */
            block_size = 8; compressed = 1;
            if (g_has_s3tc) gl_internal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            else { sw_decomp = 1; gl_internal = GL_RGBA; }
        } else if (pf_fourcc == 0x35545844) { /* "DXT5" */
            block_size = 16; compressed = 1;
            if (g_has_s3tc) gl_internal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            else { sw_decomp = 3; gl_internal = GL_RGBA; }
        } else {
            fclose(f);
            return 0;
        }
    } else {
        /* Uncompressed — check for 32-bit RGBA by bit counts */
        xxh_u32 rgb_bit_count;
        memcpy(&rgb_bit_count, header + 88, 4);
        if (rgb_bit_count == 32) {
            gl_internal = GL_RGBA;
            compressed = 0;
        } else {
            fclose(f);
            return 0;
        }
    }

    int data_size;
    if (compressed) {
        int blocks_x = ((int)dds_width + 3) / 4;
        int blocks_y = ((int)dds_height + 3) / 4;
        data_size = blocks_x * blocks_y * block_size;
    } else {
        data_size = (int)(dds_width * dds_height * 4);
    }

    unsigned char* pixels = (unsigned char*)malloc(data_size);
    if (!pixels) { fclose(f); return 0; }
    if ((int)fread(pixels, 1, data_size, f) != data_size) {
        free(pixels);
        fclose(f);
        return 0;
    }
    fclose(f);

    /* BGRA→RGBA swap if needed */
    if (!compressed && dxgi_format == DXGI_FORMAT_B8G8R8A8_UNORM) {
        for (int i = 0; i < data_size; i += 4) {
            unsigned char tmp = pixels[i];
            pixels[i] = pixels[i + 2];
            pixels[i + 2] = tmp;
        }
    }

    /* Software decompression fallback for GPUs without BC7/S3TC extensions */
    if (sw_decomp) {
        int rgba_size = (int)(dds_width * dds_height * 4);
        unsigned char *rgba = (unsigned char*)malloc(rgba_size);
        if (!rgba) { free(pixels); return 0; }
        if      (sw_decomp == 1) sw_decompress_bc1(pixels, rgba, (int)dds_width, (int)dds_height);
        else if (sw_decomp == 3) sw_decompress_bc3(pixels, rgba, (int)dds_width, (int)dds_height);
        else                     sw_decompress_bc7(pixels, rgba, (int)dds_width, (int)dds_height);
        free(pixels);
        pixels = rgba;
        data_size = rgba_size;
        compressed = 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    if (compressed) {
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, gl_internal,
                               (GLsizei)dds_width, (GLsizei)dds_height,
                               0, data_size, pixels);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)dds_width, (GLsizei)dds_height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        glDeleteTextures(1, &tex);
        free(pixels);
        return 0;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(pixels);

    if (out_w) *out_w = (int)dds_width;
    if (out_h) *out_h = (int)dds_height;

    return tex;
}

/* --- GC texture block-aligned data size (for hash input) --- */

static int gc_texture_data_size(int w, int h, unsigned int fmt) {
    int bw, bh, block_bytes;
    switch (fmt) {
        case 0:  /* GX_TF_I4 */
        case 8:  /* GX_TF_C4 */
            bw = 8; bh = 8; block_bytes = 32; break;
        case 14: /* GX_TF_CMPR */
            bw = 8; bh = 8; block_bytes = 32; break;
        case 1:  /* GX_TF_I8 */
        case 2:  /* GX_TF_IA4 */
        case 9:  /* GX_TF_C8 */
            bw = 8; bh = 4; block_bytes = 32; break;
        case 3:  /* GX_TF_IA8 */
        case 4:  /* GX_TF_RGB565 */
        case 5:  /* GX_TF_RGB5A3 */
            bw = 4; bh = 4; block_bytes = 32; break;
        case 6:  /* GX_TF_RGBA8 */
            bw = 4; bh = 4; block_bytes = 64; break;
        default:
            bw = 8; bh = 4; block_bytes = 32; break;
    }
    int blocks_x = (w + bw - 1) / bw;
    int blocks_y = (h + bh - 1) / bh;
    return blocks_x * blocks_y * block_bytes;
}

/* --- Loaded texture cache (GL ID, avoids re-reading DDS from disk) --- */
#define LOADED_CACHE_SIZE 32768
#define LOADED_CACHE_MASK (LOADED_CACHE_SIZE - 1)

typedef struct {
    xxh_u64 key;
    GLuint  gl_tex;
    int     tex_w, tex_h;
    int     occupied;
} LoadedCacheEntry;

static LoadedCacheEntry g_loaded_cache[LOADED_CACHE_SIZE];

static xxh_u64 loaded_cache_key(xxh_u64 data_hash, xxh_u64 tlut_hash, xxh_u32 fmt,
                                xxh_u32 w, xxh_u32 h) {
    xxh_u64 k = data_hash;
    k ^= tlut_hash * 0x517CC1B727220A95ULL;
    k ^= (xxh_u64)fmt * 0x6C62272E07BB0142ULL;
    k ^= (xxh_u64)w * 0x165667B19E3779F9ULL;
    k ^= (xxh_u64)h * 0x85EBCA77C2B2AE63ULL;
    return k;
}

static LoadedCacheEntry* loaded_cache_find(xxh_u64 key) {
    xxh_u32 slot = (xxh_u32)(key & LOADED_CACHE_MASK);
    for (int i = 0; i < LOADED_CACHE_SIZE; i++) {
        xxh_u32 idx = (slot + i) & LOADED_CACHE_MASK;
        if (!g_loaded_cache[idx].occupied) return NULL;
        if (g_loaded_cache[idx].key == key) return &g_loaded_cache[idx];
    }
    return NULL;
}

static void loaded_cache_insert(xxh_u64 key, GLuint tex, int w, int h) {
    xxh_u32 slot = (xxh_u32)(key & LOADED_CACHE_MASK);
    for (int i = 0; i < LOADED_CACHE_SIZE; i++) {
        xxh_u32 idx = (slot + i) & LOADED_CACHE_MASK;
        if (!g_loaded_cache[idx].occupied) {
            g_loaded_cache[idx].key = key;
            g_loaded_cache[idx].gl_tex = tex;
            g_loaded_cache[idx].tex_w = w;
            g_loaded_cache[idx].tex_h = h;
            g_loaded_cache[idx].occupied = 1;
            return;
        }
    }
}

/* --- Negative lookup cache (skip re-hashing textures with no pack match) --- */

#define NEG_CACHE_SIZE 2048
#define NEG_CACHE_MASK (NEG_CACHE_SIZE - 1)

static xxh_u64 g_neg_cache[NEG_CACHE_SIZE];
static int     g_neg_cache_valid[NEG_CACHE_SIZE];

static int neg_cache_check(xxh_u64 key) {
    xxh_u32 slot = (xxh_u32)(key & NEG_CACHE_MASK);
    return g_neg_cache_valid[slot] && g_neg_cache[slot] == key;
}

static void neg_cache_insert(xxh_u64 key) {
    xxh_u32 slot = (xxh_u32)(key & NEG_CACHE_MASK);
    g_neg_cache[slot] = key;
    g_neg_cache_valid[slot] = 1;
}

/* --- Public API --- */

static void check_compressed_texture_support(void) {
    GLint num_ext = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
    for (GLint i = 0; i < num_ext; i++) {
        const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (!ext) continue;
        if (strcmp(ext, "GL_ARB_texture_compression_bptc") == 0) g_has_bc7 = 1;
        if (strcmp(ext, "GL_EXT_texture_compression_s3tc") == 0) g_has_s3tc = 1;
    }
}

static void xxhash64_selftest(void) {
    xxh_u64 h0 = xxhash64("", 0);
    unsigned char b32[32];
    for (int i = 0; i < 32; i++) b32[i] = (unsigned char)i;
    xxh_u64 h32 = xxhash64(b32, 32);
    int ok = (h0 == 0xEF46DB3751D8E999ULL) && (h32 == 0xCBF59C5116FF32B4ULL);
    printf("[TexturePack] XXH64 selftest: %s\n", ok ? "PASS" : "FAIL");
}

void pc_texture_pack_init(void) {
    g_texpack_count = 0;
    g_texpack_active = 0;
    g_stat_lookups = g_stat_hits = g_stat_loaded = g_stat_cache_hits = g_stat_neg_hits = 0;
    memset(g_loaded_cache, 0, sizeof(g_loaded_cache));
    memset(g_neg_cache_valid, 0, sizeof(g_neg_cache_valid));

    xxhash64_selftest();

    check_compressed_texture_support();

    g_texpack_map = (TexPackEntry*)calloc(TEXPACK_MAP_SIZE, sizeof(TexPackEntry));
    g_texpack_wc_map = (TexPackWildcardEntry*)calloc(TEXPACK_WC_SIZE, sizeof(TexPackWildcardEntry));
    if (!g_texpack_map || !g_texpack_wc_map) {
        printf("[TexturePack] Failed to allocate lookup table\n");
        if (g_texpack_map) { free(g_texpack_map); g_texpack_map = NULL; }
        if (g_texpack_wc_map) { free(g_texpack_wc_map); g_texpack_wc_map = NULL; }
        return;
    }

    scan_directory("texture_pack");

    if (g_texpack_count > 0) {
        g_texpack_active = 1;
        printf("[TexturePack] Loaded %d texture entries (BC7:%s S3TC:%s)\n",
               g_texpack_count,
               g_has_bc7 ? "gpu" : "sw",
               g_has_s3tc ? "gpu" : "sw");
    } else {
        printf("[TexturePack] No texture pack found in texture_pack/\n");
    }
}

/* --- Texture preload cache file ---
 * Packs all parsed DDS pixel data into one file so subsequent launches
 * only open a single file (avoids antivirus scanning 17K+ individual DDS files).
 *
 * Format:
 *   Header: magic(4) version(4) entry_count(4) texpack_count(4)
 *   Entry[]:  cache_key(8) gl_internal(4) compressed(4) width(4) height(4) data_size(4)
 *   Pixel data follows each entry header immediately.
 */
#define TPC_MAGIC   0x43505431  /* "TPC1" */
#define TPC_VERSION 1
#define TPC_FILE    "texture_pack/texture_cache.bin"

typedef struct {
    xxh_u64 cache_key;
    xxh_u32 gl_internal;
    xxh_u32 compressed;
    xxh_u32 width;
    xxh_u32 height;
    xxh_u32 data_size;
} TPCEntryHeader;

/* Upload a single cached texture entry to GL and insert into loaded_cache */
static int tpc_upload_entry(const TPCEntryHeader* eh, const unsigned char* pixels) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    if (eh->compressed) {
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, (GLenum)eh->gl_internal,
                               (GLsizei)eh->width, (GLsizei)eh->height,
                               0, (GLsizei)eh->data_size, pixels);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     (GLsizei)eh->width, (GLsizei)eh->height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }

    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &tex);
        return 0;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    loaded_cache_insert(eh->cache_key, tex, (int)eh->width, (int)eh->height);
    return 1;
}

/* Try loading from binary cache file. Returns 1 on success, 0 if cache missing/stale. */
static int preload_from_cache(int expected_count) {
    extern SDL_Window* g_pc_window;
    FILE* f = fopen(TPC_FILE, "rb");
    if (!f) return 0;

    xxh_u32 header[4];
    if (fread(header, 4, 4, f) != 4) { fclose(f); return 0; }
    if (header[0] != TPC_MAGIC || header[1] != TPC_VERSION) { fclose(f); return 0; }

    int entry_count = (int)header[2];
    int stored_texpack_count = (int)header[3];

    /* Invalidate if texture pack file count changed */
    if (stored_texpack_count != expected_count) {
        printf("[TexturePack] Cache stale (pack has %d entries, cache has %d) — rebuilding\n",
               expected_count, stored_texpack_count);
        fclose(f);
        return 0;
    }

    int loaded = 0;
    unsigned char* buf = NULL;
    int buf_cap = 0;

    for (int i = 0; i < entry_count; i++) {
        TPCEntryHeader eh;
        if (fread(&eh, sizeof(eh), 1, f) != 1) break;

        if ((int)eh.data_size > buf_cap) {
            buf_cap = (int)eh.data_size + 4096;
            buf = (unsigned char*)realloc(buf, buf_cap);
            if (!buf) break;
        }
        if (fread(buf, 1, eh.data_size, f) != eh.data_size) break;

        if (tpc_upload_entry(&eh, buf)) loaded++;

        if (g_pc_window && (i % 500) == 0) {
            char title[128];
            snprintf(title, sizeof(title), "Animal Crossing - Loading textures... %d/%d (%d%%)",
                     i, entry_count, i * 100 / entry_count);
            SDL_SetWindowTitle(g_pc_window, title);
            SDL_PumpEvents();
        }
    }

    free(buf);
    fclose(f);

    if (g_pc_window) SDL_SetWindowTitle(g_pc_window, "Animal Crossing");

    printf("[TexturePack] Loaded %d textures from cache\n", loaded);
    return 1;
}

/* Load raw DDS file data without creating a GL texture (for cache building).
 * Returns malloc'd pixel data, fills out metadata. Caller must free(). */
static unsigned char* load_dds_raw(const char* filepath, xxh_u32* out_w, xxh_u32* out_h,
                                    xxh_u32* out_gl_internal, xxh_u32* out_compressed,
                                    int* out_data_size) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return NULL;

    unsigned char header[148];
    if (fread(header, 1, 128, f) != 128) { fclose(f); return NULL; }

    xxh_u32 magic;
    memcpy(&magic, header, 4);
    if (magic != DDS_MAGIC) { fclose(f); return NULL; }

    xxh_u32 dds_height, dds_width;
    memcpy(&dds_height, header + 12, 4);
    memcpy(&dds_width, header + 16, 4);

    xxh_u32 pf_flags, pf_fourcc;
    memcpy(&pf_flags, header + 80, 4);
    memcpy(&pf_fourcc, header + 84, 4);

    xxh_u32 dxgi_format = 0;
    GLenum gl_internal = 0;
    int compressed = 0;
    int block_size = 0;
    int sw_decomp = 0;

    if ((pf_flags & DDPF_FOURCC) && pf_fourcc == 0x30315844) {
        if (fread(header + 128, 1, 20, f) != 20) { fclose(f); return NULL; }
        memcpy(&dxgi_format, header + 128, 4);
        switch (dxgi_format) {
            case DXGI_FORMAT_BC7_UNORM:
                block_size=16; compressed=1;
                if (g_has_bc7) gl_internal=GL_COMPRESSED_RGBA_BPTC_UNORM;
                else { sw_decomp=7; gl_internal=GL_RGBA; } break;
            case DXGI_FORMAT_BC1_UNORM:
                block_size=8; compressed=1;
                if (g_has_s3tc) gl_internal=GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                else { sw_decomp=1; gl_internal=GL_RGBA; } break;
            case DXGI_FORMAT_BC3_UNORM:
                block_size=16; compressed=1;
                if (g_has_s3tc) gl_internal=GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                else { sw_decomp=3; gl_internal=GL_RGBA; } break;
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
                gl_internal=GL_RGBA; compressed=0; break;
            default: fclose(f); return NULL;
        }
    } else if ((pf_flags & DDPF_FOURCC)) {
        if (pf_fourcc == 0x31545844) {
            block_size=8; compressed=1;
            if (g_has_s3tc) gl_internal=GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            else { sw_decomp=1; gl_internal=GL_RGBA; }
        } else if (pf_fourcc == 0x35545844) {
            block_size=16; compressed=1;
            if (g_has_s3tc) gl_internal=GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            else { sw_decomp=3; gl_internal=GL_RGBA; }
        } else { fclose(f); return NULL; }
    } else {
        xxh_u32 rgb_bit_count;
        memcpy(&rgb_bit_count, header + 88, 4);
        if (rgb_bit_count == 32) { gl_internal = GL_RGBA; compressed = 0; }
        else { fclose(f); return NULL; }
    }

    int data_size;
    if (compressed) {
        int blocks_x = ((int)dds_width + 3) / 4;
        int blocks_y = ((int)dds_height + 3) / 4;
        data_size = blocks_x * blocks_y * block_size;
    } else {
        data_size = (int)(dds_width * dds_height * 4);
    }

    unsigned char* pixels = (unsigned char*)malloc(data_size);
    if (!pixels) { fclose(f); return NULL; }
    if ((int)fread(pixels, 1, data_size, f) != data_size) { free(pixels); fclose(f); return NULL; }
    fclose(f);

    if (!compressed && dxgi_format == DXGI_FORMAT_B8G8R8A8_UNORM) {
        for (int i = 0; i < data_size; i += 4) {
            unsigned char tmp = pixels[i]; pixels[i] = pixels[i + 2]; pixels[i + 2] = tmp;
        }
    }

    if (sw_decomp) {
        int rgba_size = (int)(dds_width * dds_height * 4);
        unsigned char *rgba = (unsigned char*)malloc(rgba_size);
        if (!rgba) { free(pixels); return NULL; }
        if      (sw_decomp == 1) sw_decompress_bc1(pixels, rgba, (int)dds_width, (int)dds_height);
        else if (sw_decomp == 3) sw_decompress_bc3(pixels, rgba, (int)dds_width, (int)dds_height);
        else                     sw_decompress_bc7(pixels, rgba, (int)dds_width, (int)dds_height);
        free(pixels);
        pixels = rgba;
        data_size = rgba_size;
        compressed = 0;
        gl_internal = GL_RGBA;
    }

    *out_w = dds_width;
    *out_h = dds_height;
    *out_gl_internal = (xxh_u32)gl_internal;
    *out_compressed = (xxh_u32)compressed;
    *out_data_size = data_size;
    return pixels;
}

/* Preload entry info for cache building */
typedef struct {
    xxh_u64 cache_key;
    char filepath[260];
} PreloadEntry;

void pc_texture_pack_preload_all(void) {
    if (!g_texpack_active) return;

    extern SDL_Window* g_pc_window;
    Uint64 t_start = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    int use_cache = (g_pc_settings.preload_textures >= 2);

    /* Try binary cache first (mode 2 only) */
    if (use_cache && preload_from_cache(g_texpack_count)) {
        Uint64 t_end = SDL_GetPerformanceCounter();
        double elapsed = (double)(t_end - t_start) * 1000.0 / (double)freq;
        printf("[TexturePack] Preload from cache took %.0fms\n", elapsed);
        return;
    }

    /* Load all DDS files individually */
    int total = 0, processed = 0, loaded = 0, failed = 0;
    for (int i = 0; i < TEXPACK_MAP_SIZE; i++)
        if (g_texpack_map[i].occupied) total++;
    for (int i = 0; i < TEXPACK_WC_SIZE; i++)
        if (g_texpack_wc_map[i].occupied) total++;

    /* Collect entries and load DDS + upload to GL + write cache simultaneously */
    FILE* cache_f = NULL;
    if (use_cache) {
        cache_f = fopen(TPC_FILE, "wb");
        if (cache_f) {
            xxh_u32 header[4] = { TPC_MAGIC, TPC_VERSION, 0, (xxh_u32)g_texpack_count };
            fwrite(header, 4, 4, cache_f); /* entry_count placeholder, patched later */
        }
    }

    int cache_entries = 0;

    /* Exact-match entries */
    for (int i = 0; i < TEXPACK_MAP_SIZE; i++) {
        if (!g_texpack_map[i].occupied) continue;
        processed++;

        xxh_u64 key = loaded_cache_key(g_texpack_map[i].data_hash,
                                        g_texpack_map[i].tlut_hash,
                                        g_texpack_map[i].gc_fmt,
                                        g_texpack_map[i].orig_w,
                                        g_texpack_map[i].orig_h);
        if (loaded_cache_find(key)) continue;

        xxh_u32 dds_w, dds_h, gl_int, comp;
        int data_size;
        unsigned char* pixels = load_dds_raw(g_texpack_map[i].filepath,
                                              &dds_w, &dds_h, &gl_int, &comp, &data_size);
        if (pixels) {
            TPCEntryHeader eh = { key, gl_int, comp, dds_w, dds_h, (xxh_u32)data_size };

            /* Upload to GL */
            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            if (comp)
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, (GLenum)gl_int,
                                       (GLsizei)dds_w, (GLsizei)dds_h, 0, data_size, pixels);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                             (GLsizei)dds_w, (GLsizei)dds_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

            if (glGetError() == GL_NO_ERROR) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                loaded_cache_insert(key, tex, (int)dds_w, (int)dds_h);
                loaded++;

                if (cache_f) {
                    fwrite(&eh, sizeof(eh), 1, cache_f);
                    fwrite(pixels, 1, data_size, cache_f);
                    cache_entries++;
                }
            } else {
                glDeleteTextures(1, &tex);
                failed++;
            }
            free(pixels);
        } else {
            failed++;
        }

        if (g_pc_window && (processed % 100) == 0) {
            char title[128];
            snprintf(title, sizeof(title), "Animal Crossing - Building texture cache... %d/%d (%d%%)",
                     processed, total, processed * 100 / total);
            SDL_SetWindowTitle(g_pc_window, title);
            SDL_PumpEvents();
        }
    }

    /* Wildcard entries */
    for (int i = 0; i < TEXPACK_WC_SIZE; i++) {
        if (!g_texpack_wc_map[i].occupied) continue;
        processed++;

        xxh_u64 key = loaded_cache_key(g_texpack_wc_map[i].data_hash, 0,
                                        g_texpack_wc_map[i].gc_fmt,
                                        g_texpack_wc_map[i].orig_w,
                                        g_texpack_wc_map[i].orig_h);
        if (loaded_cache_find(key)) continue;

        xxh_u32 dds_w, dds_h, gl_int, comp;
        int data_size;
        unsigned char* pixels = load_dds_raw(g_texpack_wc_map[i].filepath,
                                              &dds_w, &dds_h, &gl_int, &comp, &data_size);
        if (pixels) {
            TPCEntryHeader eh = { key, gl_int, comp, dds_w, dds_h, (xxh_u32)data_size };

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            if (comp)
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, (GLenum)gl_int,
                                       (GLsizei)dds_w, (GLsizei)dds_h, 0, data_size, pixels);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                             (GLsizei)dds_w, (GLsizei)dds_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

            if (glGetError() == GL_NO_ERROR) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                loaded_cache_insert(key, tex, (int)dds_w, (int)dds_h);
                loaded++;

                if (cache_f) {
                    fwrite(&eh, sizeof(eh), 1, cache_f);
                    fwrite(pixels, 1, data_size, cache_f);
                    cache_entries++;
                }
            } else {
                glDeleteTextures(1, &tex);
                failed++;
            }
            free(pixels);
        } else {
            failed++;
        }

        if (g_pc_window && (processed % 100) == 0) {
            char title[128];
            snprintf(title, sizeof(title), "Animal Crossing - Building texture cache... %d/%d (%d%%)",
                     processed, total, processed * 100 / total);
            SDL_SetWindowTitle(g_pc_window, title);
            SDL_PumpEvents();
        }
    }

    /* Patch entry count in cache header and close */
    if (cache_f) {
        fseek(cache_f, 8, SEEK_SET);
        xxh_u32 count32 = (xxh_u32)cache_entries;
        fwrite(&count32, 4, 1, cache_f);
        fclose(cache_f);
        printf("[TexturePack] Wrote cache: %d textures to %s\n", cache_entries, TPC_FILE);
    }

    if (g_pc_window) SDL_SetWindowTitle(g_pc_window, "Animal Crossing");

    Uint64 t_end = SDL_GetPerformanceCounter();
    double elapsed = (double)(t_end - t_start) * 1000.0 / (double)freq;
    printf("[TexturePack] Preloaded %d/%d textures in %.0fms (%d failed)\n",
           loaded, total, elapsed, failed);
}

void pc_texture_pack_shutdown(void) {
    if (g_texpack_active) {
        printf("[TexturePack] Summary: %d/%d unique textures matched (%d loaded from disk, %d cache hits, %d neg-cache skips)\n",
               g_stat_hits, g_stat_lookups, g_stat_loaded, g_stat_cache_hits, g_stat_neg_hits);
    }

    for (int i = 0; i < LOADED_CACHE_SIZE; i++) {
        if (g_loaded_cache[i].occupied && g_loaded_cache[i].gl_tex) {
            glDeleteTextures(1, &g_loaded_cache[i].gl_tex);
        }
    }
    memset(g_loaded_cache, 0, sizeof(g_loaded_cache));
    memset(g_neg_cache_valid, 0, sizeof(g_neg_cache_valid));

    if (g_texpack_map) {
        free(g_texpack_map);
        g_texpack_map = NULL;
    }
    if (g_texpack_wc_map) {
        free(g_texpack_wc_map);
        g_texpack_wc_map = NULL;
    }
    g_texpack_count = 0;
    g_texpack_active = 0;
}

int pc_texture_pack_active(void) {
    return g_texpack_active;
}

GLuint pc_texture_pack_lookup(const void* data, int data_size,
                              int w, int h, unsigned int fmt,
                              const void* tlut_data, int tlut_entries, int tlut_is_be,
                              int* out_w, int* out_h) {
    if (!g_texpack_active || !data || data_size <= 0) return 0;

    int hash_size = gc_texture_data_size(w, h, fmt);
    if (hash_size > data_size) hash_size = data_size;

    xxh_u64 data_hash = xxhash64(data, hash_size);

    /* CI textures: Dolphin hashes only used palette entries (min..max index), in BE byte order */
    xxh_u64 tlut_hash = 0;
    if (tlut_data && tlut_entries > 0) {
        const unsigned char* tex_bytes = (const unsigned char*)data;
        unsigned int pal_min = 0xFFFF, pal_max = 0;

        if (tlut_entries <= 16) {
            /* CI4: each byte = 2 pixels, 4 bits each */
            for (int i = 0; i < hash_size; i++) {
                unsigned int lo = tex_bytes[i] & 0xF;
                unsigned int hi = tex_bytes[i] >> 4;
                if (lo < pal_min) pal_min = lo;
                if (hi < pal_min) pal_min = hi;
                if (lo > pal_max) pal_max = lo;
                if (hi > pal_max) pal_max = hi;
            }
        } else if (tlut_entries <= 256) {
            /* CI8: each byte = 1 pixel index */
            for (int i = 0; i < hash_size; i++) {
                unsigned int idx = tex_bytes[i];
                if (idx < pal_min) pal_min = idx;
                if (idx > pal_max) pal_max = idx;
            }
        } else {
            /* CI14x2: each u16 & 0x3FFF = index (big-endian) */
            for (int i = 0; i + 1 < hash_size; i += 2) {
                unsigned int idx = ((unsigned int)tex_bytes[i] << 8 | tex_bytes[i+1]) & 0x3FFF;
                if (idx < pal_min) pal_min = idx;
                if (idx > pal_max) pal_max = idx;
            }
        }

        if (pal_min > pal_max) { pal_min = 0; pal_max = 0; }

        int used_entries = (int)(pal_max + 1 - pal_min);
        int tlut_offset = (int)(pal_min * 2);
        int tlut_bytes = used_entries * 2;

        if (tlut_offset + tlut_bytes > tlut_entries * 2)
            tlut_bytes = tlut_entries * 2 - tlut_offset;
        if (tlut_bytes <= 0) tlut_bytes = tlut_entries * 2;

        const unsigned char* tlut_src = (const unsigned char*)tlut_data + tlut_offset;

        if (tlut_is_be) {
            tlut_hash = xxhash64(tlut_src, tlut_bytes);
        } else {
            /* Swap u16s to BE for Dolphin compatibility */
            unsigned char* tmp = (unsigned char*)malloc(tlut_bytes);
            if (!tmp) return 0;
            for (int i = 0; i < tlut_bytes; i += 2) {
                tmp[i] = tlut_src[i + 1];
                tmp[i + 1] = tlut_src[i];
            }
            tlut_hash = xxhash64(tmp, tlut_bytes);
            free(tmp);
        }
    }

    xxh_u64 cache_key = loaded_cache_key(data_hash, tlut_hash, (xxh_u32)fmt, (xxh_u32)w, (xxh_u32)h);
    if (neg_cache_check(cache_key)) { g_stat_neg_hits++; return 0; }

    LoadedCacheEntry* loaded = loaded_cache_find(cache_key);
    if (loaded) {
        g_stat_cache_hits++;
        if (out_w) *out_w = loaded->tex_w;
        if (out_h) *out_h = loaded->tex_h;
        return loaded->gl_tex;
    }

    g_stat_lookups++;

    TexPackEntry* entry = texpack_find(data_hash, tlut_hash, (xxh_u32)fmt, (xxh_u32)w, (xxh_u32)h);
    TexPackWildcardEntry* wc_entry = NULL;
    if (!entry && tlut_hash != 0) {
        wc_entry = texpack_find_wildcard(data_hash, (xxh_u32)fmt, (xxh_u32)w, (xxh_u32)h);
    }
    if (!entry && !wc_entry) {
        neg_cache_insert(cache_key);
        return 0;
    }
    g_stat_hits++;

    int dds_w = 0, dds_h = 0;
    const char* path = entry ? entry->filepath : wc_entry->filepath;
    GLuint tex = load_dds_file(path, &dds_w, &dds_h);
    if (!tex) {
        printf("[TexturePack] Failed to load DDS: %s\n", path);
        neg_cache_insert(cache_key);
        return 0;
    }

    g_stat_loaded++;
    printf("[TexturePack] Loaded HD %dx%d (was %dx%d): %s\n",
           dds_w, dds_h, w, h, path);

    loaded_cache_insert(cache_key, tex, dds_w, dds_h);

    if (out_w) *out_w = dds_w;
    if (out_h) *out_h = dds_h;

    return tex;
}
