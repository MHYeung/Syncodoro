#include "cyd.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_check.h"
#include "esp_log.h"

#include "esp_lvgl_port.h"
#include "esp_lcd_panel_vendor.h"   // includes esp_lcd_new_panel_st7789()
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_io_spi.h"

// Includes for SD Card
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

// Touch controller (SPI)
#include "esp_lcd_touch_xpt2046.h"

static const char *TAG = "cyd";

/**
 * Board pinout (ESP32-2432S028R / similar "CYD" boards)
 * - LCD:  2.8" 240x320 ST7789 (SPI)
 * - Touch: XPT2046 (SPI)
 *
 * NOTE:
 * This file targets the common CYD-style wiring (LCD and touch on different SPI hosts).
 * If your board shares the same SPI bus for LCD+touch, you can move touch pins/host to the LCD host.
 */

// --------------------------- LCD (ST7789) ---------------------------
#define CYD_LCD_HOST             SPI2_HOST

#define PIN_NUM_TFT_MISO         12
#define PIN_NUM_TFT_MOSI         13
#define PIN_NUM_TFT_SCLK         14
#define PIN_NUM_TFT_CS           15
#define PIN_NUM_TFT_DC           2
#define PIN_NUM_TFT_RST          -1
#define PIN_NUM_TFT_BL           21

// --------------------------- SD CARD ---------------------------
#define PIN_NUM_SD_CS            4 // TF card CS pin

// The *native* ST7789 panel resolution is 240x320.
// Many CYD projects use it in landscape (320x240) by applying rotation/mirroring at the panel level.
#define LV_HRES                  240
#define LV_VRES                  320

#define LCD_PIXEL_CLOCK_HZ       (20 * 1000 * 1000)

// If colors look swapped (red/blue), change RGB->BGR
#ifndef CYD_LCD_RGB_ELEMENT_ORDER
#define CYD_LCD_RGB_ELEMENT_ORDER LCD_RGB_ELEMENT_ORDER_BGR
#endif

// Some panels need color inversion enabled
#ifndef CYD_LCD_INVERT_COLOR
#define CYD_LCD_INVERT_COLOR     0
#endif

// --------------------------- TOUCH (XPT2046) ---------------------------
#define CYD_TOUCH_HOST           SPI3_HOST

#define PIN_NUM_TOUCH_MISO       39
#define PIN_NUM_TOUCH_MOSI       32
#define PIN_NUM_TOUCH_SCLK       25
#define PIN_NUM_TOUCH_CS         33
#define PIN_NUM_TOUCH_IRQ        -1

// --------------------------- LVGL draw buffers ---------------------------
#define LVGL_DRAW_BUF_LINES      40

static esp_err_t init_lcd(cyd_handles_t *out)
{
    // Backlight OFF
    gpio_config_t bk_cfg = {
        .pin_bit_mask = 1ULL << PIN_NUM_TFT_BL,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&bk_cfg), TAG, "BK gpio_config failed");
    gpio_set_level(PIN_NUM_TFT_BL, 0);

    // SPI bus for LCD
    const spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_TFT_SCLK,
        .mosi_io_num = PIN_NUM_TFT_MOSI,
        .miso_io_num = PIN_NUM_TFT_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LV_HRES * LVGL_DRAW_BUF_LINES * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(CYD_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "spi_bus_initialize(LCD) failed");

    // Panel IO (SPI)
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_NUM_TFT_DC,
        .cs_gpio_num = PIN_NUM_TFT_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(CYD_LCD_HOST, &io_cfg, &out->lcd_io), TAG, "esp_lcd_new_panel_io_spi failed");

    // Panel driver (ST7789)
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_NUM_TFT_RST,
        .rgb_ele_order = CYD_LCD_RGB_ELEMENT_ORDER,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(out->lcd_io, &panel_cfg, &out->lcd_panel), TAG, "esp_lcd_new_panel_st7789 failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(out->lcd_panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(out->lcd_panel), TAG, "panel init failed");

#if CYD_LCD_INVERT_COLOR
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(out->lcd_panel, true), TAG, "panel invert failed");
#endif

    // Many CYD panels need a vertical mirror to match the touchscreen/UI coordinate system
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(out->lcd_panel, false, true), TAG, "panel mirror failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(out->lcd_panel, true), TAG, "panel disp_on failed");

    // Backlight ON
    gpio_set_level(PIN_NUM_TFT_BL, 1);
    return ESP_OK;
}

static esp_err_t init_touch(cyd_handles_t *out)
{

    // SPI bus for touch
    const spi_bus_config_t tp_buscfg = {
        .sclk_io_num = PIN_NUM_TOUCH_SCLK,
        .mosi_io_num = PIN_NUM_TOUCH_MOSI,
        .miso_io_num = PIN_NUM_TOUCH_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(CYD_TOUCH_HOST, &tp_buscfg, SPI_DMA_CH_AUTO), TAG, "spi_bus_initialize(TOUCH) failed");

    // Touch IO over SPI (XPT2046)
    const esp_lcd_panel_io_spi_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(PIN_NUM_TOUCH_CS);
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(CYD_TOUCH_HOST, &tp_io_cfg, &out->touch_io), TAG, "touch esp_lcd_new_panel_io_spi failed");

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = LV_HRES,
        .y_max = LV_VRES,
        .rst_gpio_num = -1,
        .int_gpio_num = PIN_NUM_TOUCH_IRQ,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 0, // match LCD mirror_y above
        },
    };

    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_spi_xpt2046(out->touch_io, &tp_cfg, &out->touch), TAG, "esp_lcd_touch_new_spi_xpt2046 failed");
    return ESP_OK;
}

esp_err_t cyd_init_sd(void)
{
    ESP_LOGI(TAG, "Initializing SD card on shared LCD SPI bus (SPI2)...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;

    // We reuse the existing CYD_LCD_HOST (SPI2)
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = CYD_LCD_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_SD_CS;
    slot_config.host_id = host.slot;

    // Because the LCD already initialized the SPI bus, we skip spi_bus_initialize
    // and just mount the SD device directly to the active bus!
    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD Card mounted successfully!");
    sdmmc_card_print_info(stdout, card);
    
    return ESP_OK;
}

esp_err_t cyd_init(cyd_handles_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = (cyd_handles_t){0};

    ESP_RETURN_ON_ERROR(init_lcd(out), TAG, "init_lcd failed");
    ESP_RETURN_ON_ERROR(init_touch(out), TAG, "init_touch failed");

    // LVGL port init
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl_port_init failed");

    // Add display to LVGL port
    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = out->lcd_io,
        .panel_handle = out->lcd_panel,
        .buffer_size = LV_HRES * LVGL_DRAW_BUF_LINES, 
        .double_buffer = true,
        .hres = LV_HRES,
        .vres = LV_VRES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .mirror_x = 0,
            .mirror_y = 0,
            .swap_xy = 0
        },
    };
    // NOTE: "flags" layout varies across esp_lvgl_port versions; set them after init to avoid brace warnings.
    disp_cfg.flags.buff_dma = true;   // DMA-capable buffers
    disp_cfg.flags.swap_bytes = true; // ST7789 expects big-endian RGB565

    out->disp = lvgl_port_add_disp(&disp_cfg);
    ESP_RETURN_ON_FALSE(out->disp, ESP_FAIL, TAG, "lvgl_port_add_disp failed");

    // Add touch input device
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = out->disp,
        .handle = out->touch,
    };
    out->indev = lvgl_port_add_touch(&touch_cfg);
    ESP_RETURN_ON_FALSE(out->indev, ESP_FAIL, TAG, "lvgl_port_add_touch failed");

    return ESP_OK;
}
