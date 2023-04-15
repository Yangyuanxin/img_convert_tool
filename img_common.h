#ifndef __IMG_COMMON_H
#define __IMG_COMMON_H

#include <stdint.h>

typedef enum {
    IMG_OK,
    IMG_PARAM_NULL_PTR,
    IMG_PARAM_OVERFLOW,
    IMG_PARAM_INVALID,
    IMG_OPEN_FILE_ERR,
    IMG_FILE_HEAD,
    IMG_FILE_TAIL,
    IMG_SEEK_ERR,
    IMG_MEM_WRONG,
    IMG_FORMAT_ERR,
    IMG_FORMAT_UNKNOWN,
    IMG_FORMAT_NOT_SUPPORT,
    IMG_OTHER_ERR,
} img_err_code;

// bitmap格式缩写的含义
// r=row,c=column,l=lsb,m=msb
typedef enum {
    FMT_BITMAP_RL  = 1,
    FMT_BITMAP_RM  = 2,
    FMT_BITMAP_CL  = 3,
    FMT_BITMAP_CM  = 4,
    FMT_BITMAP_RCL = 5,
    FMT_BITMAP_RCM = 6,
    FMT_BITMAP_CRL = 7,
    FMT_BITMAP_CRM = 8,
    FMT_WEB        = 9,
    FMT_RGB565     = 10,
    FMT_BGR565     = 11,
    FMT_ARGB1555   = 12,
    FMT_BGRA5551   = 13,
    FMT_INVALID,
} fmt_e;

extern const uint32_t web_color[216];

#endif
