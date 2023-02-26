# BMP转像素图
将标准24位色BMP图片转为LCD、OLED等屏使用的像素图，支持多种输出格式，可批量转换，方便制作bad apple、开机logo等动画

# 使用方法

## 图像编码
编码功能目前为命令行方式
1. 编译`img_enc.exe`
2. 使用命令行方式运行`img_enc.exe`
3. 详细参数见下文
   
## 图像解码
解码功能带有一个基于tkinter的GUI界面，方便预览和保存解码后的图像。
导出的图像格式为ppm，可使用PS、Honeyview等软件查看
1. 编译`img_dec.dll`
2. 使用python 打开 img_dec_gui.pyw

# 编译命令
## 命令行编译
编码功能
`gcc main.c argparse.c -o img_enc.exe -Wall -O2 -g`
解码功能
`gcc -shared -o img_dec.dll .\img_common.c .\img_dec.c`

# 参数设置
* -i 转换指定单个文件，不设置则批量转换 ./img/ 目录下的文件
* -b 大端，仅对16位格式图片生效（例如RGB565等），默认不开启，即小端
* -r 反色，仅对单色像素图生效，默认不开启
* -l 亮度，仅对单色像素图生效，取值范围0-255，默认128
* -t 透明色，仅对ARGB1555和BGRA5551有效，设置一种颜色为透明色，取值范围0x000000到0xFFFFFF
* -m 单色像素图的格式
* -f 输出格式

-m 参数详细说明
| 取值  |   说明    |
| :---: | :-------: |
|   1   | 逐行，LSB |
|   2   | 逐行，MSB |
|   3   | 逐列，LSB |
|   4   | 逐列，MSB |
|   5   | 行列，LSB |
|   6   | 行列，MSB |
|   7   | 列行，LSB |
|   8   | 列行，MSB |

-f 参数详细说明
|   取值   |                        说明                         |
| :------: | :-------------------------------------------------: |
|  bitmap  |                 位图，即单色像素图                  |
|   web    |                 8位216色 web-color                  |
|  rgb565  |                         略                          |
|  bgr565  |                         略                          |
| argb1555 | 带透明的rgb格式，最高位为透明度，1为不透明，0为透明 |
| bgra5551 | 带透明的rgb格式，最低位为透明度，1为不透明，0为透明 |

示例：
单色，模式1，关闭反色，亮度设置为50 `main.exe -f bitmap -m 1 -l 50`
单色，模式3，开启反色，亮度设置为200 `main.exe -f bitmap -m 3 -i -l 200`
rgb565 `main.exe -f rgb565`

# 格式说明

## 各格式内部数据排列方式（以 20*20 的图像为例）
逐行
Byte1    Byte2    Byte3
Byte4    Byte5    Byte6
Byte7    Byte8    Byte9
...
Byte58   Byte59   Byte60

行列
Byte1    Byte21    Byte41
Byte2    Byte22    Byte42
Byte3    Byte23    Byte43
...
Byte20   Byte40    Byte60

逐列
Byte1    Byte4    Byte7  ...  Byte58
Byte2    Byte5    Byte8  ...  Byte59
Byte3    Byte6    Byte9  ...  Byte60

列行
Byte1    Byte2    Byte3   ...  Byte20
Byte21   Byte22   Byte23  ...  Byte40
Byte41   Byte42   Byte43  ...  Byte60

## 各格式下像素点与字节中比特位对应关系
LSB 逐行、行列模式
B7  B6  B5  B4  B3  B2  B1  B0
x+7 x+6 x+5 x+4 x+3 x+2 x+1 x+0

LSB 逐列、列行模式
B7  B6  B5  B4  B3  B2  B1  B0
y+7 y+6 y+5 y+4 y+3 y+2 y+1 y+0

MSB 逐行、行列模式
B7  B6  B5  B4  B3  B2  B1  B0
x+0 x+1 x+2 x+3 x+4 x+5 x+6 x+7

MSB 逐列、列行模式
B7  B6  B5  B4  B3  B2  B1  B0
y+0 y+1 y+2 y+3 y+4 y+5 y+6 y+7

# 使用限制
* 图片只支持24位位图（各种软件叫法不同，比如RGB888、24位真彩等）
* 批处理图片必须放在当前目录下的 img 目录中
* 批处理图片必须从 0000.bmp 开始命名直到结束，序号长度必须是4位，不足4位用0填充
* 批处理图片长宽必须一致
* 生成的bin文件在当前目录下，名为 img.bin
* 生成的C语言数组文件在当前目录下，名为 img.txt
* 单色像素图的长度或宽度按8对齐向上取整，多余的长或宽填充0

*命令行参数解析功能来自`https://github.com/cofyc/argparse.git`*
