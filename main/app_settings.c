// main/app_settings.c —— 设置页：仅 SoftAP 引导；Wi-Fi 在后台任务启动，避免卡住按键。
#include "app.h"

#include "app_config_ap.h"
#include "app_profile.h"
#include "app_ui.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "app_settings";

static lv_obj_t *s_scr;
static lv_obj_t *s_card;
static lv_obj_t *s_batt;
static lv_obj_t *s_ap_info;
static lv_obj_t *s_status;
static esp_timer_handle_t s_batt_timer;
static TaskHandle_t s_ap_task;
static volatile bool s_page_alive;

static void refresh_ap_info(void)
{
    if (!s_ap_info) {
        return;
    }
    if (app_config_ap_is_running()) {
        lv_label_set_text_fmt(s_ap_info,
                              "1. 连接手机 Wi-Fi\n"
                              "   %s\n"
                              "   密码 %s\n\n"
                              "2. 浏览器打开\n"
                              "   http://192.168.4.1/\n\n"
                              "3. 网页修改头像和资料",
                              app_config_ap_ssid(),
                              app_config_ap_password());
    } else {
        lv_label_set_text(s_ap_info, "热点开启失败\n请返回后重试");
    }
}

static void ap_start_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "后台启动 SoftAP");
    esp_err_t err = app_config_ap_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SoftAP 失败: %s", esp_err_to_name(err));
    }
    if (!s_page_alive) {
        // 用户已离开设置页：立刻关掉刚拉起的热点。
        app_config_ap_stop();
    } else if (bsp_lvgl_lock(1000)) {
        refresh_ap_info();
        bsp_lvgl_unlock();
    }
    s_ap_task = NULL;
    vTaskDelete(NULL);
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
    if (app_config_ap_take_profile_updated() && s_status) {
        lv_label_set_text(s_status, "资料已更新");
    }
    bsp_lvgl_unlock();
}

void app_settings_enter(void)
{
    s_page_alive = true;
    s_scr = app_ui_screen_create();
    s_card = app_ui_card(s_scr);

    lv_obj_t *bar = app_ui_topbar_create(s_card);
    s_batt = (lv_obj_t *)lv_obj_get_user_data(bar);

    lv_obj_t *title = app_ui_label(s_card, "手机联网设置", APP_COL_INK);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title, 200);
    lv_obj_set_pos(title, 20, 40);

    s_ap_info = app_ui_label(s_card, "正在开启热点…", APP_COL_INK);
    lv_obj_set_width(s_ap_info, 200);
    lv_label_set_long_mode(s_ap_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(s_ap_info, 2, 0);
    lv_obj_set_pos(s_ap_info, 20, 72);

    s_status = app_ui_label(s_card, "", APP_COL_RED);
    lv_obj_set_style_text_align(s_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_status, 200);
    lv_obj_set_pos(s_status, 20, 250);

    lv_obj_t *hint = app_ui_label(s_card, "确定键返回通行证", APP_COL_MUTED);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, 200);
    lv_obj_set_pos(hint, 20, 292);

    lv_screen_load(s_scr);

    // 绝不能在按键回调/LVGL 锁内同步起 Wi-Fi，否则按键任务会卡死。
    if (s_ap_task == NULL) {
        xTaskCreate(ap_start_task, "ap_cfg", 8192, NULL, 5, &s_ap_task);
    }

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
    s_page_alive = false;
    if (s_batt_timer) {
        esp_timer_stop(s_batt_timer);
    }
    app_config_ap_stop();
    if (s_scr) {
        lv_obj_delete(s_scr);
    }
    s_scr = NULL;
    s_card = NULL;
    s_batt = NULL;
    s_ap_info = NULL;
    s_status = NULL;
}

void app_settings_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (btn == BSP_BTN_OK && (ev == BSP_BTN_CLICK || ev == BSP_BTN_LONG)) {
        app_goto(APP_PAGE_HOME);
    }
}
