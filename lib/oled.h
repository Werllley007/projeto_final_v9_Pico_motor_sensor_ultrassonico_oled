#ifndef OLED_H
#define OLED_H

#include <stdint.h>  // Necessário para uint8_t

// Inicializa o OLED
void oled_init();

// Limpa o display
void oled_clear();

// Exibe texto no display
void oled_display_text(const char *text, uint8_t x, uint8_t y);

// Desenha uma linha no display
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

// Exibe um bitmap no display
void oled_draw_bitmap(const uint8_t *bitmap);

#endif // OLED_H
