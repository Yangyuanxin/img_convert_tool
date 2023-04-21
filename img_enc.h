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

#ifndef __IMG_ENC_H
#define __IMG_ENC_H

#include <stdint.h>
#include "img_common.h"

typedef struct
{
    fmt_e format; // 输出图像格式
    int32_t is_big_endian; // 是否为大端格式，仅对rgb565等16位图像有效
    int32_t is_invert; // 是否反色，仅对bitmap格式有效
    int32_t use_edge_detector; // 是否使用边缘检测，仅对bitmap格式有效
    int32_t use_dithering_algorithm; // 是否使用颜色抖动，对所有图像格式有效
    int32_t luminance; // 调整亮度，默认值0，取值范围+-100，仅对bitmap格式有效
    int32_t contrast; // 调整对比度，默认值0，取值范围+-100，仅对bitmap格式有效
    uint32_t transparence; // 透明色，格式为0x00RRGGBB，仅对argb和bgra格式有效
} img_enc_param;

typedef void img_enc_ctx;

/**
 * @brief 打开图片编码器
 * 
 * @param path 需要编码的图片
 * @return img_enc_ctx* 编码器指针
 */
img_enc_ctx *img_enc_open(char *path);

/**
 * @brief 关闭图片编码器
 * 
 * @param img 编码器指针
 * @return img_err_code 错误码
 */
img_err_code img_enc_close(img_enc_ctx *img);

/**
 * @brief 设置编码器参数
 * 
 * @param img 编码器指针
 * @param param 参数指针
 * @return img_err_code 错误码
 */
img_err_code img_enc_cfg(img_enc_ctx *img, img_enc_param *param);

/**
 * @brief 获取预览图像的大小
 * 
 * @param img 编码器指针
 * @param size 预览图像的大小
 * @param width 预览图像的宽度
 * @param height 预览图像的高度
 * @return img_err_code 错误码
 */
img_err_code img_enc_get_preview_size(img_enc_ctx *img, int32_t *size, int32_t *width, int32_t *height);

/**
 * @brief 获取预览图片
 * 
 * @param img 编码器指针
 * @param data 保存预览图片数据的内存地址，格式为RGB888
 * @param len data所指向的内存区域大小，不得小于 img_enc_get_preview_size 获取的大小
 * @return img_err_code 错误码
 */
img_err_code img_enc_preview(img_enc_ctx *img, void *data, int32_t len);

/**
 * @brief 获取图像的大小
 * 
 * @param img 编码器指针
 * @param size 图像的大小
 * @param width 图像的宽度
 * @param height 图像的高度
 * @return img_err_code 错误码
 */
img_err_code img_enc_get_size(img_enc_ctx *img, int32_t *size, int32_t *width, int32_t *height);

/**
 * @brief 获取输出图像
 * 
 * @param img 编码器指针
 * @param data 保存输出图片数据的内存地址，格式由 img_enc_cfg 设置
 * @param len data所指向的内存区域大小，不得小于 img_enc_get_size 获取的大小
 * @return img_err_code 错误码
 */
img_err_code img_enc(img_enc_ctx *img, void *data, int32_t len);

#endif
