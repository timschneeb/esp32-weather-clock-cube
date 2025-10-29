//
// Created by tim on 15.10.25.
//

#include "QEMUFramebuffer.h"

#include <esp_lcd_panel_ops.h>
#include <esp_lcd_qemu_rgb.h>

QEMUFramebuffer::QEMUFramebuffer() {
    esp_lcd_rgb_qemu_config_t panel_config = {
        .width = 240,
        .height = 240,
        .bpp = RGB_QEMU_BPP_16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_qemu(&panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
}

QEMUFramebuffer::~QEMUFramebuffer() {
    ESP_ERROR_CHECK(esp_lcd_panel_del(panel_handle));
}

void QEMUFramebuffer::push(uint16_t *buffer, const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h) {
    esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + w + 1, y + h + 1, buffer);
}

void QEMUFramebuffer::panic(const char *msg, const char *footer) {
    (void)msg;
    (void)footer;
    // Not implemented
}

void QEMUFramebuffer::clear() {
    // Not implemented
}

