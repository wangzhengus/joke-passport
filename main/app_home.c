// main/app_home.c —— 护照首页（对齐 user-design-01：左红条 + 头像资料）。
#include "app.h"

#include "app_profile.h"
#include "app_ui.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_timer.h"
#include "lvgl.h"

static lv_obj_t *s_scr;
static lv_obj_t *s_batt;
static lv_obj_t *s_name;
static lv_obj_t *s_title;
static lv_obj_t *s_bio;
static lv_obj_t *s_side_text;
static lv_obj_t *s_avatar_host;
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

void app_home_refresh(void)
{
    if (!s_scr) {
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
    if (s_side_text) {
        // 红条竖排：产品名 + 编号（整段旋转 -90°）
        lv_label_set_text_fmt(s_side_text, "笑场通行证  %s", p->passport_id);
    }
    rebuild_avatar();
}

void app_home_enter(void)
{
    s_scr = app_ui_screen_create();
    lv_obj_t *bar = app_ui_topbar_create(s_scr);
    s_batt = (lv_obj_t *)lv_obj_get_user_data(bar);

    lv_obj_t *red = lv_obj_create(s_scr);
    lv_obj_remove_flag(red, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(red, 0, 28);
    lv_obj_set_size(red, 28, 292);
    lv_obj_set_style_bg_color(red, lv_color_hex(APP_COL_RED), 0);
    lv_obj_set_style_border_width(red, 0, 0);
    lv_obj_set_style_radius(red, 0, 0);
    lv_obj_set_style_pad_all(red, 0, 0);
    lv_obj_set_style_clip_corner(red, true, 0);

    s_side_text = app_ui_label(red, "", 0xFFFFFF);
    lv_obj_set_style_text_align(s_side_text, LV_TEXT_ALIGN_LEFT, 0);
    // 旋转 -90°，沿红条自下而上阅读（与设计稿一致）
    lv_obj_set_style_transform_pivot_x(s_side_text, 0, 0);
    lv_obj_set_style_transform_pivot_y(s_side_text, 0, 0);
    lv_obj_set_style_transform_angle(s_side_text, -900, 0);
    lv_obj_set_pos(s_side_text, 6, 280);

    s_avatar_host = lv_obj_create(s_scr);
    lv_obj_remove_flag(s_avatar_host, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_avatar_host, 84, 48);
    lv_obj_set_size(s_avatar_host, 72, 72);
    lv_obj_set_style_bg_opa(s_avatar_host, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_avatar_host, 0, 0);
    lv_obj_set_style_pad_all(s_avatar_host, 0, 0);

    s_name = app_ui_label(s_scr, "", APP_COL_INK);
    lv_obj_set_style_text_align(s_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_name, 200);
    lv_obj_align(s_name, LV_ALIGN_TOP_MID, 14, 130);

    s_title = app_ui_label(s_scr, "", APP_COL_INK);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_title, 200);
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 14, 154);

    lv_obj_t *rule1 = lv_obj_create(s_scr);
    lv_obj_remove_flag(rule1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(rule1, 48, 184);
    lv_obj_set_size(rule1, 170, 1);
    lv_obj_set_style_bg_color(rule1, lv_color_hex(APP_COL_LINE), 0);
    lv_obj_set_style_border_width(rule1, 0, 0);
    lv_obj_set_style_radius(rule1, 0, 0);

    s_bio = app_ui_label(s_scr, "", APP_COL_MUTED);
    lv_obj_set_style_text_align(s_bio, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_bio, 180);
    lv_label_set_long_mode(s_bio, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_bio, LV_ALIGN_TOP_MID, 14, 196);

    lv_obj_t *rule2 = lv_obj_create(s_scr);
    lv_obj_remove_flag(rule2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(rule2, 48, 236);
    lv_obj_set_size(rule2, 170, 1);
    lv_obj_set_style_bg_color(rule2, lv_color_hex(APP_COL_LINE), 0);
    lv_obj_set_style_border_width(rule2, 0, 0);
    lv_obj_set_style_radius(rule2, 0, 0);

    lv_obj_t *hint = app_ui_label(s_scr, "任意键开始笑，长按确定进入设置", APP_COL_MUTED);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, 190);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 14, -16);

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
    s_batt = NULL;
    s_name = NULL;
    s_title = NULL;
    s_bio = NULL;
    s_side_text = NULL;
    s_avatar_host = NULL;
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
