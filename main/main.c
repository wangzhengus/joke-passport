// main/main.c —— Joke Passport（笑场通行证）应用入口。
//
// 按键语义:
//   首页: 任意键短按 → 笑话页; 确定长按 → 设置
//   笑话页: 上/下换笑话; 确定短按返回首页
//   设置页: 上/下选头像; 确定短按保存返回; 确定长按不保存返回
#include "app.h"
#include "app_profile.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char *TAG = "joke_passport";
static app_page_t s_page = APP_PAGE_HOME;

static void page_exit(app_page_t page)
{
    switch (page) {
    case APP_PAGE_HOME:
        app_home_exit();
        break;
    case APP_PAGE_JOKE:
        app_joke_exit();
        break;
    case APP_PAGE_SETTINGS:
        app_settings_exit();
        break;
    }
}

static void page_enter(app_page_t page)
{
    switch (page) {
    case APP_PAGE_HOME:
        app_home_enter();
        break;
    case APP_PAGE_JOKE:
        app_joke_enter();
        break;
    case APP_PAGE_SETTINGS:
        app_settings_enter();
        break;
    }
}

void app_goto(app_page_t page)
{
    if (page == s_page) {
        if (page == APP_PAGE_HOME) {
            app_home_refresh();
        }
        return;
    }
    page_exit(s_page);
    s_page = page;
    page_enter(s_page);
}

static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    if (!bsp_lvgl_lock(500)) {
        return;
    }
    switch (s_page) {
    case APP_PAGE_HOME:
        app_home_key(btn, ev);
        break;
    case APP_PAGE_JOKE:
        app_joke_key(btn, ev);
        break;
    case APP_PAGE_SETTINGS:
        app_settings_key(btn, ev);
        break;
    }
    bsp_lvgl_unlock();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Joke Passport / 笑场通行证 启动");
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup != ESP_SLEEP_WAKEUP_UNDEFINED) {
        ESP_LOGI(TAG, "休眠唤醒原因: %d", wakeup);
    }

    esp_err_t perr = app_profile_init();
    if (perr != ESP_OK) {
        ESP_LOGE(TAG, "资料初始化失败: %s", esp_err_to_name(perr));
    }

    bsp_i2c_init();
    bsp_i2c_scan();

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败。"
                      "检查 SPI(MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    (void)bsp_battery_init();
    if (bsp_button_init(on_key, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "按键初始化失败");
        return;
    }

    if (bsp_lvgl_lock(1000)) {
        s_page = APP_PAGE_HOME;
        app_home_enter();
        bsp_lvgl_unlock();
    }

    const app_profile_t *p = app_profile_get();
    ESP_LOGI(TAG, "就绪 id=%s name=%s",
             p ? p->passport_id : "?", p ? p->name : "?");
}
