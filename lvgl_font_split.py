#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
lvgl_font_split.py
将 lv_font_conv 生成的 .c 字体文件拆分为两部分：
1. 保留引用关系的 .c 文件（cmaps, font_dsc, font 结构体）
2. 纯数据 .bin 文件（glyph_bitmap + glyph_dsc），通过 objcopy 生成

用法:
    python lvgl_font_split.py font_harmony_sans_20.c --toolchain xtensa-esp32s3-elf

依赖:
    - xtensa-esp32s3-elf-gcc / ld / objcopy / objdump（或指定其他工具链前缀）
    - Python 3.6+
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
import shutil

def parse_args():
    parser = argparse.ArgumentParser(
        description="Split LVGL .c font into data-less .c and .bin"
    )
    parser.add_argument("input_c", help="Input .c font file from lv_font_conv")
    parser.add_argument(
        "-o", "--output-prefix",
        default=None,
        help="Output file(default: same as input basename)"
    )
    parser.add_argument(
        "--toolchain", "-t",
        default="xtensa-esp32s3-elf",
        help="GCC toolchain prefix (default: xtensa-esp32s3-elf)"
    )
    parser.add_argument(
        "--lvgl-include", "-I",
        default="managed_components/lvgl__lvgl",
        help="Path to lvgl.h parent directory"
    )
    parser.add_argument(
        "--defs", "-D",
        action="append",
        default=["LV_CONF_SKIP", "LV_FONT_FMT_TXT_LARGE=1"],
        help="Extra macro definitions (can be used multiple times)"
    )
    parser.add_argument(
        "--link-script", "-T",
        default=None,
        help="Custom linker script (default: auto-generated rodata.ld)"
    )
    parser.add_argument(
        "--keep-temp",
        action="store_true",
        help="Keep temporary build directory for debugging"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Print detailed commands and output"
    )
    return parser.parse_args()


def find_toolchain(toolchain_prefix):
    """验证工具链是否存在并返回各工具路径"""
    tools = {
        "gcc": f"{toolchain_prefix}-gcc",
        "ld": f"{toolchain_prefix}-ld",
        "objcopy": f"{toolchain_prefix}-objcopy",
        "objdump": f"{toolchain_prefix}-objdump",
    }
    for name, cmd in tools.items():
        if not shutil.which(cmd):
            print(f"[ERROR] Tool not found: {cmd}")
            print(f"        Please ensure {toolchain_prefix} toolchain is in PATH")
            sys.exit(1)
    return tools

def split_data_c(content):
        include_line = """#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif
"""
        bitmap_pattern = re.compile(
            r'^\s*static\s+LV_ATTRIBUTE_LARGE_CONST\s+const\s+uint8_t\s+glyph_bitmap\s*\[\]\s*=\s*\{',
            re.MULTILINE
        )
        bitmap_match = bitmap_pattern.search(content)
        bitmap_start = bitmap_match.start()

        dsc_pattern = re.compile(
            r'^\s*static\s+const\s+lv_font_fmt_txt_glyph_dsc_t\s+glyph_dsc\s*\[\]\s*=\s*\{',
            re.MULTILINE
        )
        dsc_match = dsc_pattern.search(content)
        dsc_start = dsc_match.start()
        dsc_end = find_array_end(content, dsc_start)

        data_section = content[bitmap_start:dsc_end]
        data_c = f"""{include_line}

{data_section}
"""
        return data_c, bitmap_start, dsc_end

def split_header_c(content, bitmap_offset, dsc_offset, bitmap_start, dsc_end):
    """
    解析 .c 文件，拆分:
    - header_c: 包含 cmaps/font_dsc/font 的 .c（用于工程引用）
    """
    # 保留 bitmap 之前的所有内容
    before_bitmap = content[:bitmap_start].rstrip()

    # 保留 dsc 结束之后的所有内容
    after_dsc = content[dsc_end:].rstrip()

    # 提取字体名称（从 font_xxx 变量名）
    font_name_match = re.search(
        r'lv_font_t\s+(\w+)\s*=',
        after_dsc
    )
    if font_name_match:
        font_name = font_name_match.group(1)
    else:
        font_name = "font_custom"

    
    # 修改 header_c：将 font_dsc 中的 glyph_bitmap 和 glyph_dsc 设为 NULL
    # 去掉 font_dsc 的const
    font_dsc_pattern = re.compile(
        r'(static\s+)(const\s+)(lv_font_fmt_txt_dsc_t\s+font_dsc\s*=\s*\{)',
        re.DOTALL
    )
    after_dsc = font_dsc_pattern.sub(r'\1\3', after_dsc)

    # 将 glyph_bitmap 和 glyph_dsc 的引用替换为 NULL
    after_dsc = re.sub(
        r'\.glyph_bitmap\s*=\s*glyph_bitmap',
        '.glyph_bitmap = NULL',
        after_dsc
    )
    after_dsc = re.sub(
        r'\.glyph_dsc\s*=\s*glyph_dsc',
        '.glyph_dsc = NULL',
        after_dsc
    )

    # 在 header_c 末尾添加加载函数声明和实现
    load_func = f"""
/* ============================================
 * Auto-generated font loader for Flash mapping
 * ============================================ */
#include "esp_partition.h"
#include "esp_log.h"

#define TAG "{font_name}_load"

/* 通过 objdump -t 提取的硬编码偏移地址 */
#define GLYPH_BITMAP_OFFSET  0x{bitmap_offset:08x}
#define GLYPH_DSC_OFFSET     0x{dsc_offset:08x}

/**
 * @brief 从 Flash 分区加载字体数据
 * @param partition_label 分区名称 (如 "font_hs20")
 */
void {font_name}_load(const char *partition_label)
{{
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, partition_label);
    if (partition == NULL) {{
        ESP_LOGE(TAG, "Can't find %s partition!", partition_label);
        abort();
    }}

    esp_partition_mmap_handle_t mmap_handle;
    const void *flash_offset;
    ESP_ERROR_CHECK(esp_partition_mmap(partition, 0, partition->size,
                ESP_PARTITION_MMAP_DATA, &flash_offset, &mmap_handle));
    ESP_LOGI(TAG, "Mapped %s @ %p", partition->label, flash_offset);

    font_dsc.glyph_bitmap = flash_offset + GLYPH_BITMAP_OFFSET;
    font_dsc.glyph_dsc    = flash_offset + GLYPH_DSC_OFFSET;
}}
"""
    lines = after_dsc.splitlines()
    lines.insert(-1, load_func)

    header_c = before_bitmap + "\n" + "\n".join(lines)

    return header_c, font_name


def find_array_end(content, start_pos):
    """
    从数组定义开始位置找到匹配的闭合大括号。
    会正确处理：字符串、行注释 // 、块注释 /* */ 。
    """
    brace_depth = 0
    in_string = False
    string_char = None
    in_line_comment = False   # //
    in_block_comment = False  # /* */
    i = start_pos
    found_open = False

    while i < len(content):
        ch = content[i]

        # ========== 行注释 // ==========
        if in_line_comment:
            if ch == '\n':
                in_line_comment = False
            i += 1
            continue

        # ========== 块注释 /* */ ==========
        if in_block_comment:
            if ch == '*' and i + 1 < len(content) and content[i + 1] == '/':
                in_block_comment = False
                i += 2
            else:
                i += 1
            continue

        # ========== 字符串 "..." 或 '...' ==========
        if in_string:
            if ch == string_char and content[i - 1] != '\\':
                    in_string = False
            i += 1
            continue

        # ========== 检测注释开始 ==========
        if ch == '/' and i + 1 < len(content):
            nxt = content[i + 1]
            if nxt == '/':
                in_line_comment = True
                i += 2
                continue
            if nxt == '*':
                in_block_comment = True
                i += 2
                continue

        # ========== 检测字符串开始 ==========
        if ch in ('"', "'"):
            in_string = True
            string_char = ch
            i += 1
            continue

        # ========== 大括号匹配（仅在非注释非字符串时）==========
        if ch == '{':
            brace_depth += 1
            found_open = True
        elif ch == '}':
            brace_depth -= 1
            if found_open and brace_depth == 0:
                j = i + 1
                while j < len(content) and content[j] in ' \t\n;':
                    j += 1
                return j

        i += 1

    print("[ERROR] 无法找到数组闭合大括号")
    sys.exit(1)


def compile_and_link(data_c_path, tools, defs, lvgl_include, link_script, temp_dir, verbose=False):
    """
    编译 data_c 为 .o，链接为 .out，再 objcopy 为 .bin
    返回: (bin_path, symbol_info)
    """
    obj_path = os.path.join(temp_dir, "font_data.o")
    out_path = os.path.join(temp_dir, "font_data.out")
    bin_path = os.path.join(temp_dir, "font_data.bin")

    # 1. 编译
    gcc_cmd = [
        tools["gcc"],
        "-c",
    ]
    for d in defs:
        gcc_cmd.extend(["-D", d])

    gcc_cmd.extend(["-I", lvgl_include])
    gcc_cmd.extend(["-o", obj_path, data_c_path])

    if verbose:
        print(f"[CMD] {' '.join(gcc_cmd)}")
    result = subprocess.run(gcc_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("[ERROR] GCC 编译失败:")
        print(result.stderr)
        sys.exit(1)

    # 2. 链接
    if link_script is None:
        # 自动生成链接脚本
        link_script = os.path.join(temp_dir, "rodata_gen_bin.ld")
        with open(link_script, "w") as f:
            f.write("""SECTIONS {
    . = 0x0;
    .text : {
        *(.rodata)
    } = 0
}""")

    ld_cmd = [
        tools["ld"],
        "-T", link_script,
        obj_path,
        "-o", out_path,
    ]
    if verbose:
        print(f"[CMD] {' '.join(ld_cmd)}")
    result = subprocess.run(ld_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("[ERROR] LD 链接失败:")
        print(result.stderr)
        sys.exit(1)
    # ld 警告（无入口函数）是正常的
    if verbose and result.stderr:
        print(f"[WARN] LD stderr: {result.stderr.strip()}")

    # 3. 生成 bin
    objcopy_cmd = [
        tools["objcopy"],
        "--output-target", "elf-xtensa-le",
        "-O", "binary",
        "-S",
        out_path,
        bin_path,
    ]
    if verbose:
        print(f"[CMD] {' '.join(objcopy_cmd)}")
    result = subprocess.run(objcopy_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("[ERROR] objcopy 失败:")
        print(result.stderr)
        sys.exit(1)

    # 4. 获取符号偏移
    objdump_cmd = [tools["objdump"], "-t", out_path]
    if verbose:
        print(f"[CMD] {' '.join(objdump_cmd)}")
    result = subprocess.run(objdump_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("[ERROR] objdump 失败:")
        print(result.stderr)
        sys.exit(1)

    symbol_info = parse_symbols(result.stdout)
    return bin_path, symbol_info


def parse_symbols(objdump_output):
    """解析 objdump -t 输出，提取 glyph_bitmap 和 glyph_dsc 的偏移"""
    info = {}
    for line in objdump_output.splitlines():
        # 格式示例: 00000000 l     O .text  000e443c glyph_bitmap
        parts = line.split()
        if len(parts) >= 6:
            addr = parts[0]
            sym_type = parts[2]
            sym_name = parts[-1]
            if sym_name in ("glyph_bitmap", "glyph_dsc"):
                info[sym_name] = int(addr, 16)
    return info

def main():
    args = parse_args()

    input_path = os.path.abspath(args.input_c)
    if not os.path.exists(input_path):
        print(f"[ERROR] 文件不存在: {input_path}")
        sys.exit(1)

    prefix = args.output_prefix or os.path.splitext(os.path.basename(input_path))[0]
    out_c_path = os.path.abspath(f"{prefix}_header.c")
    out_bin_path = os.path.abspath(f"{prefix}.bin")

    print(f"[INFO] 输入文件: {input_path}")
    print(f"[INFO] 工具链前缀: {args.toolchain}")

    # 验证工具链
    tools = find_toolchain(args.toolchain)
    print(f"[INFO] 工具链验证通过")

    # 创建临时目录
    temp_dir = tempfile.mkdtemp(prefix="lvgl_font_")
    if args.verbose:
        print(f"[INFO] 临时目录: {temp_dir}")

    try:
        # 这里先读文件获取 bitmap/dsc 位置
        with open(input_path, "r", encoding="utf-8") as f:
            content = f.read()

        data_c, bitmap_start, dsc_end = split_data_c(content)
        # 写入 data_c
        data_c_path = os.path.join(temp_dir, "font_data.c")
        with open(data_c_path, "w", encoding="utf-8") as f:
            f.write(data_c)

        # 编译链接生成 bin，同时获取符号偏移
        print(f"[INFO] 正在编译生成 .bin 文件...")
        bin_temp_path, symbol_info = compile_and_link(
            data_c_path, tools, args.defs, args.lvgl_include,
            args.link_script, temp_dir, verbose=args.verbose
        )

        # 检查符号
        if "glyph_bitmap" not in symbol_info or "glyph_dsc" not in symbol_info:
            print("[ERROR] 未在输出文件中找到 glyph_bitmap / glyph_dsc 符号")
            sys.exit(1)

        bitmap_offset = symbol_info["glyph_bitmap"]
        dsc_offset = symbol_info["glyph_dsc"]

        print(f"[INFO] objdump 提取偏移: glyph_bitmap=0x{bitmap_offset:08x}, glyph_dsc=0x{dsc_offset:08x}")

        # 用提取到的偏移重新拆分生成 header_c
        print(f"[INFO] 正在生成 header.c（偏移硬编码）...")
        header_c, font_name = split_header_c(content, bitmap_offset, dsc_offset, bitmap_start, dsc_end)

        # 复制输出文件
        shutil.copy2(bin_temp_path, out_bin_path)
        with open(out_c_path, "w", encoding="utf-8") as f:
            f.write(header_c)

        # 计算 bin 大小
        bin_size = os.path.getsize(out_bin_path)

        print()
        print("=" * 50)
        print("处理完成!")
        print("=" * 50)
        print(f"字体头文件:  {out_c_path}")
        print(f"字体数据:    {out_bin_path}")
        print(f"  - 文件大小: {bin_size} bytes ({bin_size / 1024:.1f} KB)")
        print()
        print("Flash 分区偏移信息 (已硬编码到 load 函数):")
        print(f"  glyph_bitmap offset: 0x{bitmap_offset:08x} ({bitmap_offset})")
        print(f"  glyph_dsc    offset: 0x{dsc_offset:08x} ({dsc_offset})")
        print()
        print("使用示例:")
        print(f'  {font_name}_load("font_hs20");  // 偏移已硬编码')
        print()
        print("分区表示例 (partitions.csv):")
        print(f"  font_hs20, data, , 0x200000, {bin_size},")
        print("=" * 50)

    finally:
        if not args.keep_temp:
            shutil.rmtree(temp_dir, ignore_errors=True)
        else:
            print(f"[INFO] 保留临时目录: {temp_dir}")


if __name__ == "__main__":
    main()
