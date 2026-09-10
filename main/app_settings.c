// main/app_settings.c —— 设置页：选预置头像 + SoftAP 网页改资料。
#include "app.h"

#include "app_config_ap.h"
#include "app_profile.h"
#include "app_ui.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

static const char *TAG = "app_settings";

static lv_obj_t *s_scr;
static lv_obj_t *s_batt;
static lv_obj_t *s_avatar_row;
static lv_obj_t *s_info;
static lv_obj_t *s_ap_info;
static int s_sel;
static int s_saved_avatar;
static bool s_dirty;
static esp_timer_handle_t s_batt_timer;

static void refresh_info(void)
{
    const app_profile_t *p = app_profile_get();
    if (!s_info || !p) {
        return;
    }
    lv_label_set_text_fmt(s_info,
                          "姓名  %s\n头衔  %s\n简介  %s\n编号  %s",
                          p->name, p->title, p->bio, p->passport_id);
}

static void refresh_ap_info(void)
{
    if (!s_ap_info) {
        return;
    }
    if (app_config_ap_is_running()) {
        lv_label_set_text_fmt(s_ap_info,
                              "热点 %s\n打开 http://192.168.4.1/",
                              app_config_ap_ssid());
    } else {
        lv_label_set_text(s_ap_info, "热点未开启");
    }
}

static void refresh_avatars(void)
{
    if (!s_avatar_row) {
        return;
    }
    lv_obj_clean(s_avatar_row);
    for (int i = 0; i < APP_AVATAR_PRESET_COUNT; i++) {
        int x = (i % 5) * 38;
        int y = (i / 5) * 38;
        lv_obj_t *cell = lv_obj_create(s_avatar_row);
        lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(cell, x, y);
        lv_obj_set_size(cell, 34, 34);
        lv_obj_set_style_radius(cell, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cell, lv_color_hex(app_ui_avatar_color(i)), 0);
        lv_obj_set_style_border_color(cell, lv_color_hex(APP_COL_INK), 0);
        lv_obj_set_style_border_width(cell, (i == s_sel) ? 2 : 1, 0);
        lv_obj_set_style_pad_all(cell, 0, 0);

        lv_obj_t *g = app_ui_label(cell, app_ui_avatar_glyph(i), APP_COL_INK);
        lv_obj_center(g);
    }
}

static void batt_tick(void *arg)
{
    (void)arg;
    if (!bsp_lvgl_lock(100)) {
        return;
    }
    if (s_batt) {
        app_ui_topbar_set_battery(s_batt, bsp_battery_soc());
    }
    // HTTP 任务只置位标志；在此持锁刷新 UI。
    if (app_config_ap_take_profile_updated()) {
        const app_profile_t *p = app_profile_get();
        if (p) {
            s_sel = p->avatar_idx;
            s_saved_avatar = s_sel;
            s_dirty = false;
        }
        refresh_avatars();
        refresh_info();
    }
    bsp_lvgl_unlock();
}

void app_settings_enter(void)
{
    const app_profile_t *p = app_profile_get();
    s_sel = p ? p->avatar_idx : 0;
    s_saved_avatar = s_sel;
    s_dirty = false;

    s_scr = app_ui_screen_create();
    lv_obj_t *bar = app_ui_topbar_create(s_scr);
    s_batt = (lv_obj_t *)lv_obj_get_user_data(bar);

    lv_obj_t *sec = app_ui_label(s_scr, "预置头像（10）", APP_COL_MUTED);
    lv_obj_set_pos(sec, 12, 32);

    s_avatar_row = lv_obj_create(s_scr);
    lv_obj_remove_flag(s_avatar_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_avatar_row, 12, 50);
    lv_obj_set_size(s_avatar_row, 216, 76);
    lv_obj_set_style_bg_opa(s_avatar_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_avatar_row, 0, 0);
    lv_obj_set_style_pad_all(s_avatar_row, 0, 0);

    s_info = app_ui_label(s_scr, "", APP_COL_INK);
    lv_obj_set_pos(s_info, 12, 130);
    lv_obj_set_width(s_info, 216);
    lv_label_set_long_mode(s_info, LV_LABEL_LONG_WRAP);

    s_ap_info = app_ui_label(s_scr, "", APP_COL_RED);
    lv_obj_set_pos(s_ap_info, 12, 210);
    lv_obj_set_width(s_ap_info, 216);
    lv_label_set_long_mode(s_ap_info, LV_LABEL_LONG_WRAP);

    lv_obj_t *hint = app_ui_label(s_scr, "上/下选头像  确定保存  长按返回", APP_COL_MUTED);
    lv_obj_set_width(hint, 220);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -6);

    refresh_avatars();
    refresh_info();
    lv_screen_load(s_scr);

    esp_err_t ap_err = app_config_ap_start();
    if (ap_err != ESP_OK) {
        ESP_LOGE(TAG, "SoftAP 启动失败: %s", esp_err_to_name(ap_err));
    }
    refresh_ap_info();

    if (!s_batt_timer) {
        const esp_timer_create_args_t args = {
            .callback = batt_tick,
            .name = "set_batt",
        };
        esp_timer_create(&args, &s_batt_timer);
    }
    esp_timer_start_periodic(s_batt_timer, 1000000);
}

void app_settings_exit(void)
{
    if (s_batt_timer) {
        esp_timer_stop(s_batt_timer);
    }
    app_config_ap_stop();
    if (s_scr) {
        lv_obj_delete(s_scr);
    }
    s_scr = NULL;
    s_batt = NULL;
    s_avatar_row = NULL;
    s_info = NULL;
    s_ap_info = NULL;
}

void app_settings_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (btn == BSP_BTN_OK && ev == BSP_BTN_LONG) {
        // 长按丢弃本页未保存的头像选择；网页已写入的资料仍保留。
        s_sel = s_saved_avatar;
        s_dirty = false;
        app_goto(APP_PAGE_HOME);
        return;
    }
    if (ev != BSP_BTN_CLICK) {
        return;
    }
    if (btn == BSP_BTN_UP) {
        s_sel = (s_sel + APP_AVATAR_PRESET_COUNT - 1) % APP_AVATAR_PRESET_COUNT;
        s_dirty = true;
        refresh_avatars();
        return;
    }
    if (btn == BSP_BTN_DOWN) {
        s_sel = (s_sel + 1) % APP_AVATAR_PRESET_COUNT;
        s_dirty = true;
        refresh_avatars();
        return;
    }
    if (btn == BSP_BTN_OK) {
        if (s_dirty) {
            app_profile_set_avatar(s_sel);
        }
        app_goto(APP_PAGE_HOME);
    }
}
