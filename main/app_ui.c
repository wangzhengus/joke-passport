// main/app_ui.c —— 顶栏 / 标签 / 预置头像绘制。
#include "app_ui.h"

#include <stdio.h>

#include "app_profile.h"
#include "bsp_battery.h"

static const uint32_t s_avatar_colors[APP_AVATAR_PRESET_COUNT] = {
    0xFAC94A, 0x7EC8E3, 0xF4A261, 0xE76F51, 0x2A9D8F,
    0xE9C46A, 0x264653, 0x9B5DE5, 0x00BBF9, 0xF15BB5,
};

static const char *s_avatar_glyphs[APP_AVATAR_PRESET_COUNT] = {
    "笑", "冷", "哈", "嘿", "呵", "嘻", "噗", "咯", "哇", "噢",
};

uint32_t app_ui_avatar_color(int idx)
{
    if (idx < 0 || idx >= APP_AVATAR_PRESET_COUNT) {
        return s_avatar_colors[0];
    }
    return s_avatar_colors[idx];
}

const char *app_ui_avatar_glyph(int idx)
{
    if (idx < 0 || idx >= APP_AVATAR_PRESET_COUNT) {
        return s_avatar_glyphs[0];
    }
    return s_avatar_glyphs[idx];
}

void app_ui_style_label(lv_obj_t *label, uint32_t color)
{
    lv_obj_set_style_text_font(label, &lv_font_cn_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
}

lv_obj_t *app_ui_label(lv_obj_t *parent, const char *text, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text ? text : "");
    app_ui_style_label(label, color);
    return label;
}

lv_obj_t *app_ui_screen_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(APP_COL_BG), 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    return scr;
}

lv_obj_t *app_ui_topbar_create(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, 240, 28);
    lv_obj_set_style_bg_color(bar, lv_color_hex(APP_COL_BG), 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(APP_COL_LINE), 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);

    lv_obj_t *title = app_ui_label(bar, "Joke Passport", APP_COL_INK);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t *batt = app_ui_label(bar, "", APP_COL_MUTED);
    lv_obj_align(batt, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_user_data(bar, batt);

    app_ui_topbar_set_battery(batt, bsp_battery_soc());
    return bar;
}

void app_ui_topbar_set_battery(lv_obj_t *batt_label, int soc)
{
    if (!batt_label) {
        return;
    }
    if (soc < 0) {
        lv_label_set_text(batt_label, "");
        return;
    }
    lv_label_set_text_fmt(batt_label, "%d%%", soc);
}

void app_ui_draw_avatar(lv_obj_t *parent, int x, int y, int size, int idx)
{
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_remove_flag(circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(circle, x, y);
    lv_obj_set_size(circle, size, size);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(circle, lv_color_hex(app_ui_avatar_color(idx)), 0);
    lv_obj_set_style_border_color(circle, lv_color_hex(APP_COL_INK), 0);
    lv_obj_set_style_border_width(circle, 1, 0);
    lv_obj_set_style_pad_all(circle, 0, 0);

    lv_obj_t *glyph = app_ui_label(circle, app_ui_avatar_glyph(idx), APP_COL_INK);
    lv_obj_center(glyph);
}
