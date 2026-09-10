// main/app.h —— Joke Passport 页面入口（替代 demo 菜单）。
#pragma once

#include "bsp_button.h"

typedef enum {
    APP_PAGE_HOME = 0,
    APP_PAGE_JOKE,
    APP_PAGE_SETTINGS,
} app_page_t;

void app_home_enter(void);
void app_home_exit(void);
void app_home_key(bsp_btn_t btn, bsp_btn_ev_t ev);
void app_home_refresh(void);

void app_joke_enter(void);
void app_joke_exit(void);
void app_joke_key(bsp_btn_t btn, bsp_btn_ev_t ev);

void app_settings_enter(void);
void app_settings_exit(void);
void app_settings_key(bsp_btn_t btn, bsp_btn_ev_t ev);

// 由 main 提供的页面切换（已持有 LVGL 锁时调用）。
void app_goto(app_page_t page);
