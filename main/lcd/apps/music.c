#include "esp_log.h"
#include "font/my_fonts.h"
#include "lvgl.h"
#include "lvgl_private.h"
#include "style/my_style.h"
#include "ui/ui.h"
#include <stdlib.h>
#include <string.h>

#define APP_NAME "Music"

#define SONG_NAME_MAX 64
#define ARTIST_MAX    64

typedef struct {
    char song_name[SONG_NAME_MAX];
    char artist[ARTIST_MAX];
    uint32_t time_s;
} song_info_t;

static lv_obj_t* list_music;

static lv_obj_t* label_title;
static lv_obj_t* label_artist;
static lv_obj_t* bar_progress;
static lv_obj_t* label_time;
static lv_obj_t* label_lyric_next;

static void btn_prev_event_cb(lv_event_t* e)
{
    ESP_LOGI(APP_NAME, "prev music\n");
}

static void btn_play_event_cb(lv_event_t* e)
{
    ESP_LOGI(APP_NAME, "music start play");
}

static void btn_next_event_cb(lv_event_t* e)
{
    ESP_LOGI(APP_NAME, "next music");
}

static void music_screen_player_init(song_info_t* song)
{
    lv_obj_t* screen_player = my_screen_create(song->song_name, NULL);
    lv_obj_t* content = my_screen_get_content(screen_player);

    // ===== 顶部：歌曲信息 =====
    label_title = my_screen_get_title(screen_player);

    // 艺术家
    label_artist = lv_label_create(content);
    lv_obj_add_style(label_artist, &style_music_label_artist, LV_PART_MAIN);
    lv_label_set_text_static(label_artist, song->artist);

    // ===== 中部：歌词显示区域 =====
    lv_obj_t* lyric_container = lv_obj_create(content);
    lv_obj_add_style(lyric_container, &style_music_lyric_container, LV_PART_MAIN);
    lv_obj_remove_flag(lyric_container, LV_OBJ_FLAG_SCROLLABLE);

    // 当前歌词（高亮）
    lv_obj_t* label_lyric = lv_label_create(lyric_container);
    lv_obj_add_style(label_lyric, &style_music_label_lyric, LV_PART_MAIN);
    lv_label_set_long_mode(label_lyric, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_lyric, "current lyric");

    // 下一句歌词（暗淡）
    label_lyric_next = lv_label_create(lyric_container);
    lv_obj_add_style(label_lyric_next, &style_music_label_lyric_next, LV_PART_MAIN);
    lv_label_set_long_mode(label_lyric_next, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_lyric_next, "next lyric");

    // ===== 底部：进度条和控制按钮 =====

    // 进度条
    bar_progress = lv_bar_create(content);
    lv_obj_add_style(bar_progress, &style_music_bar_progress, LV_PART_MAIN);
    lv_obj_add_style(bar_progress, &style_music_bar_progress_indicator, LV_PART_INDICATOR);
    lv_bar_set_range(bar_progress, 0, 1000); // 0.0 ~ 1.0 * 1000
    lv_bar_set_value(bar_progress, 200, LV_ANIM_OFF);

    // 时间标签
    label_time = lv_label_create(content);
    lv_obj_add_style(label_time, &style_music_label_time, LV_PART_MAIN);
    lv_label_set_text(label_time, "0:00");

    // 总时间标签
    lv_obj_t* label_full_time = lv_label_create(content);
    lv_obj_add_style(label_full_time, &style_music_label_time_full, LV_PART_MAIN);
    lv_label_set_text_fmt(label_full_time, "%u:%02u", (unsigned int)song->time_s / 60, (unsigned int)song->time_s % 60);

    // 进度条拖动事件
    // lv_obj_add_event_cb(slider_progress, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 按钮容器
    lv_obj_t* btn_container = lv_obj_create(content);
    lv_obj_add_style(btn_container, &style_music_btn_container, LV_PART_MAIN);
    lv_obj_remove_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);

    // 上一首按钮
    lv_obj_t* btn_prev = lv_button_create(btn_container);
    lv_obj_add_style(btn_prev, &style_music_btn_prev, LV_PART_MAIN);
    lv_obj_t* label_prev = lv_label_create(btn_prev);
    lv_obj_add_style(label_prev, &style_text_color_back, LV_PART_MAIN);
    lv_label_set_text(label_prev, LV_SYMBOL_PREV);
    lv_obj_add_event_cb(btn_prev, btn_prev_event_cb, LV_EVENT_CLICKED, NULL);

    // 播放/暂停按钮
    lv_obj_t* btn_play = lv_button_create(btn_container);
    lv_obj_add_style(btn_play, &style_music_btn_play, LV_PART_MAIN);
    lv_obj_t* label_play = lv_label_create(btn_play);
    lv_obj_center(label_play);
    lv_label_set_text(label_play, LV_SYMBOL_PLAY);
    lv_obj_add_event_cb(btn_play, btn_play_event_cb, LV_EVENT_CLICKED, label_play);

    // 下一首按钮
    lv_obj_t* btn_next = lv_button_create(btn_container);
    lv_obj_add_style(btn_next, &style_music_btn_prev, LV_PART_MAIN);
    lv_obj_t* label_next = lv_label_create(btn_next);
    lv_obj_add_style(label_next, &style_text_color_back, LV_PART_MAIN);
    lv_label_set_text(label_next, LV_SYMBOL_NEXT);
    lv_obj_add_event_cb(btn_next, btn_next_event_cb, LV_EVENT_CLICKED, NULL);

    add_path(screen_player);
}

static void music_list_btn_cb(lv_event_t* e)
{
    song_info_t* song = lv_event_get_user_data(e);
    music_screen_player_init(song);
}

static void music_list_add_song(const char* song_name, const char* artist, uint32_t time_s)
{
    song_info_t* song = malloc(sizeof(song_info_t));
    if (!song) return;
    strncpy(song->song_name, song_name, SONG_NAME_MAX - 1);
    song->song_name[SONG_NAME_MAX - 1] = '\0';
    strncpy(song->artist, artist, ARTIST_MAX - 1);
    song->artist[ARTIST_MAX - 1] = '\0';
    song->time_s = time_s;

    lv_obj_t* btn = lv_list_add_button(list_music, NULL, song->song_name);
    lv_obj_add_event_cb(btn, music_list_btn_cb, LV_EVENT_PRESSED, song);
    btn->user_data = song;
    lv_obj_t* label_artist = lv_label_create(btn);
    lv_obj_add_style(label_artist, &style_music_list_label_artist, LV_PART_MAIN);
    lv_label_set_text(label_artist, song->artist);
}

static void music_list_exit_cb(void)
{
    for (int i = 0; i < lv_obj_get_child_count(list_music); i++)
    {
        song_info_t* song = lv_obj_get_child(list_music, i)->user_data;
        if (song)
        {
            free(song);
        }
    }
}

void app_init_music(void)
{
    lv_obj_t* screen_list = my_screen_create(APP_NAME, music_list_exit_cb);
    list_music = lv_list_create(my_screen_get_content(screen_list));
    lv_obj_add_style(list_music, &style_wifi_list, LV_PART_MAIN);

    music_list_add_song("Long Time No See", "Jay Chou", 150);
    music_list_add_song("Test1", "Yuan wei", 250);
    music_list_add_song("TEST2", "asd", 90);
    music_list_add_song("Fu Shi Shan Xia", "Chen", 240);

    add_path(screen_list);
}