// main/app_config_ap.h —— 设置页 SoftAP + 本地网页改资料（AP-only，短时开启）。
#pragma once

#include <stdbool.h>

#include "esp_err.h"

// 启动 SoftAP 与 HTTP。已运行则幂等返回 ESP_OK。
// 阻塞性：内部会 init Wi-Fi；请在设置页 enter 时调用，exit 时 stop。
esp_err_t app_config_ap_start(void);

// 停止 HTTP 与 SoftAP，释放无线栈。可重复调用。
void app_config_ap_stop(void);

bool app_config_ap_is_running(void);

// 当前热点名（固定 JokePassport）。
const char *app_config_ap_ssid(void);

// 当前 8 位数字密码；每次 start 重新随机。未启动时也可能已预生成。
const char *app_config_ap_password(void);

// 网页保存资料后置位；调用方读后清除。用于设置页刷新 UI（勿在 HTTP 任务里碰 LVGL）。
bool app_config_ap_take_profile_updated(void);
