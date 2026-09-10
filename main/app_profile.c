// main/app_profile.c —— 资料默认值、编号生成、NVS 读写。
#include "app_profile.h"

#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "app_profile";
static const char *NVS_NS = "joke_pass";

static app_profile_t s_profile;
static bool s_ready;

static void set_defaults(app_profile_t *p)
{
    memset(p, 0, sizeof(*p));
    strncpy(p->name, "王征", sizeof(p->name) - 1);
    strncpy(p->title, "冷笑话大王", sizeof(p->title) - 1);
    strncpy(p->bio, "最好笑的就是我不会笑", sizeof(p->bio) - 1);
    p->avatar_idx = 0;
}

static uint16_t mac_tail4(void)
{
    uint8_t mac[6] = {0};
    // 用 STA MAC 末两字节作为机器码后 4 位十六进制。
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        return 0x0000;
    }
    return (uint16_t)((mac[4] << 8) | mac[5]);
}

static void fill_time_parts(int *year, int *mon, int *day, int *hour, int *min)
{
    time_t now = time(NULL);
    struct tm tm_now = {0};
    localtime_r(&now, &tm_now);
    *year = tm_now.tm_year + 1900;
    *mon = tm_now.tm_mon + 1;
    *day = tm_now.tm_mday;
    *hour = tm_now.tm_hour;
    *min = tm_now.tm_min;

    // 无 RTC/未对时：用编译日期时间兜底，保证编号仍含“安装时刻”语义。
    if (*year < 2024) {
        // __DATE__ 形如 "Sep 10 2026"，__TIME__ 形如 "11:32:00"
        const char *d = __DATE__;
        const char *t = __TIME__;
        const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char mon3[4] = {d[0], d[1], d[2], 0};
        const char *mp = strstr(months, mon3);
        *mon = mp ? (int)((mp - months) / 3) + 1 : 1;
        *day = (d[4] == ' ') ? (d[5] - '0') : (d[4] - '0') * 10 + (d[5] - '0');
        *year = (d[7] - '0') * 1000 + (d[8] - '0') * 100 + (d[9] - '0') * 10 + (d[10] - '0');
        *hour = (t[0] - '0') * 10 + (t[1] - '0');
        *min = (t[3] - '0') * 10 + (t[4] - '0');
    }
}

static void ensure_passport_id(app_profile_t *p)
{
    if (p->passport_id[0] != '\0') return;

    int year, mon, day, hour, min;
    fill_time_parts(&year, &mon, &day, &hour, &min);
    if (app_passport_id_format(p->passport_id, sizeof(p->passport_id),
                               year, mon, day, hour, min, mac_tail4()) < 0) {
        strncpy(p->passport_id, "J0000000000000000", sizeof(p->passport_id) - 1);
    }
    ESP_LOGI(TAG, "生成通行证编号: %s", p->passport_id);
}

static esp_err_t nvs_load(app_profile_t *p)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_ERR_NOT_FOUND;
    if (err != ESP_OK) return err;

    size_t len = sizeof(p->name);
    err = nvs_get_str(h, "name", p->name, &len);
    if (err != ESP_OK) { nvs_close(h); return err; }

    len = sizeof(p->title);
    err = nvs_get_str(h, "title", p->title, &len);
    if (err != ESP_OK) { nvs_close(h); return err; }

    len = sizeof(p->bio);
    err = nvs_get_str(h, "bio", p->bio, &len);
    if (err != ESP_OK) { nvs_close(h); return err; }

    len = sizeof(p->passport_id);
    err = nvs_get_str(h, "pass_id", p->passport_id, &len);
    if (err != ESP_OK) { nvs_close(h); return err; }

    uint8_t av = 0;
    err = nvs_get_u8(h, "avatar", &av);
    if (err != ESP_OK) { nvs_close(h); return err; }
    p->avatar_idx = av;

    nvs_close(h);
    return ESP_OK;
}

esp_err_t app_profile_save(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_str(h, "name", s_profile.name);
    if (err == ESP_OK) err = nvs_set_str(h, "title", s_profile.title);
    if (err == ESP_OK) err = nvs_set_str(h, "bio", s_profile.bio);
    if (err == ESP_OK) err = nvs_set_str(h, "pass_id", s_profile.passport_id);
    if (err == ESP_OK) err = nvs_set_u8(h, "avatar", s_profile.avatar_idx);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t app_profile_init(void)
{
    if (s_ready) return ESP_OK;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 分区损坏或版本变更：擦除本分区后重试。会触发“重装后重新生成编号”。
        ESP_LOGW(TAG, "NVS 需擦除后重建: %s", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;

    set_defaults(&s_profile);
    err = nvs_load(&s_profile);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "无已存资料，使用默认并生成编号");
        set_defaults(&s_profile);
        ensure_passport_id(&s_profile);
        err = app_profile_save();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "首次保存失败: %s", esp_err_to_name(err));
            return err;
        }
    } else {
        ensure_passport_id(&s_profile);
        if (s_profile.avatar_idx >= APP_AVATAR_PRESET_COUNT) {
            s_profile.avatar_idx = 0;
        }
    }

    s_ready = true;
    return ESP_OK;
}

const app_profile_t *app_profile_get(void)
{
    return &s_profile;
}

esp_err_t app_profile_set_fields(const char *name, const char *title,
                                 const char *bio, int avatar_idx)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    if (avatar_idx < 0 || avatar_idx >= APP_AVATAR_PRESET_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    if (name) {
        strncpy(s_profile.name, name, sizeof(s_profile.name) - 1);
        s_profile.name[sizeof(s_profile.name) - 1] = '\0';
    }
    if (title) {
        strncpy(s_profile.title, title, sizeof(s_profile.title) - 1);
        s_profile.title[sizeof(s_profile.title) - 1] = '\0';
    }
    if (bio) {
        strncpy(s_profile.bio, bio, sizeof(s_profile.bio) - 1);
        s_profile.bio[sizeof(s_profile.bio) - 1] = '\0';
    }
    s_profile.avatar_idx = (uint8_t)avatar_idx;
    return app_profile_save();
}

esp_err_t app_profile_set_avatar(int avatar_idx)
{
    return app_profile_set_fields(NULL, NULL, NULL, avatar_idx);
}
