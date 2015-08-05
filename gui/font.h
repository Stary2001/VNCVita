#pragma once
#include <stdint.h>
#include "vec.h"

struct bitmap_font
{
	uint32_t len;
	uint8_t char_width;
	uint8_t char_height;
	struct vec_t* positions;
	vita2d_texture *bitmap;
};
typedef struct bitmap_font bitmap_font;

bitmap_font* font_load(vita2d_texture *tex, struct vec_t *map, uint32_t map_len, uint8_t width, uint8_t height);

void font_draw_char(bitmap_font *f, uint16_t x, uint16_t y, char c);
void font_draw_char_scale(bitmap_font *f, uint16_t x, uint16_t y, char c, uint32_t scale);
void font_draw_string(bitmap_font *f, uint16_t x, uint16_t y, const char *s);
void font_draw_string_scale(bitmap_font *f, uint16_t x, uint16_t y, const char *s, uint32_t scale);

