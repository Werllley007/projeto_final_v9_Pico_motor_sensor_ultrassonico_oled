#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "oled.h"

// Pinos I2C para o OLED
#define I2C_SDA 14
#define I2C_SCL 15

// Buffer global para o OLED
static uint8_t ssd[ssd1306_buffer_length];
static struct render_area frame_area;

// Função para inicializar o OLED
void oled_init() {
    // Inicialização do I2C
    i2c_init(i2c1, 100 * 1000); // Configura I2C em 100kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do controlador SSD1306
    ssd1306_init();

    // Configura área de renderização para todo o display
    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&frame_area);

    // Limpa o display inicial
    oled_clear();
}

// Função para limpar o display
void oled_clear() {
    memset(ssd, 0, ssd1306_buffer_length); // Limpa o buffer local
    render_on_display(ssd, &frame_area);   // Atualiza o display
}

// Função para exibir texto
void oled_display_text(const char *text, uint8_t x, uint8_t y) {
    ssd1306_draw_string(ssd, x, y, (char*)text); // Adiciona texto ao buffer
    render_on_display(ssd, &frame_area);  // Renderiza o buffer no display
}

// Função para desenhar uma linha no display
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    ssd1306_draw_line(ssd, x1, y1, x2, y2, true); // Adiciona a linha ao buffer
    render_on_display(ssd, &frame_area);          // Renderiza no display
}

// Função para exibir um bitmap no display
void oled_draw_bitmap(const uint8_t *bitmap) {
    memcpy(ssd, bitmap, ssd1306_buffer_length); // Copia o bitmap para o buffer
    render_on_display(ssd, &frame_area);        // Renderiza o bitmap no display
}
