/**
 * @file main.c
 * @author dma
 * @brief BMP图片批量转像素图
 * @note
 * TODO
 * 参考 PCtoLCD2002 支持更多取模方式
 * 
 * @version 0.2
 * @date 2021-07-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "argparse.h"

// 位图文件头
typedef struct __attribute__((packed)) BMP_FILE_HEAD
{
    unsigned short bfType; // 文件类型，必须是0x424D，也就是字符BM
    unsigned int bfSize; // 文件大小，包含头
    unsigned short bfReserved1; // 保留字
    unsigned short bfReserved2; // 保留字
    unsigned int bfOffBits; // 文件头到实际的图像数据的偏移字节数
}BMP_FILE_HEAD;

// 位图信息头
typedef struct __attribute__((packed)) BMP_INFO_HEAD
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
}BMP_INFO_HEAD;

// 调色板
// typedef struct RGBQUAD
// {
//     unsigned char rgbBlue; // 该颜色的蓝色分量
//     unsigned char rgbGreen; // 该颜色的绿色分量
//     unsigned char rgbRed; // 该颜色的红色分量
//     unsigned char rgbReserved; // 保留值
// };

const uint32_t web_color[216] = {
    0x000000, 0x000033, 0x000066, 0x000099, 0x0000CC, 0x0000FF,
    0x003300, 0x003333, 0x003366, 0x003399, 0x0033CC, 0x0033FF,
    0x006600, 0x006633, 0x006666, 0x006699, 0x0066CC, 0x0066FF,
    0x009900, 0x009933, 0x009966, 0x009999, 0x0099CC, 0x0099FF,
    0x00CC00, 0x00CC33, 0x00CC66, 0x00CC99, 0x00CCCC, 0x00CCFF,
    0x00FF00, 0x00FF33, 0x00FF66, 0x00FF99, 0x00FFCC, 0x00FFFF,
    0x330000, 0x330033, 0x330066, 0x330099, 0x3300CC, 0x3300FF,
    0x333300, 0x333333, 0x333366, 0x333399, 0x3333CC, 0x3333FF,
    0x336600, 0x336633, 0x336666, 0x336699, 0x3366CC, 0x3366FF,
    0x339900, 0x339933, 0x339966, 0x339999, 0x3399CC, 0x3399FF,
    0x33CC00, 0x33CC33, 0x33CC66, 0x33CC99, 0x33CCCC, 0x33CCFF,
    0x33FF00, 0x33FF33, 0x33FF66, 0x33FF99, 0x33FFCC, 0x33FFFF,
    0x660000, 0x660033, 0x660066, 0x660099, 0x6600CC, 0x6600FF,
    0x663300, 0x663333, 0x663366, 0x663399, 0x6633CC, 0x6633FF,
    0x666600, 0x666633, 0x666666, 0x666699, 0x6666CC, 0x6666FF,
    0x669900, 0x669933, 0x669966, 0x669999, 0x6699CC, 0x6699FF,
    0x66CC00, 0x66CC33, 0x66CC66, 0x66CC99, 0x66CCCC, 0x66CCFF,
    0x66FF00, 0x66FF33, 0x66FF66, 0x66FF99, 0x66FFCC, 0x66FFFF,
    0x990000, 0x990033, 0x990066, 0x990099, 0x9900CC, 0x9900FF,
    0x993300, 0x993333, 0x993366, 0x993399, 0x9933CC, 0x9933FF,
    0x996600, 0x996633, 0x996666, 0x996699, 0x9966CC, 0x9966FF,
    0x999900, 0x999933, 0x999966, 0x999999, 0x9999CC, 0x9999FF,
    0x99CC00, 0x99CC33, 0x99CC66, 0x99CC99, 0x99CCCC, 0x99CCFF,
    0x99FF00, 0x99FF33, 0x99FF66, 0x99FF99, 0x99FFCC, 0x99FFFF,
    0xCC0000, 0xCC0033, 0xCC0066, 0xCC0099, 0xCC00CC, 0xCC00FF,
    0xCC3300, 0xCC3333, 0xCC3366, 0xCC3399, 0xCC33CC, 0xCC33FF,
    0xCC6600, 0xCC6633, 0xCC6666, 0xCC6699, 0xCC66CC, 0xCC66FF,
    0xCC9900, 0xCC9933, 0xCC9966, 0xCC9999, 0xCC99CC, 0xCC99FF,
    0xCCCC00, 0xCCCC33, 0xCCCC66, 0xCCCC99, 0xCCCCCC, 0xCCCCFF,
    0xCCFF00, 0xCCFF33, 0xCCFF66, 0xCCFF99, 0xCCFFCC, 0xCCFFFF,
    0xFF0000, 0xFF0033, 0xFF0066, 0xFF0099, 0xFF00CC, 0xFF00FF,
    0xFF3300, 0xFF3333, 0xFF3366, 0xFF3399, 0xFF33CC, 0xFF33FF,
    0xFF6600, 0xFF6633, 0xFF6666, 0xFF6699, 0xFF66CC, 0xFF66FF,
    0xFF9900, 0xFF9933, 0xFF9966, 0xFF9999, 0xFF99CC, 0xFF99FF,
    0xFFCC00, 0xFFCC33, 0xFFCC66, 0xFFCC99, 0xFFCCCC, 0xFFCCFF,
    0xFFFF00, 0xFFFF33, 0xFFFF66, 0xFFFF99, 0xFFFFCC, 0xFFFFFF,
};

// 全局变量
FILE *fpr;
FILE *fpwb; // 输出二进制数据
FILE *fpwc; // 输出C数组
BMP_FILE_HEAD BFH;
BMP_INFO_HEAD BIH;

unsigned char *bmp_data = NULL;
unsigned char *rgb888_data = NULL;
unsigned char *out_data = NULL;
uint32_t out_data_size = 0;

// 函数声明
int bmp_read_head(FILE *fp);
void bmp_to_rgb888(uint8_t *in, uint8_t *out, int h, int v);
void rgb888_to_bitmap(uint8_t *in, uint8_t *out, int h, int v);
void rgb888_to_web(uint8_t *in, uint8_t *out, int h, int v);
void rgb888_to_rgb565(uint8_t *in, uint8_t *out, int h, int v);
void rgb888_to_bgr565(uint8_t *in, uint8_t *out, int h, int v);

void bin2array_start(FILE **fp, char *name);
void bin2array_convert(FILE **fp, void *in, int in_len, int size);
void bin2array_end(FILE **fp);

int convert(char *filename);
void dbg_rgb888_dump_bmp(char *path, uint8_t *data, int32_t width, int32_t height);

// 位图图像格式
// v=vertical,h=horizontal,l=lsb,m=msb
typedef enum {
    BITMAP_MODE_VL = 0,
    BITMAP_MODE_VM = 1,
    BITMAP_MODE_HL = 2,
    BITMAP_MODE_HM = 3,
    BITMAP_MODE_INVALID,
} bitmap_mode_e;

// 支持的格式
typedef enum {
    FMT_BITMAP = 0,
    FMT_WEB    = 1,
    FMT_RGB565 = 2,
    FMT_BGR565 = 3,
    // FMT_ARGB1555 = 0,
    FMT_INVALID,
} fmt_e;

typedef struct {
    fmt_e fmt;
    char fmt_str[12];
    void(*color_convert)(uint8_t *in, uint8_t *out, int h, int v);
} fmt_s;

const fmt_s format_preset[] = {
    {FMT_BITMAP, "bitmap", rgb888_to_bitmap},
    {FMT_WEB   , "web",    rgb888_to_web},
    {FMT_RGB565, "rgb565", rgb888_to_rgb565},
    {FMT_BGR565, "bgr565", rgb888_to_bgr565},
    // {FMT_ARGB1555, "argb1555"},
};
const fmt_s *aim_fmt = NULL;

// 设置
#define ELEMENT_PER_LINE 16
int32_t bitmap_mode = 0; // 反色
int32_t reverse_color = 0; // 反色
int32_t luminance = 128; // 亮度
char *format_str = NULL;
char *input_str = NULL;

// argparse
struct argparse argparse;

static const char *const usages[] = {
    "main.exe [options]",
    NULL,
};

struct argparse_option options[] = {
    OPT_HELP(),
    OPT_GROUP("Basic options"),
    OPT_INTEGER('m', "mode", &bitmap_mode, "bitmap mode, 0=vertical-LSB, 1=vertical-MSB, 2=horizontal-LSB, 3=horizontal-MSB, default 0", NULL, 0, 0),
    OPT_BOOLEAN('r', "reverse", &reverse_color, "reverse color, only for bitmap, default FALSE", NULL, 0, 0),
    OPT_INTEGER('l', "luminance", &luminance, "set luminance, only for bitmap, default 128", NULL, 0, 0),
    OPT_STRING('f', "format", &format_str, "set output format", NULL, 0, 0),
    OPT_STRING('i', "input", &input_str, "set one input file", NULL, 0, 0),
    OPT_END(),
};

int main(int argc, const char **argv)
{
    char filename[512];
    int i;
    int ret = 0;

    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "\ncontvert bmp to other format", "\noutput fotmat: bitmap, rgb565\n");
    argparse_parse(&argparse, argc, argv);

    if (luminance < 0 || luminance > 255)
    {
        printf("luminance is out of range! \n");
        return 0;
    }
    luminance *= 3; // 方便计算

    if (format_str == NULL)
    {
        printf("please set output format\n");
        return 0;
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
        return 0;
    }

	if (input_str == NULL)
	{
		strcpy(filename, ".\\img\\0000.bmp");
	}
	else
	{
		strcpy(filename, input_str);
	}

    fpr = fopen(filename, "rb");
    if(fpr == NULL)
    {
        printf("open file %s error \n", filename);
        return 0;
    }
    ret = bmp_read_head(fpr);
    if(ret)
    {
        return 0;
    }
    fclose(fpr);

    if (aim_fmt->fmt == FMT_BITMAP)
    {
        // 宽度或高度需要向上对8取整，例如15*9像素的图片，横向需要(15 / 8) * 9 = 18字节内存，纵向需要 15 * (9 / 8) =30 字节内存
        switch (bitmap_mode)
        {
            case BITMAP_MODE_VL:
            case BITMAP_MODE_VM:
                out_data_size = BIH.biWidth * ((BIH.biHeight + 7) >> 3);
                break;
            case BITMAP_MODE_HL:
            case BITMAP_MODE_HM:
                out_data_size = BIH.biHeight * ((BIH.biWidth + 7) >> 3);
                break;
            default:
                printf("known bitmap mode(%d)\n", bitmap_mode);
                return 0;
                break;
        }
    }
    else if (aim_fmt->fmt == FMT_WEB)
    {
        out_data_size = BIH.biWidth * BIH.biHeight;
    }
    else if (aim_fmt->fmt == FMT_RGB565 || aim_fmt->fmt == FMT_BGR565)
    {
        out_data_size = BIH.biWidth * BIH.biHeight * 2;
    }
    out_data = (unsigned char *)malloc(out_data_size);
    bmp_data = (unsigned char *)malloc(BIH.biSizeImage);
    rgb888_data = (unsigned char *)malloc(BIH.biWidth * BIH.biHeight * 3);

    if((fpwb = fopen(".\\img.bin", "wb")) == NULL)
    {
        printf("write file \".\\img.bin\" error \n");
        return 0;
    }

    printf("\n\n");
    bin2array_start(&fpwc, "img_data");

	if (input_str == NULL)
	{
		i = 0;
		while (1)
		{
			memset(filename, 0, 512);
			sprintf(filename, ".\\img\\%04d.bmp", i);
			if (convert(filename))
			{
				break;
			}
			i++;
		}
	}
	else
	{
		strcpy(filename, input_str);
		convert(filename);
	}

    fclose(fpwb);
    bin2array_end(&fpwc);

    free(bmp_data);
    bmp_data = NULL;
    free(rgb888_data);
    rgb888_data = NULL;
    free(out_data);
    out_data = NULL;

    return 0;
}

int convert(char *filename)
{
	if((fpr = fopen(filename, "rb")) == NULL)
	{
		printf("\nopen %s error, or convert finish\n", filename);
		return 1;
	}
	printf("\rconverting file %s ", filename);
	fflush(stdout);

	fseek(fpr, BFH.bfOffBits, SEEK_SET);
	fread(bmp_data, BIH.biSizeImage, 1, fpr);
	fclose(fpr);

	bmp_to_rgb888(bmp_data, rgb888_data, BIH.biWidth, BIH.biHeight);
	memset(out_data, 0, out_data_size);
	aim_fmt->color_convert(rgb888_data, out_data, BIH.biWidth, BIH.biHeight);

	fwrite(out_data, 1, out_data_size, fpwb);
	
	bin2array_convert(&fpwc, (void *)out_data, out_data_size, sizeof(unsigned char));

	return 0;
}

int bmp_read_head(FILE *fp)
{
    fseek(fp, 0, SEEK_SET);

    fread(&BFH, sizeof(BFH), 1, fp); // 读取BMP文件头
    fread(&BIH, sizeof(BIH), 1, fp); // 读取BMP信息头，40字节，直接用结构体读

    printf("\nBMP file head\n");
    printf("bfType = %x\n", BFH.bfType);
    printf("bfSize = %d\n", BFH.bfSize);
    printf("bfReserved1 = %d\n", BFH.bfReserved1);
    printf("bfReserved2 = %d\n", BFH.bfReserved2);
    printf("bfOffBits = %d\n", BFH.bfOffBits);

    printf("\nBMP info head\n");
    printf("biSize = %d\n", BIH.biSize);
    printf("biWidth = %d\n", BIH.biWidth);
    printf("biHeight = %d\n", BIH.biHeight);
    printf("biPlanes = %d\n", BIH.biPlanes);
    printf("biBitCount = %d\n", BIH.biBitCount);
    printf("biCompression = %d\n", BIH.biCompression);
    printf("biSizeImage = %d\n", BIH.biSizeImage);
    printf("biXPelsPerMeter = %d\n", BIH.biXPelsPerMeter);
    printf("biYPelsPerMeter = %d\n", BIH.biYPelsPerMeter);
    printf("biClrUsed = %d\n", BIH.biClrUsed);
    printf("biClrImportant = %d\n", BIH.biClrImportant);
    
    // if((BFH.bfType != 0x424D) || (BIH.biClrImportant != 0))
    if((BFH.bfType != 0x4D42))
    {
        printf("\nnot bmp file\n");
        return 1;
    }

    if (BIH.biBitCount != 24 || ((BIH.biClrImportant != 0) && (BIH.biClrImportant != 16777216)))
    {
        printf("\nnot 24 bit bmp file\n");
        return 2;
    }

    return 0;
}

#if 0
// bmp 数据翻转，对于行宽是4整数倍的图片可以替代 bmp_to_rgb888
void bmp_reversal(uint8_t *buf)
{
    int32_t i = 0, j = 0;
    int32_t h = IMG_H, v = IMG_V;
    int32_t size = 0;

    // size = h * v * 3;
    i = 0;

    // bgr转rgb
    for (i = 0; i < v; i++)
    {
        for (j = 0; j < h; j++)
        {
            buf[i * h * 3 + j * 3] ^= buf[i * h * 3 + j * 3 + 2];
            buf[i * h * 3 + j * 3 + 2] ^= buf[i * h * 3 + j * 3];
            buf[i * h * 3 + j * 3] ^= buf[i * h * 3 + j * 3 + 2];
        }
    }

    // 变换行序
    for (i = 0; i < v / 2; i++)
    {
        for (j = 0; j < h * 3; j++)
        {
            buf[i * h * 3 + j] ^= buf[(v - i - 1) * h * 3 + j];
            buf[(v - i - 1) * h * 3 + j] ^= buf[i * h * 3 + j];
            buf[i * h * 3 + j] ^= buf[(v - i - 1) * h * 3 + j];
        }
    }
}
#endif

void bmp_to_rgb888(uint8_t *in, uint8_t *out, int h, int v)
{
    int32_t i = 0, j = 0;
    unsigned int offset = 0;
    unsigned int line_size = 0;

    //a=14,b=5
    //普通取整a/b=2
    //进一取整(a+b-1)/b=3
    // (((width*biBitCount+7)/8+3)/4*4
    // (((width*biBitCount)+31)/32)*4
    // (((width*biBitCount)+31)>>5)<<2
    // ((width*3)+3)/4*4 // 24bit
    // line_size = (((h * 24) + 31) >> 5) << 2;
    line_size = ((h * 3 + 3) >> 2) << 2;

    for (i = 0; i < v; i++)
    {
        for (j = 0; j < h; j++)
        {
            offset = (v - i - 1) * line_size + j * 3;

            out[i * h * 3 + j * 3]     = in[offset + 2];
            out[i * h * 3 + j * 3 + 1] = in[offset + 1];
            out[i * h * 3 + j * 3 + 2] = in[offset];
        }
    }
}

void rgb888_to_bitmap(uint8_t *in, uint8_t *out, int h, int v)
{
    int32_t x = 0, y = 0;
    int32_t avg = 0;

    int32_t he = 0;
    // int32_t ve = 0;
    switch (bitmap_mode)
    {
        case BITMAP_MODE_VL:
        case BITMAP_MODE_VM:
            he = h;
            // ve = (v + 7) >> 3;
            break;
        case BITMAP_MODE_HL:
        case BITMAP_MODE_HM:
            he = (h + 7) >> 3;
            // ve = v;
            break;
        default:
            break;
    }

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            avg = in[y * h * 3 + x * 3] + in[y * h * 3 + x * 3 + 1] + in[y * h * 3 + x * 3 + 2];
            if ((reverse_color == 0 && avg < luminance) ||
                (reverse_color == 1 && avg > luminance))
            {
                continue;
            }

            switch (bitmap_mode)
            {
                case BITMAP_MODE_VL:
                    out[x + h * (y >> 3)] = out[x + h * (y >> 3)] | (0x01 << (y & 0x07));
                    break;
                case BITMAP_MODE_VM:
                    out[x + h * (y >> 3)] = out[x + h * (y >> 3)] | (0x80 >> (y & 0x07));
                    break;
                case BITMAP_MODE_HL:
                    out[he * y + (x >> 3)] = out[he * y + (x >> 3)] | (0x01 << (x & 0x07));
                    break;
                case BITMAP_MODE_HM:
                    out[he * y + (x >> 3)] = out[he * y + (x >> 3)] | (0x80 >> (x & 0x07));
                    break;
                default:
                    break;
            }
        }
    }
}

void rgb888_to_web(uint8_t *in, uint8_t *out, int h, int v)
{
    uint16_t *d        = (uint16_t *)out;
    const uint8_t *s   = in;
    const uint8_t *end = s + h * v;

    while (s < end) {
        uint8_t r = *s++;
        uint8_t g = *s++;
        uint8_t b = *s++;
        r = r > 229 ? 5 : (r + 26) / 51;
        g = r > 229 ? 5 : (g + 26) / 51;
        b = r > 229 ? 5 : (b + 26) / 51;
        *d++ = r * 36 + g * 6 + b;
    }
}

void rgb888_to_rgb565(uint8_t *in, uint8_t *out, int h, int v)
{
    uint16_t *d        = (uint16_t *)out;
    const uint8_t *s   = in;
    const uint8_t *end = s + h * v;

    while (s < end) {
        const int r = *s++;
        const int g = *s++;
        const int b = *s++;
        *d++        = (b >> 3) | ((g & 0xFC) << 3) | ((r & 0xF8) << 8);
    }
}

void rgb888_to_bgr565(uint8_t *in, uint8_t *out, int h, int v)
{
    uint16_t *d        = (uint16_t *)out;
    const uint8_t *s   = in;
    const uint8_t *end = s + h * v;

    while (s < end) {
        const int r = *s++;
        const int g = *s++;
        const int b = *s++;
        *d++        = (b >> 3) | ((g & 0xFC) << 3) | ((r & 0xF8) << 8);
    }
}

// 二进制数据转C数组
void bin2array_start(FILE **fp, char *name)
{
    *fp = fopen(".\\img.txt", "w");
    if(*fp == NULL)
    {
        printf("open file \".\\img.txt\" error \n");
        system("pause");
        return;
    }

    fprintf(*fp, "unsigned char %s[] = {\n", name);
}

// in 输入数据数组
// in_len 数组长度
// size 数组元素大小
void bin2array_convert(FILE **fp, void *in, int in_len, int size)
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
        return;
        // break;
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
}

void bin2array_end(FILE **fp)
{
    fprintf(*fp, "};\n");
    fclose(*fp);
}

void dbg_rgb888_dump_bmp(char *path, uint8_t *data, int32_t width, int32_t height)
{
	BMP_FILE_HEAD bfh;
    BMP_INFO_HEAD bih;
	FILE *fp;
	uint8_t line_buf[2048 * 3];
	int32_t line_size;
	int32_t i, j;

	if (width > 2048)
	{
		printf("width larger than 2048\n");
		return;
	}

	fp = fopen(path, "wb");
    if(fp == NULL)
    {
        printf("dump file %s error \n", path);
        return;
    }

	memset(&bfh, 0, sizeof(BMP_FILE_HEAD));
	memset(&bih, 0, sizeof(BMP_INFO_HEAD));
	memset(line_buf, 0, sizeof(line_buf));

    line_size = ((width * 3 + 3) >> 2) << 2;

	bfh.bfType = 0x4D42;
	bfh.bfSize = 54 + line_size * height;
	bfh.bfOffBits = 54;

	bih.biSize = 40;
	bih.biWidth = width;
	bih.biHeight = height;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = 0;
	bih.biSizeImage = line_size * height;
	bih.biXPelsPerMeter = 4724;
	bih.biYPelsPerMeter = 4724;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	fwrite(&bfh, sizeof(BMP_FILE_HEAD), 1, fp);
	fwrite(&bih, sizeof(BMP_INFO_HEAD), 1, fp);
	for(i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
        {
			line_buf[j * 3] = data[(height - i - 1) * width * 3 + j * 3 + 2];
			line_buf[j * 3 + 1] = data[(height - i - 1) * width * 3 + j * 3 + 1];
			line_buf[j * 3 + 2] = data[(height - i - 1) * width * 3 + j * 3];
        }
		fwrite(line_buf, 1, line_size, fp);
	}

	fclose(fp);
}