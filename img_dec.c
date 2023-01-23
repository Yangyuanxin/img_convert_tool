/**
 * @file img_dec.c
 * @author dma
 * @brief 其他格式图像解码为rgb888
 * @note
 * 
 * @version 0.2
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "img_dec.h"

// 图片解码函数原型，其中h(horizontal)表示横向长度，v(vertical)表示纵向长度
typedef void(*convert)(uint8_t *in, uint8_t *out, int32_t h, int32_t v);

typedef struct
{
    FILE *fp;
    int32_t file_size;
    int32_t sum_img_num;
    int32_t now_img_num;
    int32_t img_size; // 单张图片的大小，包含文件头尾
    uint8_t *buf; // 保存未解码的图片数据
    convert func;
    img_dec_param param;
} _img_dec_ctx;

// 解码函数
static void bitmap_rl_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bitmap_rm_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bitmap_cl_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bitmap_cm_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bitmap_rcl_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bitmap_rcm_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bitmap_crl_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bitmap_crm_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void web_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb565_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bgr565_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void argb1555_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void bgra5551_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v);

// 解码函数列表，必须与 fmt_e 的顺序保持一致
const convert convert_list[] = {
    NULL,
    bitmap_rl_to_rgb888,
    bitmap_rm_to_rgb888,
    bitmap_cl_to_rgb888,
    bitmap_cm_to_rgb888,
    bitmap_rcl_to_rgb888,
    bitmap_rcm_to_rgb888,
    bitmap_crl_to_rgb888,
    bitmap_crm_to_rgb888,
    web_to_rgb888,
    rgb565_to_rgb888,
    bgr565_to_rgb888,
    argb1555_to_rgb888,
    bgra5551_to_rgb888,
};

img_dec_ctx *img_dec_open(char *path)
{
    FILE *img_fp;
    int32_t file_size;
    _img_dec_ctx *ctx;

    img_fp = fopen(path, "rb");
    if (img_fp == NULL)
    {
        printf("can not open %s\n", path);
        return NULL;
    }

    fseek(img_fp, 0, SEEK_END);
    file_size = ftell(img_fp);
    fseek(img_fp, 0, SEEK_SET);
    if (file_size == 0)
    {
        printf("file %s size is zero!\n", path);
        fclose(img_fp);
        return NULL;
    }

    ctx = (_img_dec_ctx *)malloc(sizeof(_img_dec_ctx));
    if (ctx == NULL)
    {
        printf("create img_dec_ctx error\n");
        fclose(img_fp);
        return NULL;
    }
    memset(ctx, 0, sizeof(_img_dec_ctx));

    ctx->fp = img_fp;
    ctx->file_size = file_size;
    ctx->buf = NULL;

    return ctx;
}

img_err_code img_dec_close(img_dec_ctx *img)
{
    if (img == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    _img_dec_ctx *ctx = (_img_dec_ctx *)img;

    if (ctx->buf)
    {
        free(ctx->buf);
    }
    fclose(ctx->fp);
    free(ctx);

    return IMG_OK;
}

img_err_code img_dec_cfg(img_dec_ctx *img, img_dec_param *param)
{
    if (img == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    _img_dec_ctx *ctx = (_img_dec_ctx *)img;

    if (param->format == 0 || param->format >= FMT_INVALID || param->height == 0 || param->width == 0)
    {
        return IMG_PARAM_INVALID;
    }
    ctx->param = *param;

    // 宽度或高度需要向上对8取整，例如15*9像素的图片，横向需要(15 / 8) * 9 = 18字节内存，纵向需要 15 * (9 / 8) = 30 字节内存
    if (param->format == FMT_BITMAP_RL  ||
        param->format == FMT_BITMAP_RM  ||
        param->format == FMT_BITMAP_RCL ||
        param->format == FMT_BITMAP_RCM)
    {
        ctx->img_size = param->height * ((param->width + 7) >> 3) + param->img_head_size + param->img_tail_size;
    }
    else if (param->format == FMT_BITMAP_CL  ||
             param->format == FMT_BITMAP_CM  ||
             param->format == FMT_BITMAP_CRL ||
             param->format == FMT_BITMAP_CRM)
    {
        ctx->img_size = ((param->height + 7) >> 3) * param->width + param->img_head_size + param->img_tail_size;
    }
    else if (param->format == FMT_WEB)
    {
        ctx->img_size = param->height * param->width + param->img_head_size + param->img_tail_size;
    }
    else if (param->format >= FMT_RGB565 && param->format <= FMT_BGRA5551)
    {
        ctx->img_size = param->height * param->width * 2 + param->img_head_size + param->img_tail_size;
    }
    else
    {
        return IMG_PARAM_INVALID;
    }
    ctx->func = convert_list[param->format];
    ctx->sum_img_num = (ctx->file_size - ctx->param.file_offset) / ctx->img_size;
    ctx->now_img_num = 0;
    if (ctx->buf)
    {
        free(ctx->buf);
    }
    ctx->buf = (uint8_t *)malloc(ctx->img_size);
    if (ctx->buf == NULL)
    {
        printf("malloc img buffer error\n");
        return IMG_MEM_WRONG;
    }

    return IMG_OK;
}

int32_t img_dec_get_size(img_dec_ctx *img)
{
    if (img == NULL)
    {
        return -1;
    }
    _img_dec_ctx *ctx = (_img_dec_ctx *)img;
    return ctx->img_size;
}

int32_t img_dec_get_num(img_dec_ctx *img)
{
    if (img == NULL)
    {
        return -1;
    }
    _img_dec_ctx *ctx = (_img_dec_ctx *)img;
    return ctx->sum_img_num;
}

img_err_code img_dec_seek(img_dec_ctx *img, img_seek_e seek, int32_t to)
{
    if (img == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    _img_dec_ctx *ctx = (_img_dec_ctx *)img;
    int32_t seek_addr = 0;

    if (seek == SEEK_PREV)
    {
        if (ctx->now_img_num == 0)
        {
            return IMG_FILE_HEAD;
        }
        ctx->now_img_num -= 1;
    }
    else if (seek == SEEK_NEXT)
    {
        if (ctx->now_img_num == ctx->sum_img_num - 1)
        {
            return IMG_FILE_TAIL;
        }
        ctx->now_img_num += 1;
    }
    else if (seek == SEEK_HEAD)
    {
        ctx->now_img_num = 0;
    }
    else if (seek == SEEK_TAIL)
    {
        ctx->now_img_num = ctx->sum_img_num - 1;
    }
    else if (seek == SEEK_GOTO)
    {
        if (to < 0 || to > ctx->sum_img_num - 1)
        {
            return IMG_SEEK_ERR;
        }
        ctx->now_img_num = to;
    }
    else
    {
        return IMG_PARAM_INVALID;
    }

    seek_addr = ctx->param.file_offset + ctx->img_size * ctx->now_img_num;
    fseek(ctx->fp, seek_addr, SEEK_SET);

    return IMG_OK;
}

int32_t img_dec_tell(img_dec_ctx *img)
{
    if (img == NULL)
    {
        return -1;
    }
    _img_dec_ctx *ctx = (_img_dec_ctx *)img;
    return ctx->now_img_num;
}

img_err_code img_dec(img_dec_ctx *img, void *data, int32_t len)
{
    int32_t read_size = 0;
    if (img == NULL || data == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    _img_dec_ctx *ctx = (_img_dec_ctx *)img;
    if (len < ctx->param.height * ctx->param.width * 3)
    {
        return IMG_PARAM_OVERFLOW;
    }

    read_size = fread(ctx->buf, 1, ctx->img_size, ctx->fp);
    if (read_size != ctx->img_size)
    {
        return IMG_OTHER_ERR;
    }

    if (ctx->param.is_big_endian &&
        (ctx->param.format == FMT_RGB565   ||
         ctx->param.format == FMT_BGR565   ||
         ctx->param.format == FMT_ARGB1555 ||
         ctx->param.format == FMT_BGRA5551 ) )
    {
        uint16_t *d = (uint16_t *)ctx->buf;
        const uint16_t *end = d + ctx->param.height * ctx->param.width;
        while (d < end) {
            *d = ((*d & 0x00FF) << 8) | ((*d & 0xFF00) >> 8);
            d++;
        }
    }

    ctx->func(ctx->buf + ctx->param.img_head_size, data, ctx->param.width, ctx->param.height);

    return IMG_OK;
}

static void bitmap_rl_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;
    int32_t he = 0;

    memset(out, 0, h * v * 3);
    he = (h + 7) >> 3;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[he * y + (x >> 3)] & (0x01 << (x & 0x07)))
            {
                out[(y * h + x) * 3] = 0xFF;
                out[(y * h + x) * 3 + 1] = 0xFF;
                out[(y * h + x) * 3 + 2] = 0xFF;
            }
        }
    }
}

static void bitmap_rm_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;
    int32_t he = 0;

    memset(out, 0, h * v * 3);
    he = (h + 7) >> 3;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[he * y + (x >> 3)] & (0x80 >> (x & 0x07)))
            {
                out[(y * h + x) * 3] = 0xFF;
                out[(y * h + x) * 3 + 1] = 0xFF;
                out[(y * h + x) * 3 + 2] = 0xFF;
            }
        }
    }
}

static void bitmap_cl_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;
    int32_t ve = 0;

    memset(out, 0, h * v * 3);
    ve = (v + 7) >> 3;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[ve * x + (y >> 3)] & (0x01 << (y & 0x07)))
            {
                out[(y * h + x) * 3] = 0xFF;
                out[(y * h + x) * 3 + 1] = 0xFF;
                out[(y * h + x) * 3 + 2] = 0xFF;
            }
        }
    }
}

static void bitmap_cm_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;
    int32_t ve = 0;

    memset(out, 0, h * v * 3);
    ve = (v + 7) >> 3;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[ve * x + (y >> 3)] & (0x80 >> (y & 0x07)))
            {
                out[(y * h + x) * 3] = 0xFF;
                out[(y * h + x) * 3 + 1] = 0xFF;
                out[(y * h + x) * 3 + 2] = 0xFF;
            }
        }
    }
}

static void bitmap_rcl_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;

    memset(out, 0, h * v * 3);

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y + v * (x >> 3)] & (0x01 << (x & 0x07)))
            {
                out[(y * h + x) * 3] = 0xFF;
                out[(y * h + x) * 3 + 1] = 0xFF;
                out[(y * h + x) * 3 + 2] = 0xFF;
            }
        }
    }
}

static void bitmap_rcm_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;

    memset(out, 0, h * v * 3);

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y + v * (x >> 3)] & (0x80 >> (x & 0x07)))
            {
                out[(y * h + x) * 3] = 0xFF;
                out[(y * h + x) * 3 + 1] = 0xFF;
                out[(y * h + x) * 3 + 2] = 0xFF;
            }
        }
    }
}

static void bitmap_crl_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;

    memset(out, 0, h * v * 3);

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[x + h * (y >> 3)] & (0x01 << (y & 0x07)))
            {
                out[(y * h + x) * 3] = 0xFF;
                out[(y * h + x) * 3 + 1] = 0xFF;
                out[(y * h + x) * 3 + 2] = 0xFF;
            }
        }
    }
}

static void bitmap_crm_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;

    memset(out, 0, h * v * 3);

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[x + h * (y >> 3)] & (0x80 >> (y & 0x07)))
            {
                out[(y * h + x) * 3] = 0xFF;
                out[(y * h + x) * 3 + 1] = 0xFF;
                out[(y * h + x) * 3 + 2] = 0xFF;
            }
        }
    }
}

static void web_to_rgb888(uint8_t *in, uint8_t *out, int h, int v)
{
    uint8_t *d          = out;
    const uint8_t *s   = (const uint8_t *)in;
    const uint8_t *end = s + h * v;

    while (s < end) {
        register uint8_t i = *s++;
        *d++ = web_color[i] >> 16;
        *d++ = web_color[i] >> 8;
        *d++ = web_color[i];
    }
}

static void rgb565_to_rgb888(uint8_t *in, uint8_t *out, int h, int v)
{
    uint8_t *d          = out;
    const uint16_t *s   = (const uint16_t *)in;
    const uint16_t *end = s + h * v;

    while (s < end) {
        register uint16_t rgb = *s++;
        *d++ = (rgb & 0xF800) >> 8;
        *d++ = (rgb & 0x07E0) >> 3;
        *d++ = (rgb & 0x001F) << 3;
        // ffmpeg 里面这样写目的是对低几位的数据引入随机性，以消除颜色过渡不均带来的纹理
        // *d++ = ((rgb & 0xF800) >> 8) | ((rgb & 0xF800) >> 13);
        // *d++ = ((rgb & 0x07E0) >> 3) | ((rgb & 0x07E0) >> 9);
        // *d++ = ((rgb & 0x001F) << 3) | ((rgb & 0x001F) >> 2);
    }
}

static void bgr565_to_rgb888(uint8_t *in, uint8_t *out, int h, int v)
{
    uint8_t *d          = out;
    const uint16_t *s   = (const uint16_t *)in;
    const uint16_t *end = s + h * v;

    while (s < end) {
        register uint16_t bgr = *s++;
        *d++ = (bgr & 0x001F) << 3;
        *d++ = (bgr & 0x07E0) >> 3;
        *d++ = (bgr & 0xF800) >> 8;
        // ffmpeg 里面这样写目的是对低几位的数据引入随机性，以消除颜色过渡不均带来的纹理
        // *d++ = ((bgr & 0x001F) << 3) | ((bgr & 0x001F) >> 2);
        // *d++ = ((bgr & 0x07E0) >> 3) | ((bgr & 0x07E0) >> 9);
        // *d++ = ((bgr & 0xF800) >> 8) | ((bgr & 0xF800) >> 13);
    }
}

static void argb1555_to_rgb888(uint8_t *in, uint8_t *out, int h, int v)
{
    uint8_t *d          = out;
    const uint16_t *s   = (const uint16_t *)in;
    const uint16_t *end = s + h * v;

    while (s < end) {
        register uint16_t rgb = *s++;
        if (rgb & 0x8000)
        {
            *d++ = (rgb & 0x7C00) >> 7;
            *d++ = (rgb & 0x03E0) >> 2;
            *d++ = (rgb & 0x001F) << 3;
            // ffmpeg 中的写法略
        }
        else
        {
            *d++ = 0xFF;
            *d++ = 0xFF;
            *d++ = 0xFF;
        }
    }
}

static void bgra5551_to_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    uint8_t *d          = out;
    const uint16_t *s   = (const uint16_t *)in;
    const uint16_t *end = s + h * v;

    while (s < end) {
        register uint16_t rgb = *s++;
        if (rgb & 0x0001)
        {
            *d++ = (rgb & 0x003E) << 2;
            *d++ = (rgb & 0x07C0) >> 3;
            *d++ = (rgb & 0xF800) >> 8;
            // ffmpeg 中的写法略
        }
        else
        {
            *d++ = 0xFF;
            *d++ = 0xFF;
            *d++ = 0xFF;
        }
    }
}
