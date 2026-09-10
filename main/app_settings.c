// main/app_settings.c —— 设置页：精简四行（热点/密码红字/浏览器/网址）。
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
static lv_obj_t *s_line_ssid;
static lv_obj_t *s_line_pwd_label;
static lv_obj_t *s_line_pwd;
static lv_obj_t *s_line_browser;
static lv_obj_t *s_line_url;
static lv_obj_t *s_status;
static esp_timer_handle_t s_batt_timer;
static TaskHandle_t s_ap_task;
static volatile bool s_page_alive;

static void refresh_ap_info(void)
{
    if (!s_line_ssid) {
        return;
    }
    if (app_config_ap_is_running()) {
        lv_label_set_text_fmt(s_line_ssid, "热点  %s", app_config_ap_ssid());
        if (s_line_pwd_label) {
            lv_label_set_text(s_line_pwd_label, "密码");
        }
        if (s_line_pwd) {
            lv_label_set_text(s_line_pwd, app_config_ap_password());
        }
        if (s_line_browser) {
            lv_label_set_text(s_line_browser, "浏览器打开");
        }
        if (s_line_url) {
            lv_label_set_text(s_line_url, "http://192.168.4.1/");
        }
    } else {
        lv_label_set_text(s_line_ssid, "热点开启失败");
        if (s_line_pwd_label) {
            lv_label_set_text(s_line_pwd_label, "请返回后重试");
        }
        if (s_line_pwd) {
            lv_label_set_text(s_line_pwd, "");
        }
        if (s_line_browser) {
            lv_label_set_text(s_line_browser, "");
        }
        if (s_line_url) {
            lv_label_set_text(s_line_url, "");
        }
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

    s_line_ssid = app_ui_label(s_card, "正在开启热点…", APP_COL_INK);
    lv_obj_set_width(s_line_ssid, 200);
    lv_obj_set_pos(s_line_ssid, 24, 56);

    s_line_pwd_label = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_pos(s_line_pwd_label, 24, 88);

    s_line_pwd = app_ui_label(s_card, "", APP_COL_RED);
    lv_obj_set_style_text_font(s_line_pwd, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(s_line_pwd, 72, 84);

    s_line_browser = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_pos(s_line_browser, 24, 128);

    s_line_url = app_ui_label(s_card, "", APP_COL_INK);
    lv_obj_set_style_text_font(s_line_url, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_line_url, 24, 156);

    s_status = app_ui_label(s_card, "", APP_COL_RED);
    lv_obj_set_style_text_align(s_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_status, 200);
    lv_obj_set_pos(s_status, 20, 220);

    lv_obj_t *hint = app_ui_label(s_card, "确定键返回通行证", APP_COL_MUTED);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, 200);
    lv_obj_set_pos(hint, 20, 292);

    lv_screen_load(s_scr);

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
    s_line_ssid = NULL;
    s_line_pwd_label = NULL;
    s_line_pwd = NULL;
    s_line_browser = NULL;
    s_line_url = NULL;
    s_status = NULL;
}

void app_settings_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (btn == BSP_BTN_OK && (ev == BSP_BTN_CLICK || ev == BSP_BTN_LONG)) {
        app_goto(APP_PAGE_HOME);
    }
}
