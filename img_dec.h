/**
 * @file img_dec.c
 * @author dma
 * @brief 其他格式图像解码为rgb888
 * @note 支持图片序列解码等功能
 * 
 * @version 0.2
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __IMG_DEC_H
#define __IMG_DEC_H

#include <stdint.h>
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
    fmt_e format; // 图像格式
    int32_t width; // 图片水平方向长度
    int32_t height; // 图片垂直方向长度
    int32_t is_big_endian; // 是否为大端格式，仅对rgb565等16位图像有效
    int32_t file_offset; // 文件开头的偏移量
    int32_t img_head_size; // 单个图片的头部大小
    int32_t img_tail_size; // 单个图片的尾部大小
} img_dec_param;

typedef void img_dec_ctx;

/**
 * @brief 打开图片解码器
 * 
 * @param path 待解码的文件路径
 * @return img_dec_ctx* 解码器指针
 */
img_dec_ctx *img_dec_open(char *path);

/**
 * @brief 关闭图片解码器
 * 
 * @param img 已打开的解码器
 * @return img_err_code 错误码
 */
img_err_code img_dec_close(img_dec_ctx *img);

/**
 * @brief 设置图片解码参数
 * 
 * @param img 已打开的解码器
 * @param param 解码参数
 * @return img_err_code 错误码
 */
img_err_code img_dec_cfg(img_dec_ctx *img, img_dec_param *param);

/**
 * @brief 获取单张图片数据的大小，即 img_head_size + 图像数据大小 + img_tail_size
 * 
 * @param img 已打开的解码器
 * @return int32_t 图片数据的大小
 */
int32_t img_dec_get_size(img_dec_ctx *img);

/**
 * @brief 获取当前文件内的图片数量
 * 
 * @param img 已打开的解码器
 * @return int32_t 图片的数量
 */
int32_t img_dec_get_num(img_dec_ctx *img);

/**
 * @brief 跳转至当前文件内的某张图片
 * 
 * @param img 已打开的解码器
 * @param seek 参见 img_seek_e
 * @param to 仅 SEEK_GOTO 使用，跳转至指定图片，序号从0开始
 * @return img_err_code 错误码
 */
img_err_code img_dec_seek(img_dec_ctx *img, img_seek_e seek, int32_t to);

/**
 * @brief 获取当前图片序号
 * 
 * @param img 已打开的解码器
 * @return int32_t 当前图片序号
 */
int32_t img_dec_tell(img_dec_ctx *img);

/**
 * @brief 解码图片
 * 
 * @param img 已打开的解码器
 * @param data 保存输出数据的缓存，数据格式为RGB888
 * @param len 输出缓存的大小，不小于 width * height * 3
 * @return img_err_code 错误码
 */
img_err_code img_dec(img_dec_ctx *img, void *data, int32_t len);

#endif
