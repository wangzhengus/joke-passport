// main/app_profile.h —— 笑场通行证个人资料与编号（NVS 持久化）。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_passport_id.h"
#include "esp_err.h"

#define APP_PROFILE_NAME_MAX  24
#define APP_PROFILE_TITLE_MAX 24
#define APP_PROFILE_BIO_MAX   48
#define APP_AVATAR_PRESET_COUNT 10

typedef struct {
    char name[APP_PROFILE_NAME_MAX];
    char title[APP_PROFILE_TITLE_MAX];
    char bio[APP_PROFILE_BIO_MAX];
    char passport_id[APP_PASSPORT_ID_MAX];  // J + YYYYMMDDHHmm + MAC 末 4 位
    uint8_t avatar_idx;                     // 0..APP_AVATAR_PRESET_COUNT-1
} app_profile_t;

esp_err_t app_profile_init(void);
const app_profile_t *app_profile_get(void);

// 修改内存中的资料；成功后自动落盘。avatar_idx 越界返回 ESP_ERR_INVALID_ARG。
esp_err_t app_profile_set_fields(const char *name, const char *title,
                                 const char *bio, int avatar_idx);
esp_err_t app_profile_set_avatar(int avatar_idx);
esp_err_t app_profile_save(void);
