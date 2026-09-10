// main/app_ui.h —— Joke Passport 极简红白 UI 共用件。
#pragma once

#include "lvgl.h"

#define APP_COL_BG     0xFFFFFF
#define APP_COL_INK    0x1A1A1A
#define APP_COL_MUTED  0x6B6B6B
#define APP_COL_LINE   0xE8E0D0
#define APP_COL_RED    0xCC3333
#define APP_COL_AVATAR 0xFAC94A

LV_FONT_DECLARE(lv_font_cn_16);

lv_obj_t *app_ui_screen_create(void);
lv_obj_t *app_ui_topbar_create(lv_obj_t *parent);
void app_ui_topbar_set_battery(lv_obj_t *batt_label, int soc);
lv_obj_t *app_ui_label(lv_obj_t *parent, const char *text, uint32_t color);
void app_ui_style_label(lv_obj_t *label, uint32_t color);

// 预置头像底色（10 色）。
uint32_t app_ui_avatar_color(int idx);
const char *app_ui_avatar_glyph(int idx);
void app_ui_draw_avatar(lv_obj_t *parent, int x, int y, int size, int idx);
