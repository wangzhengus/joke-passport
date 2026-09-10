// main/app_joke_page.c —— 冷笑话页：进页/上下键均为随机一条（尽量不重复）。
#include "app.h"

#include "app_jokes.h"
#include "app_ui.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "lvgl.h"

static lv_obj_t *s_scr;
static lv_obj_t *s_card;
static lv_obj_t *s_batt;
static lv_obj_t *s_body;
static size_t s_idx;
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

static size_t pick_random_idx(size_t avoid)
{
    size_t n = app_jokes_count();
    if (n == 0) {
        return 0;
    }
    if (n == 1) {
        return 0;
    }
    size_t next = (size_t)(esp_random() % n);
    // 尽量不与当前相同
    if (next == avoid) {
        next = (next + 1 + (esp_random() % (n - 1))) % n;
    }
    return next;
}

static void refresh_joke(void)
{
    const app_joke_t *j = app_jokes_get(s_idx);
    if (s_body && j) {
        lv_label_set_text(s_body, j->text);
    }
}

void app_joke_enter(void)
{
    s_idx = pick_random_idx((size_t)-1);

    s_scr = app_ui_screen_create();
    s_card = app_ui_card(s_scr);

    lv_obj_t *bar = app_ui_topbar_create(s_card);
    s_batt = (lv_obj_t *)lv_obj_get_user_data(bar);

    s_body = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_pos(s_body, 20, 44);
    lv_obj_set_width(s_body, 184);
    lv_label_set_long_mode(s_body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(s_body, 4, 0);

    lv_obj_t *up = lv_label_create(s_card);
    lv_label_set_text(up, LV_SYMBOL_UP);
    lv_obj_set_style_text_font(up, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(up, lv_color_hex(APP_COL_INK), 0);
    lv_obj_align(up, LV_ALIGN_TOP_RIGHT, -14, 52);

    lv_obj_t *down = lv_label_create(s_card);
    lv_label_set_text(down, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_font(down, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(down, lv_color_hex(APP_COL_INK), 0);
    lv_obj_align(down, LV_ALIGN_BOTTOM_RIGHT, -14, -40);

    lv_obj_t *back = lv_label_create(s_card);
    lv_label_set_text(back, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(back, lv_color_hex(APP_COL_MUTED), 0);
    lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -14, -14);

    refresh_joke();
    lv_screen_load(s_scr);

    if (!s_batt_timer) {
        const esp_timer_create_args_t args = {
            .callback = batt_tick,
            .name = "joke_batt",
        };
        esp_timer_create(&args, &s_batt_timer);
    }
    esp_timer_start_periodic(s_batt_timer, 2000000);
}

void app_joke_exit(void)
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
    s_body = NULL;
}

void app_joke_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (ev != BSP_BTN_CLICK) {
        return;
    }
    size_t n = app_jokes_count();
    if (n == 0) {
        return;
    }
    if (btn == BSP_BTN_OK) {
        app_goto(APP_PAGE_HOME);
        return;
    }
    if (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) {
        s_idx = pick_random_idx(s_idx);
        refresh_joke();
    }
}
