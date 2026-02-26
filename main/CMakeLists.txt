idf_component_register(
    SRCS 
        "main.c" "ui/ui_timer.c" "bsp/cyd.c" "model/pomo_model.c"
    INCLUDE_DIRS 
        "." "ui" "bsp" "model"

    PRIV_REQUIRES esp_lcd nvs_flash esp_timer fatfs sdmmc
       )
