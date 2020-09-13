#pragma once

#ifdef USE_RAMFS
#define RAMFS_DIR "resin:/res"
#else
#define RAMFS_DIR "../resin/res"
#endif

#ifdef __WIIU__
#define NTRVIEW_DIR "fs:/vol/external01/wiiu/apps/ntrview"
#else
#define NTRVIEW_DIR "."
#endif

#define IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

static inline uint32_t NativeToLE(uint32_t in) {
#ifdef IS_BIG_ENDIAN
    return __builtin_bswap32(in);
#else
    return in;
#endif
}
static inline uint32_t LEToNative(uint32_t in) {
#ifdef IS_BIG_ENDIAN
    return __builtin_bswap32(in);
#else
    return in;
#endif
}
