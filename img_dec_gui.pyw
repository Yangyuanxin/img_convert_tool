# -*- coding: utf-8 -*-

'''
gcc 编译dll
gcc -shared -o img_dec.dll .\img_common.c .\img_dec.c
'''

import tkinter as tk
from tkinter import ttk
from tkinter.filedialog import *
# from tkinter import *
import os
import datetime
# import time
# import ctypes
from ctypes import *

# ---------------- 全局设置 ----------------
# DEBUG_MODE = True
DEBUG_MODE = False

G_GRID_PADX = 5
G_GRID_PADY = 5

SPINBOX_SIZE_MIN = 1
SPINBOX_SIZE_MAX = 2000

SPINBOX_OFFSET_MIN = 0
SPINBOX_OFFSET_MAX = 2147483647


# ---------------- dll 加载及相关设置 ----------------
FMT_BITMAP_RL  = 1
FMT_BITMAP_RM  = 2
FMT_BITMAP_CL  = 3
FMT_BITMAP_CM  = 4
FMT_BITMAP_RCL = 5
FMT_BITMAP_RCM = 6
FMT_BITMAP_CRL = 7
FMT_BITMAP_CRM = 8
FMT_WEB        = 9
FMT_RGB565     = 10
FMT_BGR565     = 11
FMT_ARGB1555   = 12
FMT_BGRA5551   = 13
FMT_INVALID    = 14

DEC_SEEK_PREV = 1
DEC_SEEK_NEXT = 2
DEC_SEEK_HEAD = 3
DEC_SEEK_TAIL = 4
DEC_SEEK_GOTO = 5

class IMG_DEC_PARAM(Structure):
    _fields_ = [
        ("format", c_int),
        ("width", c_int),
        ("height", c_int),
        ("is_big_endian", c_int),
        ("file_offset", c_int),
        ("img_head_size", c_int),
        ("img_tail_size", c_int)
    ]


img_dec_dll = cdll.LoadLibrary("./img_dec.dll")
# img_dec_dll = CDLL("./img_dec.dll")


img_dec_dll.img_dec_open.argtypes = [c_char_p]
img_dec_dll.img_dec_open.restype = c_void_p

img_dec_dll.img_dec_close.argtypes = [c_void_p]
img_dec_dll.img_dec_close.restype = c_int

img_dec_dll.img_dec_cfg.argtypes = [c_void_p, c_void_p]
img_dec_dll.img_dec_cfg.restype = c_int

img_dec_dll.img_dec_get_size.argtypes = [c_void_p]
img_dec_dll.img_dec_get_size.restype = c_int

img_dec_dll.img_dec_get_num.argtypes = [c_void_p]
img_dec_dll.img_dec_get_num.restype = c_int

img_dec_dll.img_dec_seek.argtypes = [c_void_p, c_int, c_int]
img_dec_dll.img_dec_seek.restype = c_int

img_dec_dll.img_dec_tell.argtypes = [c_void_p]
img_dec_dll.img_dec_tell.restype = c_int

img_dec_dll.img_dec.argtypes = [c_void_p, c_void_p, c_int]
img_dec_dll.img_dec.restype = c_int



# ---------------- 全局变量 ----------------

# 图像解码
img_dec_param = IMG_DEC_PARAM(0, 0, 0, 0, 0, 0, 0)
img_dec_ptr = c_void_p()

img_total_num = 0
img_index = 0

# tkinter 解码后的图像
canvas_img_data = None


# ---------------- 函数定义 ----------------

def img_seek_dec_show(seek, img_id):
    rc = img_dec_dll.img_dec_seek(img_dec_ptr, seek, img_id)

    img_size = img_dec_param.height * img_dec_param.width * 3
    out_buf = (c_ubyte * img_size)()
    rc = img_dec_dll.img_dec(img_dec_ptr, out_buf, img_size)

    ppm_head = "P6 " + str(img_dec_param.width) + " " + str(img_dec_param.height) + " " + "255 "
    ppm_img = ppm_head.encode() + out_buf

    global canvas_img_data
    canvas_img_data = tk.PhotoImage(data = ppm_img)
    canvas_src.itemconfig(canvas_img, image=canvas_img_data)
    canvas_src.config(scrollregion=(0,0,canvas_img_data.width(),canvas_img_data.height()))


def dec_cfg_set():
    global img_total_num
    global img_index

    rc = img_dec_dll.img_dec_cfg(img_dec_ptr, byref(img_dec_param))

    img_total_num = img_dec_dll.img_dec_get_num(img_dec_ptr)
    if (img_total_num == 0):
        img_index = 0
        lb_img_num_content.config(text="0/0")
        spin_img_jump.config(from_=0, to=0)
        return
    if (img_index > img_total_num - 1):
        img_index = img_total_num - 1
    num_str = str(img_index + 1) + "/" + str(img_total_num)
    lb_img_num_content.config(text=num_str)
    spin_img_jump.config(from_=1, to=img_total_num)

    img_seek_dec_show(DEC_SEEK_GOTO, img_index)

def dec_cfg_cbox_change(event):
    img_dec_param.format = img_format_cbox.current()
    img_dec_param.is_big_endian = endian_cbox.current()
    if (bool(img_dec_ptr)):
        dec_cfg_set()

def dec_cfg_spin_change(*args):
    img_dec_param.height = tk_img_height.get()
    img_dec_param.width = tk_img_width.get()
    img_dec_param.file_offset = tk_file_offset.get()
    img_dec_param.img_head_size = tk_img_head_size.get()
    img_dec_param.img_tail_size = tk_img_tail_size.get()
    if (bool(img_dec_ptr)):
        dec_cfg_set()

def select_file():
    file_path = askopenfilename()
    tk_file_path.set(file_path)

def open_file():
    global img_dec_ptr
    file_path = tk_file_path.get()
    inputfile = create_string_buffer(file_path.encode("gbk"))
    img_dec_ptr = img_dec_dll.img_dec_open(inputfile)
    if (bool(img_dec_ptr)):
        lb_status_content.config(text="打开完成")
        entry_img_file.config(state="readonly")
        bt_select_file.configure(state="disabled")
        bt_open_img.configure(state="disabled")
    else:
        lb_status_content.config(text="无法打开")

def close_file():
    global img_dec_ptr
    img_dec_dll.img_dec_close(img_dec_ptr)
    img_dec_ptr = None
    entry_img_file.config(state="normal")
    bt_select_file.configure(state="normal")
    bt_open_img.configure(state="normal")

def prev_img():
    global img_total_num
    global img_index
    if (img_index > 0):
        img_index -= 1
    else:
        return
    num_str = str(img_index + 1) + "/" + str(img_total_num)
    lb_img_num_content.config(text=num_str)
    img_seek_dec_show(DEC_SEEK_GOTO, img_index)

def next_img():
    global img_total_num
    global img_index
    if (img_index < img_total_num - 1):
        img_index += 1
    else:
        return
    num_str = str(img_index + 1) + "/" + str(img_total_num)
    lb_img_num_content.config(text=num_str)
    img_seek_dec_show(DEC_SEEK_GOTO, img_index)

def jump_img():
    global img_total_num
    global img_index
    if (img_total_num == 0):
        return
    img_index = tk_img_jump_num.get() - 1
    if (img_index > img_total_num - 1):
        img_index = img_total_num - 1
    num_str = str(img_index + 1) + "/" + str(img_total_num)
    lb_img_num_content.config(text=num_str)
    img_seek_dec_show(DEC_SEEK_GOTO, img_index)

def save_img():
    global img_index
    rc = img_dec_dll.img_dec_seek(img_dec_ptr, DEC_SEEK_GOTO, img_index)

    img_size = img_dec_param.height * img_dec_param.width * 3
    out_buf = (c_ubyte * img_size)()
    rc = img_dec_dll.img_dec(img_dec_ptr, out_buf, img_size)

    ppm_head = "P6 " + str(img_dec_param.width) + " " + str(img_dec_param.height) + " " + "255 "
    ppm_img = ppm_head.encode() + out_buf

    file_path = tk_file_path.get()
    this_path = os.path.realpath(file_path)
    dir_path = os.path.dirname(this_path)
    save_name = "%06d" % (img_index) + ".ppm"
    save_path = os.path.join(dir_path, save_name)
    with open(save_path, "wb") as fw:
        fw.write(ppm_img)
    lb_status_content.config(text="保存完成")

def save_imgs():
    img_sum_num = img_dec_dll.img_dec_get_num(img_dec_ptr)
    rc = img_dec_dll.img_dec_seek(img_dec_ptr, DEC_SEEK_GOTO, 0)

    img_size = img_dec_param.height * img_dec_param.width * 3
    out_buf = (c_ubyte * img_size)()

    file_path = tk_file_path.get()
    this_path = os.path.realpath(file_path)
    dir_path = os.path.dirname(this_path)
    save_path = os.path.join(dir_path, "save_imgs")
    if not (os.path.exists(save_path)):
        os.mkdir(save_path)

    for img_cnt in range(0, img_sum_num):
        rc = img_dec_dll.img_dec(img_dec_ptr, out_buf, img_size)

        ppm_head = "P6 " + str(img_dec_param.width) + " " + str(img_dec_param.height) + " " + "255 "
        ppm_img = ppm_head.encode() + out_buf

        file_name = "%06d" % (img_cnt) + ".ppm"
        full_path = os.path.join(save_path, file_name)
        info = "正在保存" + file_name
        lb_status_content.config(text=info)
        with open(full_path, "wb") as fw:
            fw.write(ppm_img)
    lb_status_content.config(text="保存完成")


# ---------------- GUI 界面相关参数及初始化 ----------------
window = tk.Tk()
window.title("图片转换工具")
window.geometry("700x600")

# tk 中需要实时更新的变量
tk_file_path = tk.StringVar()
tk_img_jump_num = tk.IntVar()

tk_img_height = tk.IntVar()
tk_img_height.trace("w", dec_cfg_spin_change)
tk_img_width = tk.IntVar()
tk_img_width.trace("w", dec_cfg_spin_change)
tk_file_offset = tk.IntVar()
tk_file_offset.trace("w", dec_cfg_spin_change)
tk_img_head_size = tk.IntVar()
tk_img_head_size.trace("w", dec_cfg_spin_change)
tk_img_tail_size = tk.IntVar()
tk_img_tail_size.trace("w", dec_cfg_spin_change)


# 必须和 fmt_e 保持一致
img_format_info = [
    "请选择图像格式",
    "单色位图-逐行-LSB",
    "单色位图-逐行-MSB",
    "单色位图-逐列-LSB",
    "单色位图-逐列-MSB",
    "单色位图-行列-LSB",
    "单色位图-行列-MSB",
    "单色位图-列行-LSB",
    "单色位图-列行-MSB",
    "WEB颜色",
    "RGB565",
    "BGR565",
    "ARGB1555",
    "BGRA5551",
]

img_endian_info = [
    "小端",
    "大端",
]


# ---------------- GUI 界面绘制 ----------------

frame_settings = tk.Frame(window)
frame_settings.pack(anchor=tk.W)



frame_file = tk.Frame(frame_settings)
frame_file.grid(row=0, column=0, sticky=tk.W)

lbfm_open_file = tk.LabelFrame(frame_file, text="文件")
lbfm_open_file.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.N)

bt_select_file = tk.Button(lbfm_open_file, text="选择文件", width=10, height=1, command=select_file)
bt_select_file.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)

entry_img_file = tk.Entry(lbfm_open_file, width=40, textvariable=tk_file_path)
entry_img_file.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_open_img = tk.Button(lbfm_open_file, text="打开", width=8, height=1, command=open_file)
bt_open_img.grid(row=0, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_close_img = tk.Button(lbfm_open_file, text="关闭", width=8, height=1, command=close_file)
bt_close_img.grid(row=0, column=3, padx=G_GRID_PADX, pady=G_GRID_PADY)



frame_cfg = tk.Frame(frame_settings)
frame_cfg.grid(row=1, column=0, sticky=tk.W)

lbfm_img_format = tk.LabelFrame(frame_cfg, text="格式")
lbfm_img_format.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.N)

lb_img_format = tk.Label(lbfm_img_format, text="图像格式")
lb_img_format.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
img_format_cbox = ttk.Combobox(lbfm_img_format, width=16)
img_format_cbox.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)
img_format_cbox["value"] = img_format_info
img_format_cbox.configure(state="readonly")
img_format_cbox.current(0)
img_format_cbox.bind("<<ComboboxSelected>>", dec_cfg_cbox_change)

lb_img_endian = tk.Label(lbfm_img_format, text="字节序格式")
lb_img_endian.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
endian_cbox = ttk.Combobox(lbfm_img_format, width=16)
endian_cbox.grid(row=1, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)
endian_cbox["value"] = img_endian_info
endian_cbox.configure(state="readonly")
endian_cbox.current(0)
endian_cbox.bind("<<ComboboxSelected>>", dec_cfg_cbox_change)



lbfm_img_size = tk.LabelFrame(frame_cfg, text="尺寸")
lbfm_img_size.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.N)

lb_img_width = tk.Label(lbfm_img_size, text="长度")
lb_img_width.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
spin_img_width = tk.Spinbox(lbfm_img_size, width=8, from_=SPINBOX_SIZE_MIN, to=SPINBOX_SIZE_MAX, increment='1', textvariable=tk_img_width)
spin_img_width.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)

lb_img_height = tk.Label(lbfm_img_size, text="宽度")
lb_img_height.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
spin_img_height = tk.Spinbox(lbfm_img_size, width=8, from_=SPINBOX_SIZE_MIN, to=SPINBOX_SIZE_MAX, increment='1', textvariable=tk_img_height)
spin_img_height.grid(row=1, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)



lbfm_img_oth = tk.LabelFrame(frame_cfg, text="其他")
lbfm_img_oth.grid(row=0, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.N)

lb_img_file_offset = tk.Label(lbfm_img_oth, text="文件头部偏移量")
lb_img_file_offset.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
spin_file_offset = tk.Spinbox(lbfm_img_oth, width=8, from_=SPINBOX_OFFSET_MIN, to=SPINBOX_OFFSET_MAX, increment='1', textvariable=tk_file_offset)
spin_file_offset.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)

lb_img_head_size = tk.Label(lbfm_img_oth, text="图像头部偏移量")
lb_img_head_size.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
spin_img_head_size = tk.Spinbox(lbfm_img_oth, width=8, from_=SPINBOX_OFFSET_MIN, to=SPINBOX_OFFSET_MAX, increment='1', textvariable=tk_img_head_size)
spin_img_head_size.grid(row=1, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)

lb_img_tail_size = tk.Label(lbfm_img_oth, text="图像尾部偏移量")
lb_img_tail_size.grid(row=2, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
spin_img_tail_size = tk.Spinbox(lbfm_img_oth, width=8, from_=SPINBOX_OFFSET_MIN, to=SPINBOX_OFFSET_MAX, increment='1', textvariable=tk_img_tail_size)
spin_img_tail_size.grid(row=2, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)



frame_btn_box = tk.Frame(frame_settings)
frame_btn_box.grid(row=2, column=0, sticky=tk.W)

bt_prev_img = tk.Button(frame_btn_box, text="上一张", width=10, height=1, command=prev_img)
bt_prev_img.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_next_img = tk.Button(frame_btn_box, text="下一张", width=10, height=1, command=next_img)
bt_next_img.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_next_img = tk.Button(frame_btn_box, text="跳转至", width=10, height=1, command=jump_img)
bt_next_img.grid(row=0, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)

spin_img_jump = tk.Spinbox(frame_btn_box, width=8, from_=0, to=0, increment='1', textvariable=tk_img_jump_num)
spin_img_jump.grid(row=0, column=3, padx=G_GRID_PADX, pady=G_GRID_PADY)

lb_img_num_title = tk.Label(frame_btn_box, text="当前/总数")
lb_img_num_title.grid(row=0, column=4, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.E)

lb_img_num_content = tk.Label(frame_btn_box, text="0/0")
lb_img_num_content.grid(row=0, column=5, columnspan=2, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.W)



bt_save_img = tk.Button(frame_btn_box, text="保存单张", width=10, height=1, command=save_img)
bt_save_img.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_save_img = tk.Button(frame_btn_box, text="保存全部", width=10, height=1, command=save_imgs)
bt_save_img.grid(row=1, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)

lb_status_title = tk.Label(frame_btn_box, text="信息：")
lb_status_title.grid(row=1, column=4, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.E)

lb_status_content = tk.Label(frame_btn_box, text="无")
lb_status_content.grid(row=1, column=5, columnspan=2, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.W)



lbfm_img_src = tk.LabelFrame(window, text="预览")
lbfm_img_src.pack(anchor=tk.W, padx=G_GRID_PADX, pady=G_GRID_PADY, fill=tk.BOTH, expand=True)

canvas_src = tk.Canvas(lbfm_img_src)
canvas_src_scroll_x = tk.Scrollbar(lbfm_img_src, orient=tk.HORIZONTAL)
canvas_src_scroll_x.config(command=canvas_src.xview)
canvas_src_scroll_x.pack(side=tk.BOTTOM, fill=tk.X)
canvas_src_scroll_y = tk.Scrollbar(lbfm_img_src, orient=tk.VERTICAL)
canvas_src_scroll_y.config(command=canvas_src.yview)
canvas_src_scroll_y.pack(side=tk.RIGHT, fill=tk.Y)

canvas_src.config(xscrollcommand=canvas_src_scroll_x.set)
canvas_src.config(yscrollcommand=canvas_src_scroll_y.set)
# canvas_src.config(scrollregion=(0,0,img_src.width(),img_src.height()))
canvas_img = canvas_src.create_image(G_GRID_PADX, G_GRID_PADY, anchor=tk.NW) # 坐标不要设置为0,0 图像边缘会被canvas遮住
canvas_src.pack(fill=tk.BOTH, expand=True)
# canvas_src.create_image(0,0,image=img_src,anchor=tk.NW)



def main():
    window.mainloop()

    global img_dec_ptr
    if (bool(img_dec_ptr)):
        img_dec_dll.img_dec_close(img_dec_ptr)
        img_dec_ptr = None

# ---------------- 程序入口 ----------------
if (__name__ == "__main__"):
    main()
