#include "esp_log.h"
#include "esp_mmap_assets.h"
#include "font/my_fonts.h"
#include "lvgl.h"
#include "lvgl_private.h"
#include "mmap_generate_image.h"
#include "style/my_style.h"
#include "ui/ui.h"
#include "application.h"

static lv_obj_t* label_message;
static lv_obj_t* label_status;
static lv_obj_t* gif_emotion;
static mmap_assets_handle_t asset_gif;


static void gif_mmap_init()
{
    const mmap_assets_config_t config = {
        .partition_label = "image",
        .max_files = MMAP_IMAGE_FILES,
        .checksum = MMAP_IMAGE_CHECKSUM,
        .flags = {
            .mmap_enable = true,
            .use_fs = false,
            .app_bin_check = true,
        },
    };

    mmap_assets_new(&config, &asset_gif);
    ESP_LOGI("Chat gif", "stored_files:%d", mmap_assets_get_stored_files(asset_gif));
}

static void screen_chat_exit_cb()
{
    // lv_anim_delete(label_message, NULL);
    label_message = NULL;
    label_status = NULL;
    lv_gif_pause(gif_emotion);
    gif_emotion = NULL;
    mmap_assets_del(asset_gif);
}

static void chat_stop_speak()
{
    SetMainEventBits(MAIN_EVENT_TOGGLE_CHAT);
}

void app_init_chat(void)
{
    gif_mmap_init();
    lv_obj_t* screen = my_empty_screen_create(screen_chat_exit_cb);
    lv_group_t* group = my_screen_get_group(screen);

    lv_image_dsc_t gif_dsc = {
        .data_size = mmap_assets_get_size(asset_gif, MMAP_IMAGE_EYES_SLOW_GIF),
        .data = mmap_assets_get_mem(asset_gif, MMAP_IMAGE_EYES_SLOW_GIF),
    };

    gif_emotion = lv_gif_create(screen);
    lv_gif_set_src(gif_emotion, &gif_dsc);
    lv_group_add_obj(group, gif_emotion);
    lv_obj_add_event_cb(gif_emotion, chat_stop_speak, LV_EVENT_PRESSED, NULL);

    label_message = lv_label_create(screen);
    lv_label_set_long_mode(label_message, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_add_style(label_message, &style_chat_label_message, LV_PART_MAIN);
    
    lv_obj_set_style_anim(label_message, &delay_scroll_anim, LV_PART_MAIN);
    lv_obj_set_style_anim_duration(label_message, lv_anim_speed_clamped(60, 300, 60000), LV_PART_MAIN);

    label_status = lv_label_create(screen);
    lv_obj_add_style(label_status, &style_chat_label_status, LV_PART_MAIN);
    add_path(screen);
}

void chat_set_label_status(const char* text, bool is_error)
{
    if (!label_status) return;

    lv_label_set_text(label_status, text);
    if (is_error)
    {
        lv_obj_add_style(label_status, &style_text_color_red, LV_PART_MAIN);
    }
    else
    {
        lv_obj_remove_style(label_status, &style_text_color_red, LV_PART_MAIN);
    }
}

void chat_set_label_message(const char* message)
{
    if (!label_message) return;

    lv_label_set_long_mode(label_message, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(label_message, message);
    lv_label_set_long_mode(label_message, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
}

void chat_set_gif_emotion(const char* emotion)
{
    struct emotion_gif {
        const char* emotion;
        const int gif_index;
    };
    static const struct emotion_gif emotion_gifs[] = {
        { "neutral", MMAP_IMAGE_EYES_SLOW_GIF },
        { "happy", MMAP_IMAGE_EXCITED_GIF },
        { "laughing", MMAP_IMAGE_EXCITED_GIF },
        { "funny", MMAP_IMAGE_EXCITED_GIF },
        { "sad", MMAP_IMAGE_SAD_GIF },
        { "angry", MMAP_IMAGE_ANGER_GIF },
        { "crying", MMAP_IMAGE_SAD_GIF },
        { "loving", MMAP_IMAGE_EXCITED_GIF },
        { "embarrassed", MMAP_IMAGE_RIGHT_GIF },
        { "surprised", MMAP_IMAGE_EXCITED_GIF },
        { "shocked", MMAP_IMAGE_EYES_FAST_GIF },
        { "thinking", MMAP_IMAGE_LEFT_GIF },
        { "winking", MMAP_IMAGE_EYES_FAST_GIF },
        { "cool", MMAP_IMAGE_EYES_SLOW_GIF },
        { "relaxed", MMAP_IMAGE_EYES_SLOW_GIF },
        { "delicious", MMAP_IMAGE_EXCITED_GIF },
        { "kissy", MMAP_IMAGE_EXCITED_GIF },
        { "confident", MMAP_IMAGE_EYES_SLOW_GIF },
        { "sleepy", MMAP_IMAGE_EYES_SLOW_GIF },
        { "silly", MMAP_IMAGE_DISDAIN_GIF },
        { "confused", MMAP_IMAGE_DISDAIN_GIF },
    };

    if (!gif_emotion) return;

    for (int i = 0; i < sizeof(emotion_gifs) / sizeof(struct emotion_gif); i++)
    {
        if (strcmp(emotion, emotion_gifs[i].emotion) == 0)
        {
            lv_image_dsc_t gif_dsc = {
                .data_size = mmap_assets_get_size(asset_gif, emotion_gifs[i].gif_index),
                .data = mmap_assets_get_mem(asset_gif, emotion_gifs[i].gif_index),
            };
            lv_gif_set_src(gif_emotion, &gif_dsc);
            return;
        }
    }
    ESP_LOGW("Chat", "err emotion");
}