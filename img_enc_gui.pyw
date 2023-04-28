# -*- coding: utf-8 -*-

'''
gcc 编译dll
gcc -shared -o img_enc.dll .\img_common.c .\img_enc.c
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

LUMINANCE_CONTRAST_MIN = -100
LUMINANCE_CONTRAST_MAX = 100

UINT8_MIN = 0
UINT8_MAX = 255


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


class IMG_ENC_PARAM(Structure):
    _fields_ = [
        ("format", c_int),
        ("is_big_endian", c_int),
        ("is_invert", c_int),
        ("use_edge_detector", c_int),
        ("use_dithering_algorithm", c_int),
        ("luminance", c_int),
        ("contrast", c_int),
        ("transparence", c_int),
    ]


img_enc_dll = cdll.LoadLibrary("./img_enc.dll")
# img_enc_dll = CDLL("./img_enc.dll")


img_enc_dll.img_enc_open.argtypes = [c_char_p]
img_enc_dll.img_enc_open.restype = c_void_p

img_enc_dll.img_enc_close.argtypes = [c_void_p]
img_enc_dll.img_enc_close.restype = c_int

img_enc_dll.img_enc_cfg.argtypes = [c_void_p, c_void_p]
img_enc_dll.img_enc_cfg.restype = c_int

img_enc_dll.img_enc_get_preview_size.argtypes = [c_void_p, c_void_p, c_void_p, c_void_p]
img_enc_dll.img_enc_get_preview_size.restype = c_int

img_enc_dll.img_enc_preview.argtypes = [c_void_p, c_void_p, c_int]
img_enc_dll.img_enc_preview.restype = c_int

img_enc_dll.img_enc_get_size.argtypes = [c_void_p, c_void_p, c_void_p, c_void_p]
img_enc_dll.img_enc_get_size.restype = c_int

img_enc_dll.img_enc.argtypes = [c_void_p, c_void_p, c_int]
img_enc_dll.img_enc.restype = c_int


# ---------------- 全局变量 ----------------

# 图像解码
img_enc_param = IMG_ENC_PARAM(0, 0, 0, 0, 0, 0, 0, 0)
img_enc_ptr = c_void_p()

# tkinter 解码后的图像
canvas_img_data = None

# 转换模式
CONVERT_MODE_NONE     = 0
CONVERT_MODE_SINGLE   = 1
CONVERT_MODE_MULTIPLE = 2
convert_mode = CONVERT_MODE_NONE


# ---------------- 函数定义 ----------------
class bin2c():
    def __init__(self):
        self.filename = ""
        self.c_code = ""
        self.ELEMENT_PER_LINE = 13

    # filename 为转换前的图片原始文件名
    def set_filename(self, filename):
        this_path = os.path.realpath(filename)
        dir_path = os.path.dirname(this_path)
        main_name = os.path.splitext(os.path.basename(filename))[0]
        save_name = main_name + ".c"
        self.filename = os.path.join(dir_path, save_name)
        self.c_code += "unsigned char img_%s[] = {\n" %(main_name)

    def write(self, data):
        line_count = int((len(data) + self.ELEMENT_PER_LINE - 1) / self.ELEMENT_PER_LINE)

        element_count = 0
        for i in range(0, line_count):
            if (i == line_count - 1):
                rest = len(data) - element_count
            else:
                rest = self.ELEMENT_PER_LINE

            each_line = "    "
            for j in range(0, rest):
                each_line += "0x%02x, " %(data[element_count])
                element_count += 1
            each_line += '\n'
            self.c_code += each_line

    def close(self):
        self.c_code += "};\n"
        with open(self.filename, "w") as fw:
            fw.write(self.c_code)


def enable_all_widgets(widgets):
    for child in widgets.winfo_children():
        enable_all_widgets(child)
        try:
            child.configure(state='normal')
        except Exception as e:
            pass

def disable_all_widgets(widgets):
    for child in widgets.winfo_children():
        disable_all_widgets(child)
        try:
            child.configure(state='disabled')
        except Exception as e:
            pass

def img_enc_show_preview():
    img_size = c_int(0)
    img_width = c_int(0)
    img_height = c_int(0)
    rc = img_enc_dll.img_enc_get_preview_size(img_enc_ptr, byref(img_size), byref(img_width), byref(img_height))
    if (rc != 0):
        lb_status_content.configure(text="预览失败")
        return

    out_buf = (c_ubyte * img_size.value)()
    rc = img_enc_dll.img_enc_preview(img_enc_ptr, out_buf, img_size)
    if (rc != 0):
        lb_status_content.configure(text="预览失败")
        return

    ppm_head = "P6 " + str(img_width.value) + " " + str(img_height.value) + " " + "255 "
    ppm_img = ppm_head.encode() + out_buf

    global canvas_img_data
    canvas_img_data = tk.PhotoImage(data = ppm_img)
    canvas_src.itemconfig(canvas_img, image=canvas_img_data)
    canvas_src.configure(scrollregion=(0,0,canvas_img_data.width(),canvas_img_data.height()))

def enc_cfg_read_all():
    global img_enc_param
    img_enc_param.format = img_format_cbox.current()
    img_enc_param.is_big_endian = endian_cbox.current()
    img_enc_param.is_invert = tk_img_is_invert.get()
    img_enc_param.use_edge_detector = tk_img_use_edge_detector.get()
    img_enc_param.use_dithering_algorithm = tk_img_use_dithering_algorithm.get()
    img_enc_param.luminance = tk_img_luminance.get()
    img_enc_param.contrast = tk_img_contrast.get()
    r = tk_img_transparence_r.get()
    g = tk_img_transparence_g.get()
    b = tk_img_transparence_r.get()
    img_enc_param.transparence = r * 255 * 255 + g * 255 + b

def enc_cfg_set():
    rc = img_enc_dll.img_enc_cfg(img_enc_ptr, byref(img_enc_param))
    if (rc != 0):
        lb_status_content.configure(text="设置错误")
        return

def enc_cfg_change(event):
    enc_cfg_read_all()
    if (bool(img_enc_ptr)):
        enc_cfg_set()
        img_enc_show_preview()

def enc_cfg_spin_change(*args):
    enc_cfg_read_all()
    if (bool(img_enc_ptr)):
        enc_cfg_set()
        img_enc_show_preview()

def select_file():
    file_path = askopenfilename(filetypes=[("BMP File", "*.bmp")])
    tk_file_path.set(file_path)

def open_file():
    global img_enc_ptr
    global convert_mode
    file_path = tk_file_path.get()
    inputfile = create_string_buffer(file_path.encode("gbk"))
    img_enc_ptr = img_enc_dll.img_enc_open(inputfile)
    if (bool(img_enc_ptr)):
        lb_status_content.configure(text="打开完成")
        entry_img_file.configure(state="readonly")
        bt_select_file.configure(state="disabled")
        bt_open_img.configure(state="disabled")
        disable_all_widgets(lbfm_open_folder)
        convert_mode = CONVERT_MODE_SINGLE
    else:
        lb_status_content.configure(text="无法打开")

def close_file():
    global img_enc_ptr
    global convert_mode
    img_enc_dll.img_enc_close(img_enc_ptr)
    img_enc_ptr = None
    entry_img_file.configure(state="normal")
    bt_select_file.configure(state="normal")
    bt_open_img.configure(state="normal")
    enable_all_widgets(lbfm_open_folder)
    convert_mode = CONVERT_MODE_NONE

def save_img():
    rc = img_enc_dll.img_enc_cfg(img_enc_ptr, byref(img_enc_param))
    if (rc != 0):
        lb_status_content.configure(text="设置错误")
        return

    img_size = c_int(0)
    img_width = c_int(0)
    img_height = c_int(0)
    rc = img_enc_dll.img_enc_get_size(img_enc_ptr, byref(img_size), byref(img_width), byref(img_height))
    if (rc != 0):
        lb_status_content.configure(text="转换失败")
        return
    out_buf = (c_ubyte * img_size.value)()
    rc = img_enc_dll.img_enc(img_enc_ptr, out_buf, img_size)
    if (rc != 0):
        lb_status_content.configure(text="转换失败")
        return

    file_path = tk_file_path.get()
    this_path = os.path.realpath(file_path)
    dir_path = os.path.dirname(this_path)
    
    src_full_name = os.path.basename(file_path)
    src_main_name = os.path.splitext(src_full_name)[0]

    save_name = src_main_name + ".bin"
    save_path = os.path.join(dir_path, save_name)

    with open(save_path, "wb") as fw:
        fw.write(out_buf)
    
    c_code = bin2c()
    c_code.set_filename(file_path)
    c_code.write(out_buf)
    c_code.close()
    lb_status_content.configure(text="转换完成")

def select_folder():
    folder_path = askdirectory()
    tk_folder_path.set(folder_path)

def open_folder():
    global img_enc_ptr
    global convert_mode
    entry_img_folder.configure(state="readonly")
    folder_path = tk_folder_path.get()
    if (not os.path.isdir(folder_path)):
        lb_status_content.configure(text="文件夹无效")
        return
    lb_status_content.configure(text="打开完成")
    entry_img_folder.configure(state="readonly")
    bt_select_folder.configure(state="disabled")
    bt_open_folder.configure(state="disabled")
    disable_all_widgets(lbfm_open_file)
    convert_mode = CONVERT_MODE_MULTIPLE    

def close_folder():
    global img_enc_ptr
    global convert_mode
    img_enc_ptr = None
    entry_img_folder.configure(state="normal")
    bt_select_folder.configure(state="normal")
    bt_open_folder.configure(state="normal")
    enable_all_widgets(lbfm_open_file)
    convert_mode = CONVERT_MODE_NONE

def convert_and_save(file):
    enc_file = create_string_buffer(file.encode("gbk"))
    img_enc_ptr = img_enc_dll.img_enc_open(enc_file)
    if (not bool(img_enc_ptr)):
        return 1
    
    rc = img_enc_dll.img_enc_cfg(img_enc_ptr, byref(img_enc_param))
    if (rc != 0):
        return 2

    img_size = c_int(0)
    img_width = c_int(0)
    img_height = c_int(0)
    rc = img_enc_dll.img_enc_get_size(img_enc_ptr, byref(img_size), byref(img_width), byref(img_height))
    if (rc != 0):
        return 3
    out_buf = (c_ubyte * img_size.value)()
    rc = img_enc_dll.img_enc(img_enc_ptr, out_buf, img_size)
    if (rc != 0):
        return 4
    
    img_enc_dll.img_enc_close(img_enc_ptr)

    this_path = os.path.realpath(file)
    dir_path = os.path.dirname(this_path)
    main_name = os.path.splitext(os.path.basename(file))[0]
    save_name = main_name + ".bin"
    save_path = os.path.join(dir_path, save_name)
    with open(save_path, "wb") as fw:
        fw.write(out_buf)

    c_code = bin2c()
    c_code.set_filename(file)
    c_code.write(out_buf)
    c_code.close()

    return 0

def save_imgs():
    info_text = ""
    err_count = 0
    sum_count = 0
    folder_path = tk_folder_path.get()
    dir_path = os.path.realpath(folder_path)
    enc_cfg_read_all()

    dirs = os.listdir(dir_path)
    for each_file in dirs:
        input_file = os.path.join(dir_path, each_file)
        if (not os.path.isfile(input_file)):
            continue

        ext_name = os.path.splitext(each_file)[-1]
        if (ext_name.lower() == ".bmp"):
            sum_count += 1
            rc = convert_and_save(input_file)
            if (rc != 0):
                err_count += 1
            info_text = "总计%d张，失败%d张" % (sum_count, err_count)
            lb_status_content.configure(text=info_text)

    lb_status_content.configure(text="转换完成\n" + info_text)

def do_convert():
    if (convert_mode == CONVERT_MODE_SINGLE):
        save_img()
    elif (convert_mode == CONVERT_MODE_MULTIPLE):
        save_imgs()


# ---------------- GUI 界面相关参数及初始化 ----------------
window = tk.Tk()
window.title("图片转换工具")
window.geometry("700x600")

# tk 中需要实时更新的变量
tk_file_path = tk.StringVar()
tk_folder_path = tk.StringVar()

tk_img_is_invert = tk.IntVar()
tk_img_is_invert.trace("w", enc_cfg_spin_change)
tk_img_use_edge_detector = tk.IntVar()
tk_img_use_edge_detector.trace("w", enc_cfg_spin_change)
tk_img_use_dithering_algorithm = tk.IntVar()
tk_img_use_dithering_algorithm.trace("w", enc_cfg_spin_change)

tk_img_luminance = tk.IntVar()
tk_img_luminance.trace("w", enc_cfg_spin_change)
tk_img_contrast = tk.IntVar()
tk_img_contrast.trace("w", enc_cfg_spin_change)
tk_img_transparence_r = tk.IntVar()
tk_img_transparence_r.trace("w", enc_cfg_spin_change)
tk_img_transparence_g = tk.IntVar()
tk_img_transparence_g.trace("w", enc_cfg_spin_change)
tk_img_transparence_b = tk.IntVar()
tk_img_transparence_b.trace("w", enc_cfg_spin_change)

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


lbfm_open_file = tk.LabelFrame(frame_file, text="单张转换")
lbfm_open_file.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.NW)

bt_select_file = tk.Button(lbfm_open_file, text="选择文件", width=10, height=1, command=select_file)
bt_select_file.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)

entry_img_file = tk.Entry(lbfm_open_file, width=40, textvariable=tk_file_path)
entry_img_file.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_open_img = tk.Button(lbfm_open_file, text="打开", width=8, height=1, command=open_file)
bt_open_img.grid(row=0, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_close_img = tk.Button(lbfm_open_file, text="关闭", width=8, height=1, command=close_file)
bt_close_img.grid(row=0, column=3, padx=G_GRID_PADX, pady=G_GRID_PADY)


lbfm_open_folder = tk.LabelFrame(frame_file, text="批量转换")
lbfm_open_folder.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.NW)

bt_select_folder = tk.Button(lbfm_open_folder, text="选择文件夹", width=10, height=1, command=select_folder)
bt_select_folder.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)

entry_img_folder = tk.Entry(lbfm_open_folder, width=40, textvariable=tk_folder_path)
entry_img_folder.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_open_folder = tk.Button(lbfm_open_folder, text="打开", width=8, height=1, command=open_folder)
bt_open_folder.grid(row=0, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)

bt_close_folder = tk.Button(lbfm_open_folder, text="关闭", width=8, height=1, command=close_folder)
bt_close_folder.grid(row=0, column=3, padx=G_GRID_PADX, pady=G_GRID_PADY)



frame_cfg = tk.Frame(frame_settings)
frame_cfg.grid(row=1, column=0, sticky=tk.W)

lbfm_img_format = tk.LabelFrame(frame_cfg, text="格式")
lbfm_img_format.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.NW)

lb_img_format = tk.Label(lbfm_img_format, text="图像格式")
lb_img_format.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
img_format_cbox = ttk.Combobox(lbfm_img_format, width=16)
img_format_cbox.grid(row=0, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)
img_format_cbox["value"] = img_format_info
img_format_cbox.configure(state="readonly")
img_format_cbox.current(0)
img_format_cbox.bind("<<ComboboxSelected>>", enc_cfg_change)

lb_img_endian = tk.Label(lbfm_img_format, text="字节序格式")
lb_img_endian.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
endian_cbox = ttk.Combobox(lbfm_img_format, width=16)
endian_cbox.grid(row=1, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY)
endian_cbox["value"] = img_endian_info
endian_cbox.configure(state="readonly")
endian_cbox.current(0)
endian_cbox.bind("<<ComboboxSelected>>", enc_cfg_change)



lbfm_img_oth = tk.LabelFrame(frame_cfg, text="调整")
lbfm_img_oth.grid(row=0, column=1, columnspan=2, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.NW)

lb_img_luminance = tk.Label(lbfm_img_oth, text="亮度")
lb_img_luminance.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
sl_img_luminance = tk.Scale(lbfm_img_oth, orient='horizontal', length=150, from_=LUMINANCE_CONTRAST_MIN, to=LUMINANCE_CONTRAST_MAX, showvalue=False, variable=tk_img_luminance)
sl_img_luminance.grid(row=1, column=1)
spin_img_luminance = tk.Spinbox(lbfm_img_oth, width=4, from_=LUMINANCE_CONTRAST_MIN, to=LUMINANCE_CONTRAST_MAX, increment='1', textvariable=tk_img_luminance)
spin_img_luminance.grid(row=1, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)

lb_img_contrast = tk.Label(lbfm_img_oth, text="对比度")
lb_img_contrast.grid(row=2, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
sl_img_contrast = tk.Scale(lbfm_img_oth, orient='horizontal', length=150, from_=LUMINANCE_CONTRAST_MIN, to=LUMINANCE_CONTRAST_MAX, showvalue=False, variable=tk_img_contrast)
sl_img_contrast.grid(row=2, column=1)
spin_img_contrast = tk.Spinbox(lbfm_img_oth, width=4, from_=LUMINANCE_CONTRAST_MIN, to=LUMINANCE_CONTRAST_MAX, increment='1', textvariable=tk_img_contrast)
spin_img_contrast.grid(row=2, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)



lbfm_img_file_transparence = tk.LabelFrame(frame_cfg, text="透明色")
lbfm_img_file_transparence.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.NW)

lb_img_transparence_r = tk.Label(lbfm_img_file_transparence, text="R")
lb_img_transparence_r.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
sl_img_transparence_r = tk.Scale(lbfm_img_file_transparence, orient='horizontal', length=150, from_=UINT8_MIN, to=UINT8_MAX, showvalue=False, variable=tk_img_transparence_r)
sl_img_transparence_r.grid(row=0, column=1)
spin_img_transparence_r = tk.Spinbox(lbfm_img_file_transparence, width=4, from_=UINT8_MIN, to=UINT8_MAX, increment='1', textvariable=tk_img_transparence_r)
spin_img_transparence_r.grid(row=0, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)

lb_img_transparence_r = tk.Label(lbfm_img_file_transparence, text="G")
lb_img_transparence_r.grid(row=1, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
sl_img_transparence_r = tk.Scale(lbfm_img_file_transparence, orient='horizontal', length=150, from_=UINT8_MIN, to=UINT8_MAX, showvalue=False, variable=tk_img_transparence_g)
sl_img_transparence_r.grid(row=1, column=1)
spin_img_transparence_r = tk.Spinbox(lbfm_img_file_transparence, width=4, from_=UINT8_MIN, to=UINT8_MAX, increment='1', textvariable=tk_img_transparence_g)
spin_img_transparence_r.grid(row=1, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)

lb_img_transparence_r = tk.Label(lbfm_img_file_transparence, text="B")
lb_img_transparence_r.grid(row=2, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY)
sl_img_transparence_r = tk.Scale(lbfm_img_file_transparence, orient='horizontal', length=150, from_=UINT8_MIN, to=UINT8_MAX, showvalue=False, variable=tk_img_transparence_b)
sl_img_transparence_r.grid(row=2, column=1)
spin_img_transparence_r = tk.Spinbox(lbfm_img_file_transparence, width=4, from_=UINT8_MIN, to=UINT8_MAX, increment='1', textvariable=tk_img_transparence_b)
spin_img_transparence_r.grid(row=2, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY)



lbfm_img_effect = tk.LabelFrame(frame_cfg, text="效果")
lbfm_img_effect.grid(row=1, column=1, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.NW)

ckbt_is_invert = tk.Checkbutton(lbfm_img_effect, text="反色", variable=tk_img_is_invert, onvalue=1, offvalue=0, width=8, anchor=tk.W)
ckbt_is_invert.grid(row=0, column=0)
ckbt_use_edge_detector = tk.Checkbutton(lbfm_img_effect, text="边缘检测", variable=tk_img_use_edge_detector, onvalue=1, offvalue=0, width=8, anchor=tk.W)
ckbt_use_edge_detector.grid(row=1, column=0)
ckbt_use_dithering_algorithm = tk.Checkbutton(lbfm_img_effect, text="颜色抖动", variable=tk_img_use_dithering_algorithm, onvalue=1, offvalue=0, width=8, anchor=tk.W)
ckbt_use_dithering_algorithm.grid(row=2, column=0)



frame_btn_box = tk.Frame(frame_cfg)
frame_btn_box.grid(row=1, column=2, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.NW)

bt_save_img = tk.Button(frame_btn_box, text="转换", width=10, height=1, command=do_convert)
bt_save_img.grid(row=0, column=0, padx=G_GRID_PADX, pady=G_GRID_PADY, sticky=tk.W)

lb_status_title = tk.Label(frame_btn_box, text="信息：", width=24, anchor=tk.W, justify=tk.LEFT)
lb_status_title.grid(row=1, column=0, padx=G_GRID_PADX, pady=0, sticky=tk.W)

lb_status_content = tk.Label(frame_btn_box, text="无", width=24, anchor=tk.W, justify=tk.LEFT)
lb_status_content.grid(row=2, column=0, columnspan=2, padx=G_GRID_PADX, pady=0, sticky=tk.W)



lbfm_img_src = tk.LabelFrame(window, text="预览")
lbfm_img_src.pack(anchor=tk.W, padx=G_GRID_PADX, pady=G_GRID_PADY, fill=tk.BOTH, expand=True)

canvas_src = tk.Canvas(lbfm_img_src)
canvas_src_scroll_x = tk.Scrollbar(lbfm_img_src, orient=tk.HORIZONTAL)
canvas_src_scroll_x.configure(command=canvas_src.xview)
canvas_src_scroll_x.pack(side=tk.BOTTOM, fill=tk.X)
canvas_src_scroll_y = tk.Scrollbar(lbfm_img_src, orient=tk.VERTICAL)
canvas_src_scroll_y.configure(command=canvas_src.yview)
canvas_src_scroll_y.pack(side=tk.RIGHT, fill=tk.Y)

canvas_src.configure(xscrollcommand=canvas_src_scroll_x.set)
canvas_src.configure(yscrollcommand=canvas_src_scroll_y.set)
# canvas_src.configure(scrollregion=(0,0,img_src.width(),img_src.height()))
canvas_img = canvas_src.create_image(G_GRID_PADX, G_GRID_PADY, anchor=tk.NW) # 坐标不要设置为0,0 图像边缘会被canvas遮住
canvas_src.pack(fill=tk.BOTH, expand=True)
# canvas_src.create_image(0,0,image=img_src,anchor=tk.NW)



def main():
    window.mainloop()

    global img_enc_ptr
    if (bool(img_enc_ptr)):
        img_enc_dll.img_enc_close(img_enc_ptr)
        img_enc_ptr = None

# ---------------- 程序入口 ----------------
if (__name__ == "__main__"):
    main()
