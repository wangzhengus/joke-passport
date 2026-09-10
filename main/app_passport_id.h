// main/app_passport_id.h —— 通行证编号格式化（无硬件依赖，可供 host test）。
#pragma once

#include <stddef.h>
#include <stdint.h>

#define APP_PASSPORT_ID_MAX 20

// 格式: J + YYYYMMDDHHmm + MAC 末 4 位十六进制大写。
// out 至少 APP_PASSPORT_ID_MAX。返回写入长度（不含 NUL），失败 -1。
int app_passport_id_format(char *out, size_t out_sz,
                           int year, int mon, int day, int hour, int min,
                           uint16_t mac_tail);
