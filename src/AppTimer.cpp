#include <Arduino.h>
#include "AppTimer.h"

static lv_obj_t *timer_screen;
static lv_obj_t *time_label;
static lv_obj_t *status_label;
static lv_obj_t *progress_bar;

static int set_minutes = 5;
static uint32_t target_ms = 0;
static uint32_t total_ms = 0;
static uint32_t remaining_ms = 0; // ← ADD THIS to track remaining time
static bool is_running = false;
static bool timer_finished = false;

static void anim_size_cb(void *var, int32_t v)
{
    lv_obj_set_style_transform_zoom((lv_obj_t *)var, v, 0);
}

static void anim_blink_cb(void *var, int32_t v)
{
    lv_obj_set_style_bg_opa((lv_obj_t *)var, v, 0);
}
// Add callback for buzzer
static void (*timer_alarm_callback)() = nullptr;

void AppTimer_SetAlarmCallback(void (*callback)())
{
    timer_alarm_callback = callback;
}

void AppTimer_Init()
{
    timer_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(timer_screen, lv_color_hex(0x000000), 0);

    lv_obj_t *header = lv_label_create(timer_screen);
    lv_label_set_text(header, "TIMER");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 15);

    progress_bar = lv_bar_create(timer_screen);
    lv_obj_set_size(progress_bar, 100, 8);
    lv_obj_align(progress_bar, LV_ALIGN_TOP_MID, 0, 50);
    lv_bar_set_value(progress_bar, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x00D9FF), LV_PART_INDICATOR);

    time_label = lv_label_create(timer_screen);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text_fmt(time_label, "%02d:00", set_minutes);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -10);

    status_label = lv_label_create(timer_screen);
    lv_label_set_text(status_label, LV_SYMBOL_UP LV_SYMBOL_DOWN " Set " LV_SYMBOL_PLAY " Start");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void AppTimer_Adjust(int minutes)
{
    if (is_running)
        return;

    if (timer_finished)
    {
        lv_obj_set_style_bg_color(timer_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(timer_screen, LV_OPA_COVER, 0);
        timer_finished = false;
    }

    set_minutes += minutes;
    if (set_minutes < 1)
        set_minutes = 1;
    if (set_minutes > 99)
        set_minutes = 99;

    lv_label_set_text_fmt(time_label, "%02d:00", set_minutes);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
    lv_bar_set_value(progress_bar, 100, LV_ANIM_OFF);
    remaining_ms = 0; // Reset remaining time

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, time_label);
    lv_anim_set_values(&a, 256, 300);
    lv_anim_set_time(&a, 100);
    lv_anim_set_playback_time(&a, 100);
    lv_anim_set_exec_cb(&a, anim_size_cb);
    lv_anim_start(&a);
}

void AppTimer_Toggle()
{
    if (timer_finished)
    {
        lv_obj_set_style_bg_color(timer_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(timer_screen, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
        timer_finished = false;
        remaining_ms = 0;
        lv_label_set_text_fmt(time_label, "%02d:00", set_minutes);
        lv_bar_set_value(progress_bar, 100, LV_ANIM_OFF);
        lv_label_set_text(status_label, LV_SYMBOL_UP LV_SYMBOL_DOWN " Set " LV_SYMBOL_PLAY " Start");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x888888), 0);
        return;
    }

    if (!is_running)
    {
        // Starting or Resuming
        if (remaining_ms > 0)
        {
            // RESUME from where we paused
            target_ms = millis() + remaining_ms;
        }
        else
        {
            // START fresh
            target_ms = millis() + (set_minutes * 60000UL);
            total_ms = set_minutes * 60000UL;
        }
        is_running = true;
        lv_label_set_text(status_label, LV_SYMBOL_PAUSE " Stop");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF88), 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0x00D9FF), 0);
    }
    else
    {
        // PAUSE - save remaining time
        remaining_ms = target_ms - millis();
        if (remaining_ms < 0)
            remaining_ms = 0;

        is_running = false;
        lv_label_set_text(status_label, LV_SYMBOL_PLAY " Resume");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFAA00), 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFAA00), 0);
    }
}

void AppTimer_Update()
{
    if (!is_running)
        return;

    long diff = target_ms - millis();

    if (diff <= 0)
    {
        is_running = false;
        timer_finished = true;
        remaining_ms = 0;
        lv_label_set_text(time_label, "00:00");
        lv_label_set_text(status_label, LV_SYMBOL_WARNING " TIME UP!");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0xFF0000), 0);
        lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
        // TRIGGER ALARM SOUND
        if (timer_alarm_callback != nullptr)
        {
            timer_alarm_callback();
        }

        lv_obj_set_style_bg_color(timer_screen, lv_color_hex(0xFF0000), 0);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, timer_screen);
        lv_anim_set_values(&a, LV_OPA_0, LV_OPA_50);
        lv_anim_set_time(&a, 500);
        lv_anim_set_playback_time(&a, 500);
        lv_anim_set_repeat_count(&a, 5);
        lv_anim_set_exec_cb(&a, anim_blink_cb);
        lv_anim_start(&a);

        return;
    }

    int mins = diff / 60000;
    int secs = (diff % 60000) / 1000;
    lv_label_set_text_fmt(time_label, "%02d:%02d", mins, secs);

    int progress = (int)((diff * 100) / total_ms);
    lv_bar_set_value(progress_bar, progress, LV_ANIM_OFF);

    if (diff < 10000)
    {
        lv_obj_set_style_text_color(time_label, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
    }
}

lv_obj_t *AppTimer_GetScreen()
{
    return timer_screen;
}