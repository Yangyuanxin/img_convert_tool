/**
 * @file img_enc.c
 * @author dma
 * @brief 将BMP图片转换为其他格式图像
 * @note
 * 
 * @version 0.2
 * @date 2023-04-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "img_enc.h"

#define SAFE_FREE(p) do { if (NULL != (p)){ free(p); (p) = NULL; } }while(0)

typedef uint8_t pixel_rgb888[3];
typedef int8_t pixel_rgb_err[3];

typedef void(*convert)(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bitmap_rl(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bitmap_rm(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bitmap_cl(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bitmap_cm(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bitmap_rcl(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bitmap_rcm(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bitmap_crl(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bitmap_crm(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_web(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_rgb565(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_bgr565(uint8_t *in, uint8_t *out, int32_t h, int32_t v);
static void rgb888_to_argb1555(uint8_t *in, uint8_t *out, int32_t h, int32_t v, uint32_t transparence);
static void rgb888_to_bgra5551(uint8_t *in, uint8_t *out, int32_t h, int32_t v, uint32_t transparence);

static const convert convert_list[] = {
    NULL,
    rgb888_to_bitmap_rl,
    rgb888_to_bitmap_rm,
    rgb888_to_bitmap_cl,
    rgb888_to_bitmap_cm,
    rgb888_to_bitmap_rcl,
    rgb888_to_bitmap_rcm,
    rgb888_to_bitmap_crl,
    rgb888_to_bitmap_crm,
    rgb888_to_web,
    rgb888_to_rgb565,
    rgb888_to_bgr565,
    // rgb888_to_bgra5551,
    // rgb888_to_argb1555,
};


typedef enum
{
    COLOR_RGB888, // 存储方式 RGBRGBRGB
    COLOR_GRAY_8, // 存储方式 GGG------，G表示0到255的灰度值，-表示忽略
    COLOR_BITMAP_8, // 存储方式 BBB------，B为0或1的值，-表示忽略
} _color_type_e;

typedef struct
{
    uint32_t width;
    uint32_t height;
    // int32_t padding_width;
    // int32_t padding_height;
    _color_type_e color;
    uint8_t *buf;
} _img_buf;

typedef struct
{
    uint32_t width; // 最终输出图片的宽度（预览图和它保持一致）
    uint32_t height; // 最终输出图片的宽度（预览图和它保持一致）
    int32_t img_size; // 最终输出图片的大小
    int32_t img_size_preview; // 预览图片的大小
    _img_buf in_buf; // 输入的图片原始数据，后续处理步骤不要修改这里的数据
    _img_buf effect_buf_prev; // 保存处理前的图片数据，每一步处理完成后，这两者互相调换
    _img_buf effect_buf_next; // 保存处理后的图片数据，每一步处理完成后，这两者互相调换
    convert func;
    img_enc_param param;
} _img_enc_ctx;


// 图像边缘识别算子
typedef struct {
    int32_t num;
    int32_t size;
    char core[2][4][4];
} operator;

// 索贝尔算子
operator Sobel = {
    .num = 2,
    .size = 3,
    .core = {
        {
            {-1, 0, 1, 0},
            {-2, 0, 2, 0},
            {-1, 0, 1, 0},
            { 0, 0, 0, 0},
        },
        {
            {-1, -2, -1, 0},
            { 0,  0,  0, 0},
            { 1,  2,  1, 0},
            { 0,  0,  0, 0},
        }
    }
};


// 位图文件头
typedef struct _BMP_FILE_HEAD
{
    // unsigned short bfType; // 文件类型，必须是0x424D，也就是字符BM，为方便结构体对齐，此成员单独处理
    unsigned int bfSize; // 文件大小，包含头
    unsigned short bfReserved1; // 保留字
    unsigned short bfReserved2; // 保留字
    unsigned int bfOffBits; // 文件头到实际的图像数据的偏移字节数
} BMP_FILE_HEAD;

// 位图信息头
typedef struct _BMP_INFO_HEAD
{
    unsigned int biSize; // 这个结构体的长度，为40字节
    int biWidth; // 图像的宽度
    int biHeight; // 图像的长度
    unsigned short biPlanes; // 必须是1
    unsigned short biBitCount; // 表示颜色时要用到的位数，常用的值为 1（黑白二色图）,4（16 色图）,8（256 色）,24（真彩色图）（新的.bmp 格式支持 32 位色，这里不做讨论）
    unsigned int biCompression; // 指定位图是否压缩，有效的值为 BI_RGB，BI_RLE8，BI_RLE4，BI_BITFIELDS（都是一些Windows定义好的常量，暂时只考虑BI_RGB不压缩的情况）
    unsigned int biSizeImage; // 指定实际的位图数据占用的字节数
    int biXPelsPerMeter; // 指定目标设备的水平分辨率
    int biYPelsPerMeter; // 指定目标设备的垂直分辨率
    unsigned int biClrUsed; // 指定本图象实际用到的颜色数，如果该值为零，则用到的颜色数为 2 的 biBitCount 次方
    unsigned int biClrImportant; // 指定本图象中重要的颜色数，如果该值为零，则认为所有的颜色都是重要的
} BMP_INFO_HEAD;

typedef struct _BMP_HEAD
{
    BMP_FILE_HEAD bfh;
    BMP_INFO_HEAD bih;
} BMP_HEAD;


static char *get_ext_name(char* filename)
{
    char *ptr = NULL;
    uint32_t len;

    if (filename == NULL)
    {
        return NULL;
    }

    len = strlen(filename);
    while (len)
    {
        ptr = filename + len;
        if (*ptr == '.')
        {
            return ptr + 1;
        }
        len--;
    }

    return ptr;
}

static img_err_code load_bmp_info(FILE *fp, BMP_HEAD *bh)
{
    uint16_t bfType = 0;

    memset(bh, 0, sizeof(BMP_HEAD));

    (void)fread(&bfType, 1, sizeof(bfType), fp);
    if (bfType != 0x4d42)
    {
        printf("not bitmap file\n");
        return IMG_FORMAT_UNKNOWN;
    }

    (void)fread(&bh->bfh, 1, sizeof(BMP_FILE_HEAD), fp);
    (void)fread(&bh->bih, 1, sizeof(BMP_INFO_HEAD), fp);

    if (bh->bih.biBitCount != 24)
    {
        // 暂时只支持RGB888的BMP图像
        printf("format not supported\n");
        return IMG_FORMAT_NOT_SUPPORT;
    }

    if (bh->bih.biCompression != 0)
    {
        printf("compressed bitmap format not support\n");
        return IMG_FORMAT_NOT_SUPPORT;
    }

    if (bh->bih.biHeight == 0 || bh->bih.biWidth == 0)
    {
        printf("height or width error\n");
        return IMG_FORMAT_ERR;
    }

    return IMG_OK;
}

static img_err_code load_bmp_data(FILE *fp, BMP_HEAD *bh, _img_buf *img)
{
    uint32_t stride = 0;
    uint32_t bmp_data_size = 0;
    uint32_t read_size = 0;
    uint32_t offset = 0;
    uint32_t h = 0, v = 0, i = 0, j = 0;
    uint8_t *bmp_data = NULL;

    h = bh->bih.biWidth;
    v = bh->bih.biHeight;

    // BMP图像每行4字节取整向上对齐
    stride = ((h * 3 + 3) >> 2) << 2;

    bmp_data_size = bh->bih.biSizeImage == 0 ? stride * v : bh->bih.biSizeImage;
    bmp_data = (uint8_t *)malloc(bmp_data_size);
    if (bmp_data == NULL)
    {
        return IMG_MEM_WRONG;
    }

    fseek(fp, bh->bfh.bfOffBits, SEEK_SET);
    read_size = fread(bmp_data, 1, bmp_data_size, fp);
    if (read_size != bmp_data_size)
    {
        SAFE_FREE(bmp_data);
        return IMG_FORMAT_ERR;
    }

    // 翻转顺序
    for (i = 0; i < v; i++)
    {
        for (j = 0; j < h; j++)
        {
            offset = (v - i - 1) * stride + j * 3;

            img->buf[i * h * 3 + j * 3]     = bmp_data[offset + 2];
            img->buf[i * h * 3 + j * 3 + 1] = bmp_data[offset + 1];
            img->buf[i * h * 3 + j * 3 + 2] = bmp_data[offset];
        }
    }
    img->color = COLOR_RGB888;
    img->height = bh->bih.biHeight;
    img->width = bh->bih.biWidth;

    SAFE_FREE(bmp_data);
    return IMG_OK;
}

// rgb888转为以rgb888格式保存的web颜色，仅预览使用
static void rgb8882rgb888_web(uint8_t *in, int32_t h, int32_t v)
{
    static uint8_t color_table[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};
    int32_t i = 0;
    int32_t color_id = 0;
    uint8_t color = 0;

    for (i = 0; i < h * v * 3; i++)
    {
        // 0,26,77,128,179,229,255 量化相对而言效果略好
        // 0,42,85,128,170,213,255 量化最简单直接
        color = in[i];
        color_id = color > 229 ? 5 : (color + 26) / 51;
        in[i] = color_table[color_id];
    }
    return;
}

// rgb888转为以rgb888格式保存的rgb555颜色，仅预览使用（这里偷懒，rgb565按rgb555处理）
static void rgb8882rgb888_rgb555(uint8_t *in, int32_t h, int32_t v)
{
    int32_t i = 0;
    for (i = 0; i < h * v * 3; i++)
    {
        in[i] = in[i] & 0xF8;
    }
    return;
}

static void rgb8882gray(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    // 参考资料
    // https://www.cnblogs.com/zhangjiansheng/p/6925722.html
    // 采用 BT.601 标准中 Y 分量的转换公式
    // Gray = R0.299 + G0.587 + B*0.114

    int32_t y = 0;
    int32_t i = 0;
    int32_t j = 0;
    for (i = 0; i < h * v * 3; i += 3)
    {
        y = (in[i+0]*76 + in[i+1]*150 + in[i+2]*29) >> 8;
        out[j] = y;
        j += 1;
    }
    return;
}

static void gray2rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t i = 0;
    for (i = 0; i < h * v; i++)
    {
        out[i*3+0] = out[i*3+1] = out[i*3+2] = in[i];
    }
    return;
}

static void gray2binarization(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t i;
    for (i = 0; i < h*v; i++)
    {
        out[i] = in[i] > 128 ? 255 : 0;
    }
    return;
}

// 线性调整亮度、对比度，效果肯定不如ps之类的专业软件，但能凑合用
// 亮度和对比度的默认值都是0，取值范围+-100
static void gray_luminance(uint8_t *in, uint8_t *out, int32_t h, int32_t v, int32_t luminance, int32_t contrast)
{
    int32_t i, j;
    int32_t tmp;

    for (i = 0; i < v; i++)
    {
        for (j = 0; j < h; j++)
        {
            tmp = in[i*h+j];

            if (luminance > 0)
            {
                tmp = (tmp-255) * 100 / (luminance + 100) + 255;
            }
            if (luminance < 0)
            {
                tmp = (tmp) * 100 / (-luminance + 100);
            }

            if (contrast > 0)
            {
                tmp = (tmp-128) * (contrast + 100) / 100 + 128;
            }
            if (contrast < 0)
            {
                tmp = (tmp-128) * 100 / (-contrast + 100) + 128;
            }

            if (tmp > 255) {tmp = 255;}
            if (tmp < 0) {tmp = 0;}
            out[i*h+j] = tmp;
        }
    }

    return;
}

static img_err_code sobel_edge_detector(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t he = h + 2;
    int32_t ve = v + 2;
    int32_t i, j, k;
    int32_t m, n;
    int32_t sum = 0;
    int32_t opt_size = Sobel.size;
    int32_t opt_num = Sobel.num;
    uint8_t *canvas_in = NULL;
    canvas_in = (uint8_t *)malloc(he*ve);
    if (canvas_in == NULL)
    {
        return IMG_MEM_WRONG;
    }
    memset(canvas_in, 0, he*ve);
    memset(out, 0, h*v);

    // 复制原图到中心位置
    for (i = 0; i < v; i++)
    {
        memcpy(&canvas_in[(i+1)*he+1], &in[i*h], h);
    }

    // 复制四周边缘的1像素
    canvas_in[0] = in[0];
    canvas_in[he-1] = in[(h-1)];
    canvas_in[(ve-1)*he] = in[(v-1)*h];
    canvas_in[he*ve-1] = in[(h*v-1)];
    // for (i = 0; i < h; i++)
    // {
    //     canvas_in[i+1] = in[i];
    // }
    memcpy(&canvas_in[1], in, h);
    // for (i = 0; i < h; i++)
    // {
    //     canvas_in[he*(ve-1)+i+1] = in[h*(v-1)+i];
    // }
    memcpy(&canvas_in[he*(ve-1)+1], &in[h*(v-1)], h);
    for (i = 0; i < v; i++)
    {
        canvas_in[(i+1)*he] = in[i*h];
    }
    for (i = 0; i < v; i++)
    {
        canvas_in[(i+2)*he-1] = in[(i+1)*h-1];
    }

    // 卷积
    for (k = 0; k < opt_num; k++)
    {
        for (i = 0; i < v; i++)
        {
            for (j = 0; j < h; j++)
            {
                sum = 0;
                for (m = 0; m < opt_size; m++)
                {
                    for (n = 0; n < opt_size; n++)
                    {
                        sum += canvas_in[(i + m) * he + j + n] * Sobel.core[k][m][n];
                    }
                }
                sum /= 2;
                if (sum < 0) {sum = -sum;}
                sum += out[i * h + j];
                if (sum > 255) {sum = 255;}
                out[i * h + j] = sum;
            }
        }
    }

    SAFE_FREE(canvas_in);
    return IMG_OK;
}

static uint8_t dither_limit_denominator_16(uint8_t in, int8_t err, int32_t numerator)
{
    int32_t out = in + err * numerator / 16;
    if (out > 255) {out = 255;}
    if (out < 0) {out = 0;}
    return (uint8_t)out;
}

static void dither_color_space_bitmap(uint8_t *in, uint8_t *out, int8_t *err)
{
    // 这里写为>127或>=128，避免err溢出
    *out = *in > 127 ? 255 : 0;
    *err = *in - *out;
    return;
}

static void dither_color_space_web(pixel_rgb888 *in, pixel_rgb888 *out, pixel_rgb_err *err)
{
    static uint8_t color_table[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};
    int32_t i = 0;
    int32_t color_id = 0;
    uint8_t color = 0;

    for (i = 0; i < 3; i++)
    {
        // 0,26,77,128,179,229,255 量化相对而言效果略好
        // 0,42,85,128,170,213,255 量化最简单直接
        color = (*in)[i];
        color_id = color > 229 ? 5 : (color + 26) / 51;
        (*out)[i] = color_table[color_id];
        (*err)[i] = (*in)[i] - (*out)[i];
    }
    return;
}

// 颜色抖动算法都按照RGB555处理，对于RGB565到RGB555的G通道色彩损失相当于用抖动算法弥补
static void dither_color_space_rgb555(pixel_rgb888 *in, pixel_rgb888 *out, pixel_rgb_err *err)
{
    int32_t i = 0;
    for (i = 0; i < 3; i++)
    {
        (*out)[i] = (*in)[i] & 0xF8;
        (*err)[i] = (*in)[i] & 0x07;
    }
    return;
}

static img_err_code floyd_steinberg_dither_gray8(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    // 参考资料
    // https://blog.csdn.net/qq_42676511/article/details/120626723
    // https://github.com/Rudranil-Sarkar/Floyd-Steinberg-dithering-algo/blob/master/bitmap.cpp

    int32_t he = h + 2;
    int32_t ve = v + 2;
    int32_t i, j;

    uint8_t *canvas_in = NULL;
    canvas_in = (uint8_t *)malloc(he*ve);
    if (canvas_in == NULL)
    {
        return IMG_MEM_WRONG;
    }
    memset(canvas_in, 0, he*ve);
    memset(out, 0, h*v);

    // 图像边缘向外扩展1像素，复制原图到中心位置
    for (i = 0; i < v; i++)
    {
        memcpy(&canvas_in[(i+1)*he+1], &in[i*h], h);
    }

    // 扩散矩阵
    // ...  ...  src  7/16  ...
    // ...  3/16 5/16 1/16  ...
    for (i = 0; i < v; i++)
    {
        for (j = 0; j < h; j++)
        {
            uint8_t *pixel = NULL;
            uint8_t *p_in = &canvas_in[(i+1)*he+j+1];
            uint8_t *p_out = &out[i*h+j];
            int8_t p_err = 0;

            // 计算误差
            dither_color_space_bitmap(p_in, p_out, &p_err);

            // 扩散误差
            pixel = &canvas_in[(i+1)*he+j+1];
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err, 7);

            pixel += he - 2;
            *pixel = dither_limit_denominator_16(*pixel, p_err, 3);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err, 5);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err, 1);
        }
    }

    SAFE_FREE(canvas_in);
    return IMG_OK;
}

static img_err_code floyd_steinberg_dither_rgb888(uint8_t *in, uint8_t *out, int32_t h, int32_t v,
                                                  void(*dither_color_space)(pixel_rgb888 *in, pixel_rgb888 *out, pixel_rgb_err *err))
{
    // 参考资料
    // https://blog.csdn.net/qq_42676511/article/details/120626723
    // https://github.com/Rudranil-Sarkar/Floyd-Steinberg-dithering-algo/blob/master/bitmap.cpp

    int32_t he = h + 2;
    int32_t ve = v + 2;
    int32_t i, j;

    uint8_t *canvas_in = NULL;
    canvas_in = (uint8_t *)malloc(he*ve*3);
    if (canvas_in == NULL)
    {
        return IMG_MEM_WRONG;
    }
    memset(canvas_in, 0, he*ve*3);
    memset(out, 0, h*v*3);

    // 图像边缘向外扩展1像素，复制原图到中心位置
    for (i = 0; i < v; i++)
    {
        memcpy(&canvas_in[(i+1)*he*3 + 3], &in[i*h*3], h * 3);
    }

    // 抖动算法
    for (i = 0; i < v; i++)
    {
        for (j = 0; j < h; j++)
        {
            uint8_t *pixel = NULL;
            pixel_rgb888 *p_in = (pixel_rgb888 *)&canvas_in[(i+1)*he*3+(j+1)*3];
            pixel_rgb888 *p_out = (pixel_rgb888 *)&out[i*h*3+j*3];
            pixel_rgb_err p_err = {0};

            // 计算误差
            dither_color_space(p_in, p_out, &p_err);
            
            // 扩散误差
            pixel = &canvas_in[(i+1)*he*3+(j+1)*3];
            pixel += 3;
            *pixel = dither_limit_denominator_16(*pixel, p_err[0], 7);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[1], 7);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[2], 7);

            pixel += he * 3 - 9;
            *pixel = dither_limit_denominator_16(*pixel, p_err[0], 3);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[1], 3);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[2], 3);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[0], 5);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[1], 5);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[2], 5);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[0], 1);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[1], 1);
            pixel += 1;
            *pixel = dither_limit_denominator_16(*pixel, p_err[2], 1);
        }
    }

    SAFE_FREE(canvas_in);
    return IMG_OK;
}

static void color_invert(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t i;
    for (i = 0; i < h*v; i++)
    {
        out[i] = ~in[i];
    }
    return;
}

// 每一步处理完成后调换 effect_buf_prev 和 effect_buf_next，处理完成后 effect_buf_prev 内保存最终的图像
static img_err_code img_enc_effect(img_enc_ctx *img)
{
    img_err_code err_code = IMG_OK;
    _img_buf img_buf_tmp;
    uint32_t out_size = 0;
    _img_enc_ctx *ctx = (_img_enc_ctx *)img;
    out_size = ctx->in_buf.height * ctx->in_buf.width * 3;

    memcpy(ctx->effect_buf_prev.buf, ctx->in_buf.buf, out_size);
    ctx->effect_buf_prev.color = COLOR_RGB888;
    ctx->effect_buf_prev.width = ctx->in_buf.width;
    ctx->effect_buf_prev.height = ctx->in_buf.height;

    if (ctx->param.format >= FMT_BITMAP_RL && ctx->param.format <= FMT_BITMAP_CRM)
    {
        // rgb转灰度
        rgb8882gray(ctx->effect_buf_prev.buf, ctx->effect_buf_next.buf, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
        ctx->effect_buf_next.color = COLOR_GRAY_8;
        ctx->effect_buf_next.width = ctx->effect_buf_prev.width;
        ctx->effect_buf_next.height = ctx->effect_buf_prev.height;
        img_buf_tmp = ctx->effect_buf_prev;
        ctx->effect_buf_prev = ctx->effect_buf_next;
        ctx->effect_buf_next = img_buf_tmp;

        // 灰度图像修改亮度
        gray_luminance(ctx->effect_buf_prev.buf, ctx->effect_buf_next.buf, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height,
                       ctx->param.luminance, ctx->param.contrast);
        ctx->effect_buf_next.color = COLOR_GRAY_8;
        ctx->effect_buf_next.width = ctx->effect_buf_prev.width;
        ctx->effect_buf_next.height = ctx->effect_buf_prev.height;
        img_buf_tmp = ctx->effect_buf_prev;
        ctx->effect_buf_prev = ctx->effect_buf_next;
        ctx->effect_buf_next = img_buf_tmp;

        // 边缘识别
        if (ctx->param.use_edge_detector)
        {
            err_code = sobel_edge_detector(ctx->effect_buf_prev.buf, ctx->effect_buf_next.buf, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
            if (err_code)
            {
                return err_code;
            }
            ctx->effect_buf_next.color = COLOR_GRAY_8;
            ctx->effect_buf_next.width = ctx->effect_buf_prev.width;
            ctx->effect_buf_next.height = ctx->effect_buf_prev.height;
            img_buf_tmp = ctx->effect_buf_prev;
            ctx->effect_buf_prev = ctx->effect_buf_next;
            ctx->effect_buf_next = img_buf_tmp;
        }

        // 抖动
        if (ctx->param.use_dithering_algorithm)
        {
            err_code = floyd_steinberg_dither_gray8(ctx->effect_buf_prev.buf, ctx->effect_buf_next.buf, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
            if (err_code)
            {
                return err_code;
            }
            ctx->effect_buf_next.color = COLOR_GRAY_8;
            ctx->effect_buf_next.width = ctx->effect_buf_prev.width;
            ctx->effect_buf_next.height = ctx->effect_buf_prev.height;
            img_buf_tmp = ctx->effect_buf_prev;
            ctx->effect_buf_prev = ctx->effect_buf_next;
            ctx->effect_buf_next = img_buf_tmp;
        }

        // 反色
        if (ctx->param.is_invert)
        {
            color_invert(ctx->effect_buf_prev.buf, ctx->effect_buf_next.buf, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
            ctx->effect_buf_next.color = COLOR_GRAY_8;
            ctx->effect_buf_next.width = ctx->effect_buf_prev.width;
            ctx->effect_buf_next.height = ctx->effect_buf_prev.height;
            img_buf_tmp = ctx->effect_buf_prev;
            ctx->effect_buf_prev = ctx->effect_buf_next;
            ctx->effect_buf_next = img_buf_tmp;
        }
    }
    else if (ctx->param.format == FMT_WEB)
    {
        // 抖动
        if (ctx->param.use_dithering_algorithm)
        {
            err_code = floyd_steinberg_dither_rgb888(ctx->effect_buf_prev.buf, ctx->effect_buf_next.buf,
                                                     ctx->effect_buf_prev.width, ctx->effect_buf_prev.height, dither_color_space_web);
            if (err_code)
            {
                return err_code;
            }
            ctx->effect_buf_next.color = COLOR_RGB888;
            ctx->effect_buf_next.width = ctx->effect_buf_prev.width;
            ctx->effect_buf_next.height = ctx->effect_buf_prev.height;
            img_buf_tmp = ctx->effect_buf_prev;
            ctx->effect_buf_prev = ctx->effect_buf_next;
            ctx->effect_buf_next = img_buf_tmp;
        }
    }
    else if (ctx->param.format >= FMT_RGB565 && ctx->param.format <= FMT_BGRA5551)
    {
        // 抖动
        if (ctx->param.use_dithering_algorithm)
        {
            err_code = floyd_steinberg_dither_rgb888(ctx->effect_buf_prev.buf, ctx->effect_buf_next.buf,
                                                     ctx->effect_buf_prev.width, ctx->effect_buf_prev.height, dither_color_space_rgb555);
            if (err_code)
            {
                return err_code;
            }
            ctx->effect_buf_next.color = COLOR_RGB888;
            ctx->effect_buf_next.width = ctx->effect_buf_prev.width;
            ctx->effect_buf_next.height = ctx->effect_buf_prev.height;
            img_buf_tmp = ctx->effect_buf_prev;
            ctx->effect_buf_prev = ctx->effect_buf_next;
            ctx->effect_buf_next = img_buf_tmp;
        }
    }

    return IMG_OK;
}

img_enc_ctx *img_enc_open(char *path)
{
    FILE *img_fp;
    uint32_t buf_size;
    _img_enc_ctx *ctx;
    BMP_HEAD bh;
    img_err_code err_code = IMG_OK;
    char *ext_name;

    ext_name = get_ext_name(path);
    if(ext_name == NULL)
    {
        printf("unknown file name\n");
        return NULL;
    }
    if (strcmp(ext_name, "bmp") != 0 && strcmp(ext_name, "BMP") != 0)
    {
        printf("file format not support\n");
        return NULL;
    }

    img_fp = fopen(path, "rb");
    if (img_fp == NULL)
    {
        printf("can not open %s\n", path);
        return NULL;
    }

    err_code = load_bmp_info(img_fp, &bh);
    if (err_code)
    {
        goto end;
    }

    ctx = (_img_enc_ctx *)malloc(sizeof(_img_enc_ctx));
    if (ctx == NULL)
    {
        printf("create img_ctx error\n");
        goto end;
    }
    memset(ctx, 0, sizeof(_img_enc_ctx));

    buf_size = bh.bih.biHeight * bh.bih.biWidth * 3;
    ctx->in_buf.buf = (uint8_t *)malloc(buf_size);
    ctx->effect_buf_prev.buf = (uint8_t *)malloc(buf_size);
    ctx->effect_buf_next.buf = (uint8_t *)malloc(buf_size);
    if (ctx->in_buf.buf == NULL ||
        ctx->effect_buf_prev.buf == NULL ||
        ctx->effect_buf_next.buf == NULL)
    {
        printf("create img buf error\n");
        goto end;
    }
    memset(ctx->in_buf.buf, 0, buf_size);
    memset(ctx->effect_buf_prev.buf, 0, buf_size);
    memset(ctx->effect_buf_next.buf, 0, buf_size);

    if (load_bmp_data(img_fp, &bh, &ctx->in_buf))
    {
        goto end;
    }

    fclose(img_fp);

    return ctx;

end:
    SAFE_FREE(ctx->in_buf.buf);
    SAFE_FREE(ctx->effect_buf_prev.buf);
    SAFE_FREE(ctx->effect_buf_next.buf);
    SAFE_FREE(ctx);
    fclose(img_fp);
    return NULL;
}

img_err_code img_enc_close(img_enc_ctx *img)
{
    if (img == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    _img_enc_ctx *ctx = (_img_enc_ctx *)img;

    SAFE_FREE(ctx->in_buf.buf);
    SAFE_FREE(ctx->effect_buf_prev.buf);
    SAFE_FREE(ctx->effect_buf_next.buf);
    SAFE_FREE(ctx);

    return IMG_OK;
}

img_err_code img_enc_cfg(img_enc_ctx *img, img_enc_param *param)
{
    if (img == NULL || param == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    _img_enc_ctx *ctx = (_img_enc_ctx *)img;

    if (param->format == 0 || param->format >= FMT_INVALID)
    {
        return IMG_PARAM_INVALID;
    }
    if (param->contrast > 100 || param->contrast < -100)
    {
        return IMG_PARAM_INVALID;
    }
    if (param->luminance > 100 || param->luminance < -100)
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
        ctx->img_size = ctx->in_buf.height * ((ctx->in_buf.width + 7) >> 3);
    }
    else if (param->format == FMT_BITMAP_CL  ||
             param->format == FMT_BITMAP_CM  ||
             param->format == FMT_BITMAP_CRL ||
             param->format == FMT_BITMAP_CRM)
    {
        ctx->img_size = ((ctx->in_buf.height + 7) >> 3) * ctx->in_buf.width;
    }
    else if (param->format == FMT_WEB)
    {
        ctx->img_size = ctx->in_buf.height * ctx->in_buf.width;
    }
    else if (param->format >= FMT_RGB565 && param->format <= FMT_ARGB1555)
    {
        ctx->img_size = ctx->in_buf.height * ctx->in_buf.width * 2;
    }
    ctx->func = convert_list[param->format];

    ctx->width = ctx->in_buf.width;
    ctx->height = ctx->in_buf.height;
    ctx->img_size_preview = ctx->width * ctx->height * 3;

    return IMG_OK;
}

img_err_code img_enc_get_preview_size(img_enc_ctx *img, int32_t *size, int32_t *width, int32_t *height)
{
    if (img == NULL || size == NULL || width == NULL || height == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    _img_enc_ctx *ctx = (_img_enc_ctx *)img;
    *size = ctx->img_size_preview;
    *width = ctx->width;
    *height = ctx->height;

    return IMG_OK;
}

img_err_code img_enc_get_size(img_enc_ctx *img, int32_t *size, int32_t *width, int32_t *height)
{
    if (img == NULL || size == NULL || width == NULL || height == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    _img_enc_ctx *ctx = (_img_enc_ctx *)img;
    *size = ctx->img_size;
    *width = ctx->width;
    *height = ctx->height;

    return IMG_OK;
}

img_err_code img_enc_preview(img_enc_ctx *img, void *data, int32_t len)
{
    img_err_code err_code = IMG_OK;
    _img_enc_ctx *ctx = NULL;
    uint32_t out_size = 0;
    if (img == NULL || data == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    ctx = (_img_enc_ctx *)img;
    out_size = ctx->in_buf.height * ctx->in_buf.width * 3;
    if (len < out_size)
    {
        return IMG_PARAM_INVALID;
    }
    memset(data, 0, len);

    // 预处理
    err_code = img_enc_effect(img);
    if (err_code)
    {
        return err_code;
    }

    if (ctx->param.format >= FMT_BITMAP_RL && ctx->param.format <= FMT_BITMAP_CRM)
    {
        if (ctx->effect_buf_prev.color == COLOR_GRAY_8)
        {
            // 二值化并转为rgb
            gray2binarization(ctx->effect_buf_prev.buf, ctx->effect_buf_next.buf, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
            gray2rgb888(ctx->effect_buf_next.buf, data, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
        }
        else
        {
            return IMG_OTHER_ERR;
        }
    }
    else if (ctx->param.format == FMT_WEB)
    {
        // 色彩转换
        rgb8882rgb888_web(ctx->effect_buf_prev.buf, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
        if (ctx->effect_buf_prev.color == COLOR_RGB888)
        {
            memcpy(data, ctx->effect_buf_prev.buf, out_size);
        }
        else
        {
            return IMG_OTHER_ERR;
        }
    }
    else if (ctx->param.format >= FMT_RGB565 && ctx->param.format <= FMT_BGRA5551)
    {
        // 色彩转换
        rgb8882rgb888_rgb555(ctx->effect_buf_prev.buf, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
        if (ctx->effect_buf_prev.color == COLOR_RGB888)
        {
            memcpy(data, ctx->effect_buf_prev.buf, out_size);
        }
        else
        {
            return IMG_OTHER_ERR;
        }
    }

    return IMG_OK;
}

img_err_code img_enc(img_enc_ctx *img, void *data, int32_t len)
{
    img_err_code err_code = IMG_OK;
    _img_enc_ctx *ctx = NULL;
    if (img == NULL || data == NULL)
    {
        return IMG_PARAM_NULL_PTR;
    }
    ctx = (_img_enc_ctx *)img;
    if (len < ctx->img_size)
    {
        return IMG_PARAM_OVERFLOW;
    }
    memset(data, 0, len);

    // 预处理
    err_code = img_enc_effect(img);
    if (err_code)
    {
        return err_code;
    }

    if (ctx->param.format <= FMT_BITMAP_CRM)
    {
        ctx->func(ctx->effect_buf_prev.buf, data, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
    }
    // 彩色的直接使用原图转换就行
    else if (ctx->param.format <= FMT_BGR565)
    {
        ctx->func(ctx->effect_buf_prev.buf, data, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height);
    }
    else if (ctx->param.format == FMT_ARGB1555)
    {
        rgb888_to_argb1555(ctx->effect_buf_prev.buf, data, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height, ctx->param.transparence);
    }
    else if (ctx->param.format <= FMT_BGRA5551)
    {
        rgb888_to_bgra5551(ctx->effect_buf_prev.buf, data, ctx->effect_buf_prev.width, ctx->effect_buf_prev.height, ctx->param.transparence);
    }

    // 大小端转换
    if (ctx->param.is_big_endian &&
        (ctx->param.format == FMT_RGB565   ||
         ctx->param.format == FMT_BGR565   ||
         ctx->param.format == FMT_ARGB1555 ||
         ctx->param.format == FMT_BGRA5551 ) )
    {
        uint16_t *d         = (uint16_t *)data;
        const uint16_t *end = d + ctx->effect_buf_prev.width * ctx->effect_buf_prev.height;
        while (d < end) {
            *d = ((*d & 0x00FF) << 8) | ((*d & 0xFF00) >> 8);
            d++;
        }
    }

    return IMG_OK;
}

static void rgb888_to_bitmap_rl(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;
    int32_t he = (h + 7) >> 3;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y * h + x] < 128)
            {
                continue;
            }
            out[he * y + (x >> 3)] = out[he * y + (x >> 3)] | (0x01 << (x & 0x07));
        }
    }
}

static void rgb888_to_bitmap_rm(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;
    int32_t he = (h + 7) >> 3;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y * h + x] < 128)
            {
                continue;
            }
            out[he * y + (x >> 3)] = out[he * y + (x >> 3)] | (0x80 >> (x & 0x07));
        }
    }
}

static void rgb888_to_bitmap_cl(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;
    int32_t ve = (v + 7) >> 3;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y * h + x] < 128)
            {
                continue;
            }
            out[ve * x + (y >> 3)] = out[ve * x + (y >> 3)] | (0x01 << (y & 0x07));
        }
    }
}

static void rgb888_to_bitmap_cm(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;
    int32_t ve = (v + 7) >> 3;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y * h + x] < 128)
            {
                continue;
            }
            out[ve * x + (y >> 3)] = out[ve * x + (y >> 3)] | (0x80 >> (y & 0x07));
        }
    }
}

static void rgb888_to_bitmap_rcl(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y * h + x] < 128)
            {
                continue;
            }
            out[y + v * (x >> 3)] = out[y + v * (x >> 3)] | (0x01 << (x & 0x07));
        }
    }
}

static void rgb888_to_bitmap_rcm(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y * h + x] < 128)
            {
                continue;
            }
            out[y + v * (x >> 3)] = out[y + v * (x >> 3)] | (0x80 >> (x & 0x07));
        }
    }
}

static void rgb888_to_bitmap_crl(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y * h + x] < 128)
            {
                continue;
            }
            out[x + h * (y >> 3)] = out[x + h * (y >> 3)] | (0x01 << (y & 0x07));
        }
    }
}

static void rgb888_to_bitmap_crm(uint8_t *in, uint8_t *out, int32_t h, int32_t v)
{
    int32_t x = 0, y = 0;

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            if (in[y * h + x] < 128)
            {
                continue;
            }
            out[x + h * (y >> 3)] = out[x + h * (y >> 3)] | (0x80 >> (y & 0x07));
        }
    }
}

static void rgb888_to_web(uint8_t *in, uint8_t *out, int h, int v)
{
    uint8_t *d         = (uint8_t *)out;
    const uint8_t *s   = in;
    const uint8_t *end = s + h * v * 3;

    while (s < end) {
        uint8_t r = *s++;
        uint8_t g = *s++;
        uint8_t b = *s++;
        // 0,26,77,128,179,229,255 量化相对而言效果略好
        // 0,42,85,128,170,213,255 量化最简单直接
        r = r > 229 ? 5 : (r + 26) / 51;
        g = g > 229 ? 5 : (g + 26) / 51;
        b = b > 229 ? 5 : (b + 26) / 51;
        *d++ = r * 36 + g * 6 + b;
    }
}

static void rgb888_to_rgb565(uint8_t *in, uint8_t *out, int h, int v)
{
    uint16_t *d        = (uint16_t *)out;
    const uint8_t *s   = in;
    const uint8_t *end = s + h * v * 3;

    while (s < end) {
        const int r = *s++;
        const int g = *s++;
        const int b = *s++;
        *d++        = (b >> 3) | ((g & 0xFC) << 3) | ((r & 0xF8) << 8);
    }
}

static void rgb888_to_bgr565(uint8_t *in, uint8_t *out, int h, int v)
{
    uint16_t *d        = (uint16_t *)out;
    const uint8_t *s   = in;
    const uint8_t *end = s + h * v * 3;

    while (s < end) {
        const int r = *s++;
        const int g = *s++;
        const int b = *s++;
        *d++        = ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3);
    }
}

static void rgb888_to_argb1555(uint8_t *in, uint8_t *out, int h, int v, uint32_t transparence)
{
    uint16_t *d        = (uint16_t *)out;
    const uint8_t *s   = in;
    const uint8_t *end = s + h * v * 3;
    uint8_t tr = (transparence >> 16) & 0xFF;
    uint8_t tg = (transparence >> 8) & 0xFF;
    uint8_t tb = (transparence >> 0) & 0xFF;

    while (s < end) {
        const int r = *s++;
        const int g = *s++;
        const int b = *s++;
        if (r == tr && g == tg && b == tb)
        {
            *d++ = 0;
        }
        else
        {
            *d++ = (b >> 3) | ((g & 0xF8) << 2) | ((r & 0xF8) << 7) | 0x8000;
        }
    }
}

static void rgb888_to_bgra5551(uint8_t *in, uint8_t *out, int h, int v, uint32_t transparence)
{
    uint16_t *d        = (uint16_t *)out;
    const uint8_t *s   = in;
    const uint8_t *end = s + h * v * 3;
    uint8_t tr = (transparence >> 16) & 0xFF;
    uint8_t tg = (transparence >> 8) & 0xFF;
    uint8_t tb = (transparence >> 0) & 0xFF;

    while (s < end) {
        const int r = *s++;
        const int g = *s++;
        const int b = *s++;
        if (r == tr && g == tg && b == tb)
        {
            *d++ = 0;
        }
        else
        {
            *d++ = ((b & 0xF8) << 8) | ((g & 0xF8) << 3) | (r >> 2) | 0x0001;
        }
    }
}
