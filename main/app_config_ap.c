// main/app_config_ap.c —— SoftAP 本地配置页。
//
// 模式：AP-only，短时配网（仅设置页生命周期内开启）。
// 手机连上热点后浏览器打开 http://192.168.4.1/ 提交姓名/头衔/简介/头像序号。
// 自定义图片上传留到后续版本（无 PSRAM，需分块与解码预算）。
#include "app_config_ap.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_profile.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "config_ap";

#define AP_CHANNEL 1
#define AP_MAX_CONN 1
#define BODY_MAX 512

static char s_ssid[32];
static bool s_netif_ready;
static bool s_wifi_inited;
static bool s_wifi_started;
static bool s_running;
static esp_netif_t *s_ap_netif;
static httpd_handle_t s_httpd;
static volatile bool s_profile_updated;

static const char PAGE_HTML[] =
    "<!DOCTYPE html><html><head><meta charset=utf-8>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>Joke Passport</title>"
    "<style>"
    "body{font-family:sans-serif;margin:16px;background:#f7f4ef;color:#1a1a1a}"
    "h1{font-size:18px;margin:0 0 12px}"
    "label{display:block;font-size:12px;color:#666;margin-top:10px}"
    "input,select{width:100%;box-sizing:border-box;padding:8px;margin-top:4px;"
    "border:1px solid #ccc;border-radius:4px;font-size:16px}"
    "button{margin-top:16px;width:100%;padding:12px;background:#cc3333;color:#fff;"
    "border:0;border-radius:6px;font-size:16px}"
    ".hint{font-size:12px;color:#666;margin-top:12px;line-height:1.4}"
    "</style></head><body>"
    "<h1>笑场通行证 · 设置</h1>"
    "<form method=POST action=/save>"
    "<label>姓名<input name=name maxlength=20 required></label>"
    "<label>头衔<input name=title maxlength=20 required></label>"
    "<label>简介<input name=bio maxlength=40 required></label>"
    "<label>预置头像"
    "<select name=avatar>"
    "<option value=0>0 黄</option><option value=1>1 橙</option>"
    "<option value=2>2 红</option><option value=3>3 粉</option>"
    "<option value=4>4 紫</option><option value=5>5 蓝</option>"
    "<option value=6>6 青</option><option value=7>7 绿</option>"
    "<option value=8>8 棕</option><option value=9>9 灰</option>"
    "</select></label>"
    "<button type=submit>保存到设备</button>"
    "</form>"
    "<p class=hint>自定义头像上传即将支持。保存成功后可断开热点，"
    "在设备上短按确定保存头像选择并返回。</p>"
    "</body></html>";

static const char OK_HTML[] =
    "<!DOCTYPE html><html><head><meta charset=utf-8>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>已保存</title></head><body style=\"font-family:sans-serif;margin:24px\">"
    "<h1>已保存</h1><p>资料已写入设备。可关闭本页并断开 Wi-Fi。</p>"
    "<p><a href=/>继续修改</a></p></body></html>";

static void build_ssid(void)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(s_ssid, sizeof(s_ssid), "JokePass-%02X%02X", mac[4], mac[5]);
}

static int hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// application/x-www-form-urlencoded 就地解码到 dst，最多 dst_sz-1。
static void url_decode(char *dst, size_t dst_sz, const char *src, size_t src_len)
{
    size_t di = 0;
    for (size_t i = 0; i < src_len && di + 1 < dst_sz; i++) {
        char c = src[i];
        if (c == '+') {
            dst[di++] = ' ';
        } else if (c == '%' && i + 2 < src_len) {
            int hi = hex_val(src[i + 1]);
            int lo = hex_val(src[i + 2]);
            if (hi >= 0 && lo >= 0) {
                dst[di++] = (char)((hi << 4) | lo);
                i += 2;
            }
        } else {
            dst[di++] = c;
        }
    }
    dst[di] = '\0';
}

// 从 form body 取字段（未解码的原始切片），找不到返回 false。
static bool form_get_raw(const char *body, size_t body_len, const char *key,
                         const char **val, size_t *val_len)
{
    size_t key_len = strlen(key);
    size_t i = 0;
    while (i < body_len) {
        size_t start = i;
        while (i < body_len && body[i] != '=' && body[i] != '&') i++;
        size_t klen = i - start;
        if (i < body_len && body[i] == '=' && klen == key_len &&
            memcmp(body + start, key, key_len) == 0) {
            i++;
            size_t vstart = i;
            while (i < body_len && body[i] != '&') i++;
            *val = body + vstart;
            *val_len = i - vstart;
            return true;
        }
        while (i < body_len && body[i] != '&') i++;
        if (i < body_len && body[i] == '&') i++;
    }
    return false;
}

// 写入 HTML 属性安全文本（去掉 " < > &），防止表单 value 被截断或注入。
static void html_attr(char *dst, size_t dst_sz, const char *src)
{
    size_t di = 0;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    for (size_t i = 0; src[i] && di + 1 < dst_sz; i++) {
        char c = src[i];
        if (c == '"' || c == '<' || c == '>' || c == '&') {
            continue;
        }
        dst[di++] = c;
    }
    dst[di] = '\0';
}

static esp_err_t root_get(httpd_req_t *req)
{
    const app_profile_t *p = app_profile_get();
    char name[APP_PROFILE_NAME_MAX];
    char title[APP_PROFILE_TITLE_MAX];
    char bio[APP_PROFILE_BIO_MAX];
    html_attr(name, sizeof(name), p ? p->name : "");
    html_attr(title, sizeof(title), p ? p->title : "");
    html_attr(bio, sizeof(bio), p ? p->bio : "");

    char page[1800];
    int av = p ? p->avatar_idx : 0;
    int n = snprintf(
        page, sizeof(page),
        "<!DOCTYPE html><html><head><meta charset=utf-8>"
        "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
        "<title>Joke Passport</title>"
        "<style>"
        "body{font-family:sans-serif;margin:16px;background:#f7f4ef;color:#1a1a1a}"
        "h1{font-size:18px}label{display:block;font-size:12px;color:#666;margin-top:10px}"
        "input,select{width:100%%;box-sizing:border-box;padding:8px;margin-top:4px;"
        "border:1px solid #ccc;border-radius:4px;font-size:16px}"
        "button{margin-top:16px;width:100%%;padding:12px;background:#cc3333;color:#fff;"
        "border:0;border-radius:6px;font-size:16px}"
        ".hint{font-size:12px;color:#666;margin-top:12px;line-height:1.4}"
        "</style></head><body>"
        "<h1>笑场通行证 · 设置</h1>"
        "<form method=POST action=/save>"
        "<label>姓名<input name=name maxlength=20 required value=\"%s\"></label>"
        "<label>头衔<input name=title maxlength=20 required value=\"%s\"></label>"
        "<label>简介<input name=bio maxlength=40 required value=\"%s\"></label>"
        "<label>预置头像<select name=avatar>"
        "<option value=0%s>0</option><option value=1%s>1</option>"
        "<option value=2%s>2</option><option value=3%s>3</option>"
        "<option value=4%s>4</option><option value=5%s>5</option>"
        "<option value=6%s>6</option><option value=7%s>7</option>"
        "<option value=8%s>8</option><option value=9%s>9</option>"
        "</select></label>"
        "<button type=submit>保存到设备</button></form>"
        "<p class=hint>连热点后打开本页。自定义图片上传稍后支持。</p>"
        "</body></html>",
        name, title, bio,
        av == 0 ? " selected" : "", av == 1 ? " selected" : "",
        av == 2 ? " selected" : "", av == 3 ? " selected" : "",
        av == 4 ? " selected" : "", av == 5 ? " selected" : "",
        av == 6 ? " selected" : "", av == 7 ? " selected" : "",
        av == 8 ? " selected" : "", av == 9 ? " selected" : "");
    if (n < 0 || n >= (int)sizeof(page)) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        return httpd_resp_send(req, PAGE_HTML, HTTPD_RESP_USE_STRLEN);
    }
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, page, n);
}

static esp_err_t save_post(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > BODY_MAX) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body too large");
        return ESP_FAIL;
    }

    char body[BODY_MAX + 1];
    int received = 0;
    while (received < req->content_len) {
        int r = httpd_req_recv(req, body + received, req->content_len - received);
        if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (r <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv fail");
            return ESP_FAIL;
        }
        received += r;
    }
    body[received] = '\0';

    const char *raw;
    size_t raw_len;
    char name[APP_PROFILE_NAME_MAX];
    char title[APP_PROFILE_TITLE_MAX];
    char bio[APP_PROFILE_BIO_MAX];
    char avatar_s[8];
    name[0] = title[0] = bio[0] = avatar_s[0] = '\0';

    if (form_get_raw(body, (size_t)received, "name", &raw, &raw_len)) {
        url_decode(name, sizeof(name), raw, raw_len);
    }
    if (form_get_raw(body, (size_t)received, "title", &raw, &raw_len)) {
        url_decode(title, sizeof(title), raw, raw_len);
    }
    if (form_get_raw(body, (size_t)received, "bio", &raw, &raw_len)) {
        url_decode(bio, sizeof(bio), raw, raw_len);
    }
    if (form_get_raw(body, (size_t)received, "avatar", &raw, &raw_len)) {
        url_decode(avatar_s, sizeof(avatar_s), raw, raw_len);
    }

    if (name[0] == '\0' || title[0] == '\0' || bio[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing fields");
        return ESP_FAIL;
    }

    int avatar = atoi(avatar_s);
    if (avatar < 0 || avatar >= APP_AVATAR_PRESET_COUNT) {
        avatar = 0;
    }

    esp_err_t err = app_profile_set_fields(name, title, bio, avatar);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "save fail");
        return ESP_FAIL;
    }

    s_profile_updated = true;
    ESP_LOGI(TAG, "网页已更新资料 name=%s avatar=%d", name, avatar);

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, OK_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t start_httpd(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.max_open_sockets = 3;
    cfg.lru_purge_enable = true;
    cfg.stack_size = 6144;

    esp_err_t err = httpd_start(&s_httpd, &cfg);
    if (err != ESP_OK) return err;

    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get,
    };
    const httpd_uri_t save = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = save_post,
    };
    httpd_register_uri_handler(s_httpd, &root);
    httpd_register_uri_handler(s_httpd, &save);
    return ESP_OK;
}

esp_err_t app_config_ap_start(void)
{
    esp_err_t err;

    if (s_running) return ESP_OK;

    build_ssid();

    // NVS 已由 app_profile_init 准备好；此处再调一次保持幂等。
    (void)nvs_flash_init();

    if (!s_netif_ready) {
        err = esp_netif_init();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
        err = esp_event_loop_create_default();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
        s_netif_ready = true;
    }

    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
        if (!s_ap_netif) return ESP_ERR_NO_MEM;
    }

    if (!s_wifi_inited) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        err = esp_wifi_init(&cfg);
        if (err != ESP_OK) return err;
        s_wifi_inited = true;
    }

    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) return err;
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) return err;

    wifi_config_t ap = {0};
    strncpy((char *)ap.ap.ssid, s_ssid, sizeof(ap.ap.ssid));
    ap.ap.ssid_len = strlen(s_ssid);
    ap.ap.channel = AP_CHANNEL;
    ap.ap.max_connection = AP_MAX_CONN;
    ap.ap.authmode = WIFI_AUTH_OPEN;

    err = esp_wifi_set_config(WIFI_IF_AP, &ap);
    if (err != ESP_OK) return err;
    err = esp_wifi_start();
    if (err != ESP_OK) return err;
    s_wifi_started = true;

    err = start_httpd();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd 启动失败: %s", esp_err_to_name(err));
        app_config_ap_stop();
        return err;
    }

    s_running = true;
    ESP_LOGI(TAG, "SoftAP 已开 SSID=%s URL=http://192.168.4.1/", s_ssid);
    return ESP_OK;
}

void app_config_ap_stop(void)
{
    if (s_httpd) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }
    if (s_wifi_started) {
        esp_wifi_stop();
        s_wifi_started = false;
    }
    if (s_wifi_inited) {
        esp_wifi_deinit();
        s_wifi_inited = false;
    }
    if (s_ap_netif) {
        esp_netif_destroy_default_wifi(s_ap_netif);
        s_ap_netif = NULL;
    }
    s_running = false;
    ESP_LOGI(TAG, "SoftAP 已关闭");
}

bool app_config_ap_is_running(void)
{
    return s_running;
}

const char *app_config_ap_ssid(void)
{
    if (s_ssid[0] == '\0') {
        build_ssid();
    }
    return s_ssid;
}

bool app_config_ap_take_profile_updated(void)
{
    bool v = s_profile_updated;
    s_profile_updated = false;
    return v;
}
