#include <vita2d.h>
#include <string.h>
#include <stdlib.h>
#include "font.h"
#include "gui.h"

bitmap_font* font_load(vita2d_texture *tex, vec_t *map, uint32_t map_len, uint8_t width, uint8_t height)
{
	bitmap_font *f = (bitmap_font*)malloc(sizeof(bitmap_font));
	if(f == NULL) return f;

	f->bitmap = tex;
	f->positions = map;
	f->char_width = width;
	f->char_height = height;
	f->len = map_len;
	return f;
}

void font_draw_char(bitmap_font *f, uint16_t x, uint16_t y, char c)
{
	vita2d_draw_texture_part(f->bitmap, x, y, f->positions[c].x, f->positions[c].y, f->char_width, f->char_height);
}

void font_draw_char_scale(bitmap_font *f, uint16_t x, uint16_t y, char c, uint32_t scale)
{
        vita2d_draw_texture_part_scale(f->bitmap, x, y, f->positions[c].x, f->positions[c].y, f->char_width, f->char_height, scale, scale);
}

void font_draw_string(bitmap_font *f, uint16_t x, uint16_t y, const char *s)
{
	int i = 0;
	int end = strlen(s);
	for(; i < end; i++)
	{
		font_draw_char(f, x, y, s[i]);
		x += f->char_width;
	}
}

void font_draw_string_scale(bitmap_font *f, uint16_t x, uint16_t y, const char *s, uint32_t scale)
{
        int i = 0;
        int end = strlen(s);
        for(; i < end; i++)
        {
                font_draw_char_scale(f, x, y, s[i], scale);
                x += f->char_width * scale;
        }
}
