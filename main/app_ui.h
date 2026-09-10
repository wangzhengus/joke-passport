// main/app_ui.h —— Joke Passport 极简 UI（黑底 + 白卡片圆角，适配外壳）。
#pragma once

#include "lvgl.h"

#define APP_COL_OUTSIDE 0x000000
#define APP_COL_BG      0xFFFFFF
#define APP_COL_INK     0x1A1A1A
#define APP_COL_MUTED   0x8A8A8A
#define APP_COL_LINE    0xE8E0D0
#define APP_COL_RED     0xCC3333
#define APP_COL_AVATAR  0xFAC94A
#define APP_RADIUS      20
#define APP_RED_W       28
#define APP_SCREEN_W    240
#define APP_SCREEN_H    320

LV_FONT_DECLARE(lv_font_cn_16);

// 黑底全屏；返回白色圆角内容卡（四角 APP_RADIUS，带裁剪）。
lv_obj_t *app_ui_screen_create(void);
lv_obj_t *app_ui_card(lv_obj_t *scr);

// 在内容卡上建顶栏：标题居中，电量靠右。user_data = 电量 label。
lv_obj_t *app_ui_topbar_create(lv_obj_t *card);
void app_ui_topbar_set_battery(lv_obj_t *batt_label, int soc);

lv_obj_t *app_ui_label(lv_obj_t *parent, const char *text, uint32_t color);
void app_ui_style_label(lv_obj_t *label, uint32_t color);

// 预置头像底色 / 字。
uint32_t app_ui_avatar_color(int idx);
const char *app_ui_avatar_glyph(int idx);
void app_ui_draw_avatar(lv_obj_t *parent, int x, int y, int size, int idx);
