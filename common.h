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
