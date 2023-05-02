/**
 * @file main.c
 * @author dma
 * @brief 图像转换工具
 * @note
 * 
 * @version 0.4
 * @date 2023-05-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "argparse.h"
#include "img_dec.h"
#include "img_enc.h"
#include "img_common.h"

#define SPECIFICATION \
"==== format specification ====\n" \
"bitmap_rl, bitmap_rm, bitmap_cl, bitmap_cm,\n" \
"bitmap_rcl, bitmap_rcm, bitmap_crl, bitmap_crm,\n" \
"web, rgb565, bgr565, argb1555, bgra5551,\n" \
"\n" \
"For more information please refer to the readme.md\n" \

#define SAFE_FREE(p) do { if (NULL != (p)){ free(p); (p) = NULL; } }while(0)

typedef struct {
    fmt_e fmt;
    char fmt_str[12];
} fmt_s;

const fmt_s format_preset[] = {
    {0             , ""},
    {FMT_BITMAP_RL , "bitmap_rl"},
    {FMT_BITMAP_RM , "bitmap_rm"},
    {FMT_BITMAP_CL , "bitmap_cl"},
    {FMT_BITMAP_CM , "bitmap_cm"},
    {FMT_BITMAP_RCL, "bitmap_rcl"},
    {FMT_BITMAP_RCM, "bitmap_rcm"},
    {FMT_BITMAP_CRL, "bitmap_crl"},
    {FMT_BITMAP_CRM, "bitmap_crm"},
    {FMT_WEB       , "web"},
    {FMT_RGB565    , "rgb565"},
    {FMT_BGR565    , "bgr565"},
    {FMT_ARGB1555  , "argb1555"},
    {FMT_BGRA5551  , "bgra5551"},
};

const fmt_s *aim_fmt = NULL;

// 全局变量
FILE *fpw;
FILE *fpr;

// 设置
#define ELEMENT_PER_LINE 16

int32_t invert_color = 0; // 反色
int32_t big_endian = 0; // 大端
int32_t edge = 0; // 边缘检测算法
int32_t dithering = 0; // 抖动算法
int32_t luminance = 0; // 亮度
int32_t contrast = 0; // 对比度
uint32_t transparence = 0x12345678; // 透明色

int32_t decode_height = 0; // 解码图像的高度
int32_t decode_width = 0; // 解码图像的宽度
int32_t file_offset = 0;
int32_t img_head_size = 0;
int32_t img_tail_size = 0;

char *mode_str = NULL;
char *format_str = NULL;
char *input_str = NULL;

// argparse
struct argparse argparse;

static const char *const usages[] = {
    "img_convertor.exe [options]",
    NULL,
};

struct argparse_option options[] = {
    OPT_HELP(),
    OPT_GROUP("Basic options"),
    OPT_STRING('m', "mode", &mode_str, "convert mode, enc or dec", NULL, 0, 0),
    OPT_STRING('f', "format", &format_str, "set input/output format(format specification see below)", NULL, 0, 0),

    OPT_BOOLEAN('r', "reverse", &invert_color, "invert color, only for encode and bitmap format, default FALSE", NULL, 0, 0),
    OPT_BOOLEAN('b', "bigendian", &big_endian, "big endian, only for 16bit format, e.g. rgb565, argb565, default FALSE", NULL, 0, 0),
    OPT_BOOLEAN('e', "edge", &edge, "use edge detector algorithm, only for encode and bitmap format, default FALSE", NULL, 0, 0),
    OPT_BOOLEAN('d', "dithering", &dithering, "use dithering algorithm, only for encode, default FALSE", NULL, 0, 0),
    OPT_INTEGER('l', "luminance", &luminance, "set luminance, only for encode and bitmap format, between -100 and +100, default 0", NULL, 0, 0),
    OPT_INTEGER('c', "contrast", &contrast, "set contrast, only for encode and bitmap format, between -100 and +100, default 0", NULL, 0, 0),
    OPT_INTEGER('t', "transparence", &transparence, "set a color as transparent color, only for encode and argb1555, bgra5551 format", NULL, 0, 0),

    OPT_INTEGER('W', "width", &decode_width, "set image width, only for decode", NULL, 0, 0),
    OPT_INTEGER('H', "height", &decode_height, "set image height, only for decode", NULL, 0, 0),
    OPT_INTEGER('s', "shift", &file_offset, "file offset, only for decode", NULL, 0, 0),
    OPT_INTEGER('H', "head", &img_head_size, "image head size, only for decode", NULL, 0, 0),
    OPT_INTEGER('T', "tail", &img_tail_size, "image tail size, only for decode", NULL, 0, 0),

    OPT_STRING('i', "input", &input_str, "set input file", NULL, 0, 0),
    OPT_END(),
};

static void change_ext_name(char* filename, char *ext_name)
{
    char *separator = NULL;
    int32_t ext_len = strlen(ext_name);
    int32_t name_len = strlen(filename);
    
    separator = strrchr(filename, '.');
    if (separator == NULL ||
        separator - filename <= 1 // 排除 .\ ..\ 之类的相对路径
        )
    {
        separator = filename + name_len;
        *separator = '.';
    }
    separator += 1;
    strcpy(separator, ext_name);
    separator += ext_len;
    *separator = 0;
}


// 二进制数据转C数组
int32_t bin2array_start(FILE **fp, char *filename, char *arr_name)
{
    *fp = fopen(filename, "w");
    if(*fp == NULL)
    {
        printf("open file (%s) error \n", filename);
        return 1;
    }

    fprintf(*fp, "unsigned char %s[] = {\n", arr_name);
    return 0;
}

// in 输入数据数组
// in_len 数组长度
// size 数组元素大小
int32_t bin2array_convert(FILE **fp, void *in, int in_len, int size)
{
    int i = 0;
    int rest = 0;
    int count = 0;
    int line = 0;
    int line_count = 0;
    char fmt_str1[16];
    char fmt_str2[16];
    unsigned char *ptr_u8 = (unsigned char *)in;
    // unsigned short *ptr_u16 = (unsigned short *)in;
    // unsigned int *ptr_u32 = (unsigned int *)in;

    line_count = (in_len + ELEMENT_PER_LINE - 1) / ELEMENT_PER_LINE;

    switch (size)
    {
    case 1:
        strcpy(fmt_str1, "0x%02X, ");
        strcpy(fmt_str2, "0x%02X,\n");
        break;
    // case 2:
    //     strcpy(fmt_str1, "0x%04X, ");
    //     strcpy(fmt_str2, "0x%04X,\n");
    //     break;
    // case 4:
    //     strcpy(fmt_str1, "0x%08X, ");
    //     strcpy(fmt_str2, "0x%08X,\n");
    //     break;
    
    default:
        printf("element size(%d) is invalid\n", size);
        return 1;
    }

    count = 0;
    for (line = 0; line < line_count; line++)
    {
        if (in_len - count < ELEMENT_PER_LINE)
        {
            rest = in_len % ELEMENT_PER_LINE - 1;
        }
        else
        {
            rest = ELEMENT_PER_LINE - 1;
        }

        fprintf(*fp, "    ");
        for (i = 0; i < rest; i++)
        {
            fprintf(*fp, fmt_str1, ptr_u8[count]);
            count += 1;
        }
        fprintf(*fp, fmt_str2, ptr_u8[count]);
        count += 1;
    }

    return 0;
}

void bin2array_end(FILE **fp)
{
    fprintf(*fp, "};\n");
    fclose(*fp);
}

int32_t rgb888_dump_ppm(char *path, uint8_t *data, int32_t width, int32_t height)
{
    FILE *fp;
    char ppm_head[128];
    int32_t ppm_head_len;

    fp = fopen(path, "wb");
    if(fp == NULL)
    {
        printf("save file %s error \n", path);
        return 1;
    }

    memset(ppm_head, 0, sizeof(ppm_head));
    sprintf(ppm_head, "P6 %d %d 255 ", width, height);
    ppm_head_len = strlen(ppm_head);
    fwrite(ppm_head, 1, ppm_head_len, fp);
    fwrite(data, 1, height * width * 3, fp);
    fclose(fp);
    return 0;
}

int main(int argc, const char **argv)
{
    char tmp_name[512];
    int32_t i = 0;
    int32_t ret = 0;
    int32_t out_size = 0;
    int32_t out_width = 0;
    int32_t out_height = 0;
    uint8_t *out_data;
    img_enc_ctx *enc_ctx = NULL;
    img_dec_ctx *dec_ctx = NULL;

    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, NULL, SPECIFICATION);
    argparse_parse(&argparse, argc, argv);

    if (input_str == NULL ||
        strlen(input_str) > sizeof(tmp_name) - 32 ||
        strrchr(input_str, '.') == NULL)
    {
        printf("input file name error(%s)\n", input_str);
        return 1;
    }

    if (format_str == NULL ||
        mode_str == NULL)
    {
        argparse_usage(&argparse);
        return 1;
    }
    
    for (i = 0; i < FMT_INVALID; i++)
    {
        if (strcmp(format_preset[i].fmt_str, format_str) == 0)
        {
            aim_fmt = &format_preset[i];
            break;
        }
    }
    if (aim_fmt == NULL)
    {
        printf("unknown format(%s)\n", format_str);
        return 1;
    }

    if (strcmp(mode_str, "enc") == 0)
    {
        enc_ctx = img_enc_open(input_str);
        if (enc_ctx == NULL)
        {
            printf("open file %s error\n", input_str);
            return 1;
        }

        img_enc_param enc_param = {
            .format = aim_fmt->fmt,
            .is_big_endian = big_endian,
            .is_invert = invert_color,
            .use_edge_detector = edge,
            .use_dithering_algorithm = dithering,
            .luminance = luminance,
            .contrast = contrast,
            .transparence = transparence,
        };
        ret = img_enc_cfg(enc_ctx, &enc_param);
        if (ret)
        {
            img_enc_close(enc_ctx);
            printf("set enc param error, code %d\n", ret);
            return 1;
        }

        ret = img_enc_get_size(enc_ctx, &out_size, &out_width, &out_height);
        if (ret)
        {
            img_enc_close(enc_ctx);
            printf("get enc size error, code %d\n", ret);
            return 1;
        }

        out_data = (uint8_t *)malloc(out_size);
        ret = img_enc(enc_ctx, out_data, out_size);
        if (ret)
        {
            SAFE_FREE(out_data);
            img_enc_close(enc_ctx);
            printf("enc error, code %d\n", ret);
            return 1;
        }
        img_enc_close(enc_ctx);

        strcpy(tmp_name, input_str);
        change_ext_name(tmp_name, "bin");
        fpw = fopen(tmp_name, "wb");
        if(fpw == NULL)
        {
            printf("save file %s error\n", tmp_name);
        }
        else
        {
            fwrite(out_data, 1, out_size, fpw);
            fclose(fpw);
            printf("enc finish, save file in %s\n", tmp_name);
        }

        change_ext_name(tmp_name, "c");
        if (bin2array_start(&fpw, tmp_name, "img"))
        {
            printf("save file %s error\n", tmp_name);
        }
        else
        {
            bin2array_convert(&fpw, out_data, out_size, sizeof(unsigned char));
            bin2array_end(&fpw);
            printf("enc finish, save file in %s\n", tmp_name);
        }

        SAFE_FREE(out_data);
    }
    else if (strcmp(mode_str, "dec") == 0)
    {
        dec_ctx = img_dec_open(input_str);
        if (dec_ctx == NULL)
        {
            printf("open file %s error\n", input_str);
            return 1;
        }

        img_dec_param dec_param = {
            .format = aim_fmt->fmt,
            .width = decode_width,
            .height = decode_height,
            .is_big_endian = big_endian,
            .file_offset = file_offset,
            .img_head_size = img_head_size,
            .img_tail_size = img_tail_size,
        };
        ret = img_dec_cfg(dec_ctx, &dec_param);
        if (ret)
        {
            img_dec_close(dec_ctx);
            printf("set dec param error, code %d\n", ret);
            return 1;
        }

        int32_t dec_count = img_dec_get_num(dec_ctx);

        char name_with_count[512] = {0};
        char *separator = NULL;
        strcpy(name_with_count, input_str);
        separator = strrchr(name_with_count, '.');

        out_size = decode_width * decode_height * 3;
        out_data = (uint8_t *)malloc(out_size);
        for (i = 0; i < dec_count; i++)
        {
            img_dec_seek(dec_ctx, SEEK_GOTO, i);
            ret = img_dec(dec_ctx, out_data, out_size);
            if (ret)
            {
                SAFE_FREE(out_data);
                img_dec_close(dec_ctx);
                printf("dec error, code %d\n", ret);
                return 1;
            }

            sprintf(separator, "_%05d", i);
            change_ext_name(name_with_count, "ppm");
            if(rgb888_dump_ppm(name_with_count, out_data, decode_width, decode_height) == 0)
            {
                printf("dec finish, save file in %s\n", name_with_count);
            }
        }
    }
    else
    {
        printf("please set convert mode\n");
        return 1;
    }

    return 0;
}
