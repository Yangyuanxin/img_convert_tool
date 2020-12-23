/**
 * @file main.c
 * @author dma
 * @brief BMP图片批量转像素图
 * @note
 * TODO
 * 支持设置输入、输出文件路径
 * 支持C语言数组输出
 * 
 * @version 0.1
 * @date 2020-12-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include "argparse.h"

//位图文件头
typedef struct __attribute__((packed)) BMPFILEHEAD
{
	unsigned short bfType;//文件类型，必须是0x424D，也就是字符BM
	unsigned int bfSize;//文件大小，包含头
	unsigned short bfReserved1;//保留字
	unsigned short bfReserved2;//保留字
	unsigned int bfOffBits;//文件头到实际的图像数据的偏移字节数
}BMPFILEHEAD;

//位图信息头
typedef struct __attribute__((packed)) BMPINFOHEAD
{
	unsigned int biSize;//这个结构体的长度，为40字节
	int biWidth;//图像的宽度
	int biHeight;//图像的长度
	unsigned short biPlanes;//必须是1
	unsigned short biBitCount;//表示颜色时要用到的位数，常用的值为 1（黑白二色图）,4（16 色图）,8（256 色）,24（真彩色图）（新的.bmp 格式支持 32 位色，这里不做讨论）
	unsigned int biCompression;//指定位图是否压缩，有效的值为 BI_RGB，BI_RLE8，BI_RLE4，BI_BITFIELDS（都是一些Windows定义好的常量，暂时只考虑BI_RGB不压缩的情况）
	unsigned int biSizeImage;//指定实际的位图数据占用的字节数
	int biXPelsPerMeter;//指定目标设备的水平分辨率
	int biYPelsPerMeter;//指定目标设备的垂直分辨率
	unsigned int biClrUsed;//指定本图象实际用到的颜色数，如果该值为零，则用到的颜色数为 2 的 biBitCount 次方
	unsigned int biClrImportant;//指定本图象中重要的颜色数，如果该值为零，则认为所有的颜色都是重要的
}BMPINFOHEAD;

//调色板
//typedef struct RGBQUAD
//{
//	unsigned char rgbBlue;//该颜色的蓝色分量
//	unsigned char rgbGreen;//该颜色的绿色分量
//	unsigned char rgbRed;//该颜色的红色分量
//	unsigned char rgbReserved;//保留值
//};


//全局变量
FILE *rfp;
FILE *wfp;
BMPFILEHEAD BFH;
BMPINFOHEAD BIH;

unsigned char *bmp_data = NULL;//彩色BMP文件数据指针
unsigned char *bit_data = NULL;//黑白图像文件数据指针
uint32_t bit_data_size = 0;


//设置
int32_t invert_color = 0; // 反色
int32_t luminance = 128; // 亮度


//argparse
struct argparse argparse;

static const char *const usages[] = {
	"main.exe [options]",
	NULL,
};

struct argparse_option options[] = {
	OPT_HELP(),
	OPT_GROUP("Basic options"),
	OPT_BOOLEAN('i', "invert", &invert_color, "invert color,  default FALSE", NULL, 0, 0),
	OPT_INTEGER('l', "luminance", &luminance, "set luminance, default 128", NULL, 0, 0),
	OPT_END(),
};


//函数
int ReadBMPHead(FILE *fp);
unsigned int ReadPoint(int x, int y);
void DrawBITPoint(unsigned int x, unsigned int y);
void CleanBITPoint(unsigned int x, unsigned int y);
unsigned char ReadBITPoint(unsigned int x, unsigned int y);
void BMP2BIT();//24色位图转黑白图


int main(int argc, const char **argv)
{
	char filename[512];
	int i;
	int ret = 0;

	argparse_init(&argparse, options, usages, 0);
	argparse_parse(&argparse, argc, argv);

	if(luminance < 0 || luminance > 255)
	{
		printf("luminance is out of range! \n");
		return 0;
	}
	luminance *= 3; // 方便计算

	rfp = fopen(".\\img\\0000.bmp", "rb");
	if(rfp == NULL)
	{
		printf("open file \".\\img\\0000.bmp\" error \n");
		system("pause");
		return 0;
	}
	ret = ReadBMPHead(rfp);
	if(ret)
	{
		return 0;
	}
	fclose(rfp);

	bmp_data = (unsigned char *)malloc(BIH.biSizeImage);
	
	// 以图像高度为准，例如15*8像素的图片，需要15 * (8 / 8)字节内存，像素的图片需要15 * (9 / 8)向上取整 = 15 * 2字节内存
	bit_data_size = BIH.biWidth * ((BIH.biHeight - 1) / 8 + 1);
	bit_data = (unsigned char *)malloc(bit_data_size);

	if((wfp = fopen(".\\img.bin", "wb")) == NULL)
	{
		printf("创建文件出错！\n");
		system("pause");
		return 0;
	}

	printf("\n\n");
	i = 0;
	while (1)
	{
		memset(filename, 0, 512);
		sprintf(filename, ".\\img\\%04d.bmp", i);

		if((rfp = fopen(filename, "rb")) == NULL)
		{
			printf("\n打开文件 %s 出错或全部文件已处理完成\n", filename);
			system("pause");
			break;
			//continue;
		}
		printf("\r正在处理文件： %s ", filename);
		fflush(stdout);

		fseek(rfp, BFH.bfOffBits, SEEK_SET);
		fread(bmp_data, BIH.biSizeImage, 1, rfp);
		fclose(rfp);

		BMP2BIT();
		fwrite(bit_data, bit_data_size, 1, wfp);

		i++;
	}

	fclose(wfp);

	free(bmp_data);
	bmp_data = NULL;
	free(bit_data);
	bit_data = NULL;

	return 0;
}

int ReadBMPHead(FILE *fp)
{
	fseek(fp, 0, SEEK_SET);

	fread(&BFH, sizeof(BFH), 1, fp); //读取BMP文件头
	fread(&BIH, sizeof(BIH), 1, fp);//读取BMP信息头，40字节，直接用结构体读

	printf("\nBMP文件头\n");
	printf("bfType = %x\n", BFH.bfType);
	printf("bfSize = %d\n", BFH.bfSize);
	printf("bfReserved1 = %d\n", BFH.bfReserved1);
	printf("bfReserved2 = %d\n", BFH.bfReserved2);
	printf("bfOffBits = %d\n", BFH.bfOffBits);

	printf("\nBMP信息头\n");
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
	
	
//	if((BFH.bfType != 0x424D) || (BIH.biClrImportant != 0))
	if((BFH.bfType != 0x4D42))
	{
		printf("\n不是BMP文件！\n");
		return 1;
	}

	if (BIH.biBitCount != 24 || ((BIH.biClrImportant != 0) && (BIH.biClrImportant != 16777216)))
	{
		printf("\n不是24位BMP文件！\n");
		return 2;
	}

	return 0;
}

//坐标从0开始计算
unsigned int ReadPoint(int x, int y)
{
	unsigned int pos = 0;
	unsigned int line_size = 0;
	unsigned int RGB = 0;
	//unsigned int BGR = 0;

	if (bmp_data == NULL)
	{
		// printf("无图像数据\n");
		return 1;
	}
	
	if (x >= BIH.biWidth || y >= BIH.biHeight)
	{
		//printf("读取的像素点超出图像范围\n");
		return 1;
	}

	//a=14,b=5
	//普通取整a/b=2
	//进一取整(a+b-1)/b=3

	// (((width*biBitCount+7)/8+3)/4*4
	// (((width*biBitCount)+31)/32)*4
	// (((width*biBitCount)+31)>>5)<<2
	line_size = (((BIH.biWidth * BIH.biBitCount)+31)>>5)<<2;
	pos = (BIH.biHeight - y - 1) * line_size + x * 3;

	//BGR |= bmp_data[pos] << 16;
	//BGR |= bmp_data[pos + 1] << 8;
	//BGR |= bmp_data[pos + 2];

	RGB = *(unsigned int *)(&bmp_data[pos]) & 0x00FFFFFF;

	return RGB;
}

inline void DrawBITPoint(unsigned int x, unsigned int y)
{
	//避免超出屏幕范围
	//if(x < BIH.biWidth && y < BIH.biHeight)
	{
		//bit_data[x + BIH.biWidth * (y >> 3)] = bit_data[x + BIH.biWidth * (y >> 3)] | (0x80 >> (y & 0x07));//MSB
		bit_data[x + BIH.biWidth * (y >> 3)] = bit_data[x + BIH.biWidth * (y >> 3)] | (0x01 << (y & 0x07));//LSB
	}
}

inline void CleanBITPoint(unsigned int x, unsigned int y)
{
	//避免超出屏幕范围
	//if(x < BIH.biWidth && y < BIH.biHeight)
	{
		//bit_data[x + BIH.biWidth * (y >> 3)] = bit_data[x + BIH.biWidth * (y >> 3)] & (~(0x80 >> (y & 0x07)));//MSB
		bit_data[x + BIH.biWidth * (y >> 3)] = bit_data[x + BIH.biWidth * (y >> 3)] & (~(0x01 << (y & 0x07)));//LSB
	}
}

inline unsigned char ReadBITPoint(unsigned int x, unsigned int y)
{
	//避免超出屏幕范围
	//if(x < BIH.biWidth && y < BIH.biHeight)
	{
		//读取像素
		//if(bit_data[x + BIH.biWidth * (y >> 3)] & (0x80 >> (y & 0x07)))//MSB
		if(bit_data[x + BIH.biWidth * (y >> 3)] & (0x01 << (y & 0x07)))//LSB
		{
			return 1;
		}
	}
	return 0;
}

//彩图转黑白
void BMP2BIT()
{
	int x, y;
	uint32_t RGB;
	
	memset(bit_data, 0, bit_data_size);
	for (y=0; y<BIH.biHeight; y++)
	{
		for (x=0; x<BIH.biWidth; x++)
		{
			RGB = ReadPoint(x,y);
			if (((RGB >> 16) & 0x0000FF) + ((RGB >> 8) & 0x0000FF) + (RGB & 0x0000FF) < luminance)
			{
				invert_color ? CleanBITPoint(x, y) : DrawBITPoint(x, y);
			}
			else
			{
				invert_color ? DrawBITPoint(x, y) : CleanBITPoint(x, y);
			}
		}
	}
}
