idf_component_register(
    SRCS "main.c"
         "app_passport_id.c"
         "app_profile.c"
         "app_jokes.c"
         "app_ui.c"
         "app_home.c"
         "app_joke_page.c"
         "app_settings.c"
         "app_config_ap.c"
         "lv_font_cn_16.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_event esp_http_server esp_hw_support esp_netif esp_timer
             esp_wifi nvs_flash
    PRIV_REQUIRES lvgl
)
