#include "lvgl.h"
#include "font/my_fonts.h"
#include "ui/ui.h"
#include "style/my_style.h"
#include <string.h>

/* ==================== 配置参数 ==================== */
#define REC_MAX_DURATION_MS     60000   // 最大录音时间 60秒
#define REC_MIN_DURATION_MS     1000    // 最小录音时间 1秒
#define REC_TIMER_PERIOD_MS     10      // 定时器周期 10ms

/* ==================== 录音状态 ==================== */
typedef enum {
    REC_STATE_IDLE,         // 空闲状态
    REC_STATE_RECORDING,    // 录音中
    REC_STATE_PLAYING,      // 播放中
} rec_state_t;

/* ==================== 硬件抽象接口 ==================== */
typedef struct {
    bool (*init)(void);
    bool (*start_record)(void);
    bool (*stop_record)(void);
    bool (*start_play)(void);
    bool (*stop_play)(void);
    uint32_t (*get_record_time_ms)(void);
    uint32_t (*get_play_time_ms)(void);
    uint32_t (*get_total_time_ms)(void);
    void (*set_volume)(uint8_t vol);
} recorder_hal_t;

/* ==================== 静态变量 ==================== */
static lv_obj_t* label_time;
static lv_obj_t* btn_record;
static lv_obj_t* btn_play;
static lv_obj_t* progress_bar;
static lv_obj_t* msgbox;
static lv_obj_t* label_status;
static lv_timer_t* rec_timer;

static rec_state_t current_state = REC_STATE_IDLE;
static bool has_recording = false;
static uint32_t record_duration_ms = 0;

/* 硬件接口实例（默认使用模拟实现） */
static const recorder_hal_t* hal = NULL;

/* ==================== 硬件模拟实现（用于PC模拟） ==================== */
static uint32_t mock_record_start_time = 0;
static uint32_t mock_play_start_time = 0;
static uint32_t mock_current_play_time = 0;
static uint32_t mock_total_time = 0;

static bool mock_init(void)
{
    return true;
}

static bool mock_start_record(void)
{
    mock_record_start_time = lv_tick_get();
    return true;
}

static bool mock_stop_record(void)
{
    mock_total_time = lv_tick_get() - mock_record_start_time;
    return true;
}

static bool mock_start_play(void)
{
    mock_play_start_time = lv_tick_get();
    mock_current_play_time = 0;
    return true;
}

static bool mock_stop_play(void)
{
    return true;
}

static uint32_t mock_get_record_time_ms(void)
{
    return lv_tick_get() - mock_record_start_time;
}

static uint32_t mock_get_play_time_ms(void)
{
    mock_current_play_time = lv_tick_get() - mock_play_start_time;
    if (mock_current_play_time > mock_total_time) {
        mock_current_play_time = mock_total_time;
    }
    return mock_current_play_time;
}

static uint32_t mock_get_total_time_ms(void)
{
    return mock_total_time;
}

static void mock_set_volume(uint8_t vol)
{
    (void)vol;
}

static const recorder_hal_t mock_hal = {
    .init = mock_init,
    .start_record = mock_start_record,
    .stop_record = mock_stop_record,
    .start_play = mock_start_play,
    .stop_play = mock_stop_play,
    .get_record_time_ms = mock_get_record_time_ms,
    .get_play_time_ms = mock_get_play_time_ms,
    .get_total_time_ms = mock_get_total_time_ms,
    .set_volume = mock_set_volume,
};

/* ==================== 工具函数 ==================== */

/**
 * @brief 更新录音状态显示
 */
static void update_status_display(void)
{
    if (has_recording) {
        lv_label_set_text(label_status, LV_SYMBOL_OK"Saved");
        lv_obj_add_style(label_status, &style_text_color_green, LV_PART_MAIN);
    } else {
        lv_label_set_text(label_status, LV_SYMBOL_CLOSE"Empty");
        lv_obj_remove_style(label_status, &style_text_color_green, LV_PART_MAIN);
    }
}

/**
 * @brief 更新按钮状态
 */
static void update_button_states(void)
{
    switch (current_state) {
        case REC_STATE_IDLE:
            lv_obj_remove_style(btn_record, &style_recorder_btn_running, LV_PART_MAIN);
            lv_label_set_text(lv_obj_get_child(btn_record, 0), "Record");
            lv_obj_remove_state(btn_record, LV_STATE_DISABLED);

            lv_obj_remove_style(btn_play, &style_recorder_btn_running, LV_PART_MAIN);
            lv_label_set_text(lv_obj_get_child(btn_play, 0), "Play");
            if (has_recording) {
                lv_obj_remove_state(btn_play, LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(btn_play, LV_STATE_DISABLED);
            }
            update_status_display();
            break;

        case REC_STATE_RECORDING:
            lv_obj_add_style(btn_record, &style_recorder_btn_running, LV_PART_MAIN);
            lv_label_set_text(lv_obj_get_child(btn_record, 0), "Stop");
            lv_obj_remove_state(btn_record, LV_STATE_DISABLED);
            lv_obj_add_state(btn_play, LV_STATE_DISABLED);
            break;

        case REC_STATE_PLAYING:
            lv_obj_add_state(btn_record, LV_STATE_DISABLED);
            lv_obj_add_style(btn_play, &style_recorder_btn_running, LV_PART_MAIN);
            lv_label_set_text(lv_obj_get_child(btn_play, 0), "Stop");
            lv_obj_remove_state(btn_play, LV_STATE_DISABLED);
            break;
    }
}

/**
 * @brief 更新时间显示
 */
static void update_time_display(uint32_t ms)
{
    uint16_t minutes = ms / 60000;
    uint8_t seconds = (ms % 60000) / 1000;
    uint8_t centis = (ms % 1000) / 10;

    lv_label_set_text_fmt(label_time, "%02u:%02u.%02u", minutes, seconds, centis);
}

/**
 * @brief 显示录音时间太短弹窗
 */
static void show_too_short_dialog(void)
{
    msgbox = my_msgbox_create(false);
    lv_msgbox_add_title(msgbox, "Error");
    lv_msgbox_add_text(msgbox, "The recording time is too short.");

    add_path(msgbox);
}

/* ==================== 录音控制函数 ==================== */

/**
 * @brief 开始录音
 */
static void start_recording(void)
{
    if (hal == NULL) return;

    hal->start_record();
    current_state = REC_STATE_RECORDING;
    has_recording = false;
    record_duration_ms = 0;

    update_time_display(0);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    update_button_states();

    lv_timer_resume(rec_timer);
}

/**
 * @brief 停止录音并检查时长
 */
static void stop_recording(void)
{
    if (hal == NULL) return;

    record_duration_ms = hal->get_record_time_ms();
    hal->stop_record();

    if (record_duration_ms < REC_MIN_DURATION_MS) {
        // 录音时间太短，不保存
        current_state = REC_STATE_IDLE;
        has_recording = false;
        update_time_display(0);
        update_button_states();
        show_too_short_dialog();
    } else {
        // 保存录音
        current_state = REC_STATE_IDLE;
        has_recording = true;
        update_time_display(record_duration_ms);
        update_button_states();
    }
}

/**
 * @brief 开始播放
 */
static void start_playback(void)
{
    if (hal == NULL || !has_recording) return;

    hal->start_play();
    current_state = REC_STATE_PLAYING;

    update_time_display(0);
    update_button_states();

    lv_timer_resume(rec_timer);
}

/**
 * @brief 停止播放
 */
static void stop_playback(void)
{
    if (hal == NULL) return;

    hal->stop_play();
    current_state = REC_STATE_IDLE;

    update_time_display(record_duration_ms);
    update_button_states();
}

/* ==================== 事件回调 ==================== */

/**
 * @brief 定时器回调
 */
static void timer_cb(lv_timer_t* timer)
{
    (void)timer;

    switch (current_state) {
        case REC_STATE_RECORDING: {
            uint32_t elapsed = hal->get_record_time_ms();
            update_time_display(elapsed);

            // 检查是否达到最大录音时间
            if (elapsed >= REC_MAX_DURATION_MS) {
                stop_recording();
            }
            break;
        }

        case REC_STATE_PLAYING: {
            uint32_t play_time = hal->get_play_time_ms();
            uint32_t total_time = hal->get_total_time_ms();

            update_time_display(play_time);

            // 更新进度条
            if (total_time > 0) {
                int32_t progress = (int32_t)((play_time * 100) / total_time);
                lv_bar_set_value(progress_bar, progress, LV_ANIM_OFF);
            }

            // 检查播放是否完成
            if (play_time >= total_time) {
                stop_playback();
            }
            break;
        }

        default:
            break;
    }
}

/**
 * @brief 录音按钮点击回调
 */
static void record_btn_cb(lv_event_t* e)
{
    (void)e;

    switch (current_state) {
        case REC_STATE_IDLE:
            start_recording();
            break;
        case REC_STATE_RECORDING:
            stop_recording();
            break;
        default:
            break;
    }
}

/**
 * @brief 播放按钮点击回调
 */
static void play_btn_cb(lv_event_t* e)
{
    (void)e;

    switch (current_state) {
        case REC_STATE_IDLE:
            start_playback();
            break;
        case REC_STATE_PLAYING:
            stop_playback();
            break;
        default:
            break;
    }
}

/* ==================== 硬件接口设置 ==================== */

/**
 * @brief 设置硬件抽象接口
 * @param hal_interface 硬件接口结构体指针，传入NULL使用默认模拟实现
 */
void recorder_set_hal(const recorder_hal_t* hal_interface)
{
    if (hal_interface != NULL) {
        hal = hal_interface;
    } else {
        hal = &mock_hal;
    }

    if (hal->init) {
        hal->init();
    }
}

/* ==================== 界面初始化 ==================== */

void app_init_recorder(void)
{
    // 初始化硬件接口（默认使用模拟实现）
    recorder_set_hal(NULL);

    // 创建屏幕
    lv_obj_t* screen = my_screen_create("Recorder", NULL);


    lv_obj_t* content = my_screen_get_content(screen);

    // 创建时间显示标签
    label_time = lv_label_create(content);
    lv_label_set_text(label_time, "00:00.00");
    lv_obj_add_style(label_time, &style_recorder_label_time, LV_PART_MAIN);

    // 创建录音按钮
    btn_record = lv_button_create(content);
    lv_obj_add_event_cb(btn_record, record_btn_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_style(btn_record, &style_recorder_btn_record, LV_PART_MAIN);

    lv_obj_t* label_record = lv_label_create(btn_record);
    lv_label_set_text(label_record, "Record");
    lv_obj_add_style(label_record, &style_recorder_btn_text, LV_PART_MAIN);

    // 创建播放按钮
    btn_play = lv_button_create(content);
    lv_obj_add_event_cb(btn_play, play_btn_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_state(btn_play, LV_STATE_DISABLED);
    lv_obj_add_style(btn_play, &style_recorder_btn_play, LV_PART_MAIN);

    lv_obj_t* label_play = lv_label_create(btn_play);
    lv_label_set_text(label_play, "Play");
    lv_obj_add_style(label_play, &style_recorder_btn_text, LV_PART_MAIN);

    // 创建进度条
    progress_bar = lv_bar_create(content);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_obj_add_style(progress_bar, &style_recorder_progress_bar, LV_PART_MAIN);

    // 创建录音状态标签
    label_status = lv_label_create(content);
    lv_label_set_text(label_status, LV_SYMBOL_CLOSE"Empty");
    lv_obj_add_style(label_status, &style_recorder_label_status, LV_PART_MAIN);

    // 创建定时器（初始暂停）
    rec_timer = lv_timer_create(timer_cb, REC_TIMER_PERIOD_MS, NULL);
    lv_timer_pause(rec_timer);

    add_path(screen);
}
