// main/app_home.c —— 护照首页：黑底圆角卡 + 左侧红条（文字向左旋转 90°）+ 居中资料。
#include "app.h"

#include <stdio.h>

#include "app_profile.h"
#include "app_ui.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_timer.h"
#include "lvgl.h"

static lv_obj_t *s_scr;
static lv_obj_t *s_card;
static lv_obj_t *s_batt;
static lv_obj_t *s_name;
static lv_obj_t *s_title;
static lv_obj_t *s_bio;
static lv_obj_t *s_side_text;
static lv_obj_t *s_avatar_host;
static lv_obj_t *s_hint;
static esp_timer_handle_t s_batt_timer;

static void batt_tick(void *arg)
{
    (void)arg;
    if (!bsp_lvgl_lock(100)) {
        return;
    }
    if (s_batt) {
        app_ui_topbar_set_battery(s_batt, bsp_battery_soc());
    }
    bsp_lvgl_unlock();
}

static void rebuild_avatar(void)
{
    if (!s_avatar_host) {
        return;
    }
    lv_obj_clean(s_avatar_host);
    const app_profile_t *p = app_profile_get();
    app_ui_draw_avatar(s_avatar_host, 0, 0, 72, p ? p->avatar_idx : 0);
}

// 红条文案：横排字符串再整体向左旋转 90°（与设计稿一致），并在条内居中。
static void update_side_text(const char *passport_id)
{
    if (!s_side_text) {
        return;
    }
    if (passport_id && passport_id[0]) {
        lv_label_set_text_fmt(s_side_text, "笑场通行证  %s", passport_id);
    } else {
        lv_label_set_text(s_side_text, "笑场通行证");
    }
    lv_obj_set_style_transform_angle(s_side_text, 0, 0);
    lv_obj_update_layout(s_side_text);
    lv_coord_t w = lv_obj_get_width(s_side_text);
    lv_coord_t h = lv_obj_get_height(s_side_text);
    lv_obj_set_style_transform_pivot_x(s_side_text, w / 2, 0);
    lv_obj_set_style_transform_pivot_y(s_side_text, h / 2, 0);
    // LVGL 角度单位 0.1°，-900 = 向左旋转 90°
    lv_obj_set_style_transform_angle(s_side_text, -900, 0);
    lv_obj_align(s_side_text, LV_ALIGN_CENTER, 0, 0);
}

void app_home_refresh(void)
{
    if (!s_card) {
        return;
    }
    const app_profile_t *p = app_profile_get();
    if (!p) {
        return;
    }
    if (s_name) {
        lv_label_set_text(s_name, p->name);
    }
    if (s_title) {
        lv_label_set_text(s_title, p->title);
    }
    if (s_bio) {
        lv_label_set_text(s_bio, p->bio);
    }
    update_side_text(p->passport_id);
    rebuild_avatar();
}

void app_home_enter(void)
{
    s_scr = app_ui_screen_create();
    s_card = app_ui_card(s_scr);

    lv_obj_t *bar = app_ui_topbar_create(s_card);
    s_batt = (lv_obj_t *)lv_obj_get_user_data(bar);

    lv_obj_t *red = lv_obj_create(s_card);
    lv_obj_remove_flag(red, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(red, 0, 0);
    lv_obj_set_size(red, APP_RED_W, APP_SCREEN_H);
    lv_obj_set_style_bg_color(red, lv_color_hex(APP_COL_RED), 0);
    lv_obj_set_style_border_width(red, 0, 0);
    lv_obj_set_style_radius(red, 0, 0);
    lv_obj_set_style_pad_all(red, 0, 0);
    lv_obj_set_style_clip_corner(red, false, 0);

    s_side_text = app_ui_label(red, "笑场通行证", 0xFFFFFF);
    lv_obj_set_style_text_align(s_side_text, LV_TEXT_ALIGN_CENTER, 0);

    s_avatar_host = lv_obj_create(s_card);
    lv_obj_remove_flag(s_avatar_host, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_avatar_host, 72, 72);
    lv_obj_set_style_bg_opa(s_avatar_host, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_avatar_host, 0, 0);
    lv_obj_set_style_pad_all(s_avatar_host, 0, 0);
    lv_obj_set_pos(s_avatar_host, 84, 44);

    s_name = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_style_text_align(s_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_name, 180);
    lv_obj_set_pos(s_name, 30, 128);

    s_title = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_title, 180);
    lv_obj_set_pos(s_title, 30, 152);

    lv_obj_t *rule1 = lv_obj_create(s_card);
    lv_obj_remove_flag(rule1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(rule1, 50, 178);
    lv_obj_set_size(rule1, 140, 1);
    lv_obj_set_style_bg_color(rule1, lv_color_hex(APP_COL_LINE), 0);
    lv_obj_set_style_border_width(rule1, 0, 0);
    lv_obj_set_style_radius(rule1, 0, 0);

    s_bio = app_ui_label(s_card, "", APP_COL_MUTED);
    lv_obj_set_style_text_align(s_bio, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_bio, 180);
    lv_label_set_long_mode(s_bio, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(s_bio, 30, 188);

    lv_obj_t *rule2 = lv_obj_create(s_card);
    lv_obj_remove_flag(rule2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(rule2, 50, 248);
    lv_obj_set_size(rule2, 140, 1);
    lv_obj_set_style_bg_color(rule2, lv_color_hex(APP_COL_LINE), 0);
    lv_obj_set_style_border_width(rule2, 0, 0);
    lv_obj_set_style_radius(rule2, 0, 0);

    s_hint = app_ui_label(s_card, "任意键开始笑  长按确定设置", APP_COL_MUTED);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_hint, 200);
    lv_obj_set_style_text_opa(s_hint, LV_OPA_70, 0);
    lv_obj_set_pos(s_hint, 20, 286);

    app_home_refresh();
    lv_screen_load(s_scr);

    if (!s_batt_timer) {
        const esp_timer_create_args_t args = {
            .callback = batt_tick,
            .name = "home_batt",
        };
        esp_timer_create(&args, &s_batt_timer);
    }
    esp_timer_start_periodic(s_batt_timer, 2000000);
}

void app_home_exit(void)
{
    if (s_batt_timer) {
        esp_timer_stop(s_batt_timer);
    }
    if (s_scr) {
        lv_obj_delete(s_scr);
    }
    s_scr = NULL;
    s_card = NULL;
    s_batt = NULL;
    s_name = NULL;
    s_title = NULL;
    s_bio = NULL;
    s_side_text = NULL;
    s_avatar_host = NULL;
    s_hint = NULL;
}

void app_home_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (btn == BSP_BTN_OK && ev == BSP_BTN_LONG) {
        app_goto(APP_PAGE_SETTINGS);
        return;
    }
    if (ev == BSP_BTN_CLICK) {
        (void)btn;
        app_goto(APP_PAGE_JOKE);
    }
}
