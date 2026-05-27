# Smart-Puppy 项目概述

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

更多内容参考[AGENTS.md](./AGENTS.md)