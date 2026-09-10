// main/app_home.c —— 护照首页：黑底圆角卡 + 左侧红条顶满 + 居中资料。
#include "app.h"

#include <stdio.h>
#include <string.h>

#include "app_profile.h"
#include "app_ui.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_timer.h"
#include "lvgl.h"

// 姓名目标约 36px：字库仅 16px，用 zoom≈2.25 倍放大。
#define NAME_ZOOM ((256 * 36) / 16)
#define HINT_ZOOM 168

static lv_obj_t *s_scr;
static lv_obj_t *s_card;
static lv_obj_t *s_batt;
static lv_obj_t *s_name;
static lv_obj_t *s_name_bold;  // 错位叠加，模拟加粗
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

// 红条竖排文案：每字一行，左右居中。
static void set_side_vertical(lv_obj_t *lab, const char *passport_id)
{
    static const char *title_lines[] = {"笑", "场", "通", "行", "证", NULL};
    char buf[128];
    size_t n = 0;
    buf[0] = '\0';
    for (int i = 0; title_lines[i]; i++) {
        n += (size_t)snprintf(buf + n, sizeof(buf) - n, "%s%s", i ? "\n" : "", title_lines[i]);
    }
    if (passport_id && passport_id[0]) {
        n += (size_t)snprintf(buf + n, sizeof(buf) - n, "\n");
        for (const char *c = passport_id; *c && n + 3 < sizeof(buf); c++) {
            n += (size_t)snprintf(buf + n, sizeof(buf) - n, "\n%c", *c);
        }
    }
    lv_label_set_text(lab, buf);
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
    if (s_name_bold) {
        lv_label_set_text(s_name_bold, p->name);
    }
    if (s_title) {
        lv_label_set_text(s_title, p->title);
    }
    if (s_bio) {
        lv_label_set_text(s_bio, p->bio);
    }
    if (s_side_text) {
        set_side_vertical(s_side_text, p->passport_id);
        lv_obj_align(s_side_text, LV_ALIGN_TOP_MID, 0, 8);
    }
    rebuild_avatar();
}

void app_home_enter(void)
{
    s_scr = app_ui_screen_create();
    s_card = app_ui_card(s_scr);

    lv_obj_t *bar = app_ui_topbar_create(s_card);
    s_batt = (lv_obj_t *)lv_obj_get_user_data(bar);

    // 红条顶到最上、通到最下；父卡 clip + 圆角，左上/左下自然成圆。
    lv_obj_t *red = lv_obj_create(s_card);
    lv_obj_remove_flag(red, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(red, 0, 0);
    lv_obj_set_size(red, APP_RED_W, APP_SCREEN_H);
    lv_obj_set_style_bg_color(red, lv_color_hex(APP_COL_RED), 0);
    lv_obj_set_style_border_width(red, 0, 0);
    lv_obj_set_style_radius(red, 0, 0);
    lv_obj_set_style_pad_all(red, 0, 0);

    s_side_text = app_ui_label(red, "", 0xFFFFFF);
    lv_obj_set_width(s_side_text, APP_RED_W - 2);
    lv_obj_set_style_text_align(s_side_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_side_text, 1, 0);

    // 主内容区相对整卡水平居中（不因红条右移）。
    s_avatar_host = lv_obj_create(s_card);
    lv_obj_remove_flag(s_avatar_host, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_avatar_host, 72, 72);
    lv_obj_set_style_bg_opa(s_avatar_host, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_avatar_host, 0, 0);
    lv_obj_set_style_pad_all(s_avatar_host, 0, 0);
    lv_obj_align(s_avatar_host, LV_ALIGN_TOP_MID, 0, 48);

    // 姓名：放大 + 轻微错位叠字模拟加粗
    s_name = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_style_text_align(s_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_name, 200);
    lv_obj_set_style_transform_zoom(s_name, NAME_ZOOM, 0);
    lv_obj_align(s_name, LV_ALIGN_TOP_MID, 0, 132);

    s_name_bold = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_style_text_align(s_name_bold, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_name_bold, 200);
    lv_obj_set_style_transform_zoom(s_name_bold, NAME_ZOOM, 0);
    lv_obj_align(s_name_bold, LV_ALIGN_TOP_MID, 1, 132);

    s_title = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_title, 200);
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 188);

    lv_obj_t *rule1 = lv_obj_create(s_card);
    lv_obj_remove_flag(rule1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(rule1, 140, 1);
    lv_obj_set_style_bg_color(rule1, lv_color_hex(APP_COL_LINE), 0);
    lv_obj_set_style_border_width(rule1, 0, 0);
    lv_obj_set_style_radius(rule1, 0, 0);
    lv_obj_align(rule1, LV_ALIGN_TOP_MID, 0, 214);

    s_bio = app_ui_label(s_card, "", APP_COL_MUTED);
    lv_obj_set_style_text_align(s_bio, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_bio, 180);
    lv_label_set_long_mode(s_bio, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_bio, LV_ALIGN_TOP_MID, 0, 224);

    lv_obj_t *rule2 = lv_obj_create(s_card);
    lv_obj_remove_flag(rule2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(rule2, 140, 1);
    lv_obj_set_style_bg_color(rule2, lv_color_hex(APP_COL_LINE), 0);
    lv_obj_set_style_border_width(rule2, 0, 0);
    lv_obj_set_style_radius(rule2, 0, 0);
    lv_obj_align(rule2, LV_ALIGN_TOP_MID, 0, 268);

    s_hint = app_ui_label(s_card, "任意键开始笑，长按确定进入设置", APP_COL_MUTED);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_hint, 200);
    lv_label_set_long_mode(s_hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_transform_zoom(s_hint, HINT_ZOOM, 0);
    lv_obj_align(s_hint, LV_ALIGN_BOTTOM_MID, 0, -14);

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
    s_name_bold = NULL;
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
