# 语言要求

**所有输出内容必须使用中文**  
**所有思考过程必须使用中文**  
代码注释优先使用中文，日志输出优先使用英文

# Smart-Puppy 项目开发指南

## 项目概述

Smart-Puppy 是一个基于 ESP32-S3 的智能桌面机器人项目，搭载 ST7789 LCD 屏幕（320x240）、扬声器、麦克风、舵机和加速度计。固件使用 ESP-IDF 5.5.4 开发，UI 框架为 LVGL v9.5。

AI相关功能基于 [xiaozhi_esp32](https://github.com/78/xiaozhi-esp32) v2.2.6，支持 语音唤醒、AI 语音对话、舵机控制、音乐播放等功能。  
主要的修改有：
- 删除屏幕显示和lvgl相关内容
- 删除wifi连接配网相关内容
- 删除多语言相关内容，只保留简体中文
- 删除音频、字体等资源打包、烧录、下载等相关内容
- 删除ota更新，保留激活验证
- 删除其他开发板、codec支持
- 引出C语言函数接口

## 硬件引脚

| 功能 | GPIO | 说明 |
|------|------|------|
| **LCD** | | |
| LCD SCK | 2 | SPI 时钟 |
| LCD SDA | 43 | SPI 数据 |
| LCD DC | 44 | 数据/命令选择 |
| LCD CS | 1 | 片选 |
| LCD RST | 42 | 复位 |
| LCD 背光 | 41 | LED 背光 PWM |
| **按键** | 45 | 单 GPIO 多功能按键 |
| **音频** | | |
| 扬声器 BCLK | 12 | I2S 位时钟 |
| 扬声器 WS | 11 | I2S 字选择 |
| 扬声器 DIN | 13 | I2S 数据输出 |
| 功放使能 PA_CTRL | 14 | MAX98357 SD_MODE |
| 麦克风 BCLK | 7 | PDM 时钟 |
| 麦克风 WS | 15 | PDM 数据（左/右通道） |
| 麦克风 DOUT | 16 | PDM 数据输入 |
| **舵机** | | |
| 前左腿 LF | 17 | LEDC ch0, 50Hz PWM |
| 前右腿 RF | 3 | LEDC ch2, 注意不是常见的 16 |
| 后左腿 LR | 18 | LEDC ch1 |
| 后右腿 RR | 46 | LEDC ch3 |
| 尾巴 Tail | 38 | LEDC ch4 |
| **加速度计 SC7A20H** | | |
| I2C SDA | 47 | I2C 数据线 |
| I2C SCL | 48 | I2C 时钟线 |
| INT1 | NC | 未连接 |
| INT2 | 21 | 运动检测中断 |
| **电源管理** | | |
| 电池 ADC | 10 | ADC1_CH9，300k/100k 分压（4:1） |
| 充电检测 | 9 | 低电平表示充电中 |

## 分区表

| 分区名 | 类型 | 大小 | 用途 |
|--------|------|------|------|
| nvs | data/nvs | 0x6000 | 非易失存储（设置、Wi-Fi密码） |
| phy_init | data/phy | 0x1000 | PHY 初始化数据 |
| factory | app/factory | 3M | 固件 |
| audio | data/spiffs | 100K | 音频文件（WAV） |
| image | data/spiffs | 500K | GIF 表情（内存映射） |
| font | data/raw | 600K | 中文字体二进制 |
| model | data/raw | 960K | ML 模型数据 |

## 项目结构

```
main/
├── main.c                    # app_main 入口
├── CMakeLists.txt            # 编译配置，需要手动添加新 C 文件到 SOURCES
├── Kconfig.projbuild         # Kconfig 硬件引脚配置
├── lcd/
│   ├── lcd.c/.h              # ST7789 初始化、LVGL 初始化、背光 PWM
│   ├── ui/ui.c/.h            # UI 框架：屏幕管理、导航栈、主题、App 分发
│   ├── key/key.c/.h          # 按键驱动（单 GPIO，支持单击/双击/长按/三击）
│   ├── style/my_style.c/.h   # 所有 App 的 LVGL 样式（LV_STYLE_CONST_INIT 方式）
│   ├── font/                 # 字体文件（FontAwesome 各尺寸 + 中文字体）
│   ├── gif/                  # GIF 表情资源
│   └── apps/                 # 各应用界面
│       ├── chat.c            # AI 对话界面
│       ├── clock.c           # 时钟/舵机按钮
│       ├── servo_ctrl.c      # 舵机控制
│       ├── music.c           # 本地音乐播放器
│       ├── recorder.c        # 录音器
│       ├── settings.c        # 设置（亮度/音量/主题/Wi-Fi开关/休眠/关于）
│       └── wifi.c            # Wi-Fi 扫描/连接
├── accel/                    # 加速度计驱动（SC7A20H）
├── servo/                    # 舵机控制
├── power/                    # 电池/电源管理
└── wifi_manager/             # Wi-Fi 管理
```

## UI 框架核心约定

### App 注册流程

1. 在 `main/lcd/apps/` 下创建 `xxx.c`
2. 在 `main/CMakeLists.txt` 的 SOURCES 列表中添加 `"lcd/apps/xxx.c"`
3. 在 `main/lcd/ui/ui.c` 中声明 `void app_init_xxx(void);`
4. 在 `ui.c` 的 `apps[]` 数组里添加 `{ "标题", 图标常量, app_init_xxx }`

### 屏幕创建

```c
// 标准屏幕：标题栏 + 内容区域 + LVGL group
lv_obj_t* screen = my_screen_create("标题", exit_callback);
lv_obj_t* content = my_screen_get_content(screen);  // child 1
lv_obj_t* header  = my_screen_get_header(screen);    // child 0
lv_obj_t* title   = my_screen_get_title(screen);     // header->child 0
lv_group_t* group = my_screen_get_group(screen);

// 空屏幕：无标题栏
lv_obj_t* screen = my_empty_screen_create(exit_callback);

// 弹窗
lv_obj_t* msgbox = my_msgbox_create(has_group);

// 推入导航栈（带动画）
add_path(screen);
```

### 导航栈

- `add_path(screen)` — 推入新页面（从底部滑入动画）
- `delete_path()` — 弹出当前页面（长按按键自动调用）
- exit_callback 在页面退出时被调用，用于清理静态变量

### 按键交互

单 GPIO（45）按键，通过单击/双击/长按/三击实现不同功能：

- **单击**：`LV_KEY_NEXT`（聚焦下一个控件）
- **双击**：`LV_KEY_ENTER`（激活/确认）
- **长按**：`LV_KEY_ESC`（返回上一页）
- **三击**：反转导航方向 + 切换循环图标

编辑模式下（`key_edit_mode_invert()`）：

- 单击变为 `LV_KEY_RIGHT`/`LV_KEY_LEFT`

### 自动主题

自定义 LVGL 主题 `my_theme_apply_cb` 会自动将 `lv_button`、`lv_buttonmatrix`、`lv_switch` 添加到当前屏幕的 group 中，无需手动调用 `lv_group_add_obj`（但其他控件类型需要手动添加）。

### 样式系统

所有样式在 `my_style.c` 中定义，使用 `LV_STYLE_CONST_INIT` 方式声明为常量，在 `my_style.h` 中 `extern` 声明。

```c
// 定义（my_style.c）
static const lv_style_const_prop_t style_xxx_props[] = {
    LV_STYLE_CONST_WIDTH(100),
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_BLUE),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_xxx, style_xxx_props);

// 声明（my_style.h）
extern const lv_style_t style_xxx;
```

常用颜色宏：

- `UI_COLOR_FORE` — `LV_COLOR_MAKE(0x26, 0x32, 0x38)` 深色（标题栏背景）
- `UI_COLOR_BACK` — `LV_COLOR_MAKE(0xF5, 0xF5, 0xF5)` 浅色（内容背景）
- `UI_COLOR_BLUE` — `LV_COLOR_MAKE(0x21, 0x96, 0xF3)` 蓝色
- `UI_COLOR_RED` — `LV_COLOR_MAKE(0xD7, 0x3A, 0x4A)` 红色
- `UI_COLOR_GREEN` — `LV_COLOR_MAKE(0x28, 0xA7, 0x45)` 绿色
- `UI_COLOR_GREY` — `LV_COLOR_MAKE(0xAA, 0xAA, 0xAA)` 灰色

### 线程安全

在非 LVGL 任务（如回调、中断）中更新 UI 时，必须加锁：

```c
lvgl_port_lock(0);
// 更新 UI 控件...
lvgl_port_unlock();
```

## LVGL v9 API 注意

- 图片描述符为 `lv_image_dsc_t`（v8 为 `lv_img_dsc_t`）
- 类型检查使用 `lv_obj_has_class(obj, &lv_button_class)`（v8 为 `lv_obj_check_type`）
- 长文本模式：`LV_LABEL_LONG_MODE_SCROLL_CIRCULAR`（带 `_MODE_`）
- 样式属性：`LV_STYLE_CONST_WIDTH` 等（v8 为 `LV_STYLE_CONST_WIDTH`，相同但部分枚举值有差异）

## 硬件抽象层（HAL）模式

录音器和音乐播放器都使用 HAL 接口模式，方便在 PC 模拟和真实硬件间切换：

```c
typedef struct {
    bool (*init)(void);
    bool (*play)(...);
    bool (*stop)(void);
    // ...
} xxx_hal_t;

static const xxx_hal_t mock_hal = { ... };  // 模拟实现
static const xxx_hal_t* hal = &mock_hal;     // 默认使用模拟

void xxx_set_hal(const xxx_hal_t* real_hal); // 切换真实硬件
```

## 音频子系统

- AudioCodec 在 `board.cc` 中以单例模式初始化，使用 I2S Simplex 模式
- 输入采样率 16000Hz，输出采样率 24000Hz
- 通过 `Board::GetInstance().GetAudioCodec()` 获取音频编解码器实例
- `AudioCodec::OutputData(vector<int16_t>&)` 输出音频到扬声器
- `AudioCodec::EnableOutput(bool)` 控制功放使能
- `AudioCodec::SetOutputVolume(int)` 设置音量（0-100）

## SPIFFS 挂载

项目中 `audio` 和 `image` 两个 SPIFFS 分区：

- `image` 分区在 CMakeLists.txt 中用 `spiffs_create_partition_assets` 自动处理，支持 mmap 方式读取 GIF
- `audio` 分区在xiaozhi_esp32下的CMakeLists.txt中同样方式处理

## 持久化存储（NVS）

设置和配置通过 NVS 持久化，命名空间为常量字符串：

```c
nvs_handle_t handle;
nvs_open("settings", NVS_READWRITE, &handle);
nvs_set_u8(handle, "key", value);
nvs_commit(handle);
nvs_close(handle);
```

常用命名空间：`"settings"`（设置）、`"board"`（UUID）

## 字体图标

项目使用 FontAwesome 字体图标，定义了以下尺寸：

| 字体 | 声明宏 | 大小 | 用途 |
|------|--------|------|------|
| font_awesome_64 | `FONT_SYMBOL_64_xxx` | 64px | App 图标 |
| font_awesome_24 | - | 24px | 标题栏按钮 |
| font_awesome_18 | `FONT_SYMBOL_18_xxx` | 18px | 电池图标 |
| font_awesome_16 | `FONT_SYMBOL_16_xxx` | 16px | 列表/状态图标 |
| han_sans_cn_medium_16 | - | 16px | 中文显示 |

LVGL 内置符号也可直接使用（如 `LV_SYMBOL_PLAY`、`LV_SYMBOL_WIFI` 等）。

## 调试与日志

```c
#include "esp_log.h"
#define TAG "模块名"
ESP_LOGI(TAG, "信息: %d", value);
ESP_LOGW(TAG, "警告");
ESP_LOGE(TAG, "错误");
```

## 构建命令

```bash
# 设置编译环境
idf-get

# 配置项目（menuconfig）
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py flash

# 监控串口输出
idf.py monitor
```
