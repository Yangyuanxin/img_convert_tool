#ifndef __IMG_DEC_H
#define __IMG_DEC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "img_common.h"

typedef enum {
    SEEK_PREV = 1,
    SEEK_NEXT = 2,
    SEEK_HEAD = 3,
    SEEK_TAIL = 4,
    SEEK_GOTO = 5,
    SEEK_INVALID,
} img_seek_e;

typedef struct
{
    fmt_e format;
    // bitmap_mode_e bitmap_mode;
    int32_t height;
    int32_t width;
    int32_t file_offset;
    int32_t img_head_size;
    int32_t img_tail_size;
} img_param;

typedef void img_ctx;

img_ctx *img_open(char *path);
img_err_code img_close(img_ctx *img);
img_err_code img_cfg(img_ctx *img, img_param param);
int32_t img_get_size(img_ctx *img);
int32_t img_get_num(img_ctx *img);
img_err_code img_seek(img_ctx *img, img_seek_e seek, int32_t to);
int32_t img_tell(img_ctx *img);
img_err_code img_dec(img_ctx *img, void *data, int32_t *len);

#endif
