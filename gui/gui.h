#pragma once

#include <stdint.h>
#include "vec.h"
#include "../util/list.h"

typedef enum 
{
	GUI_PANE,
	GUI_LABEL
} gui_type;

struct gui_object
{
	struct gui_object *parent;
	gui_type type;
	vec_t pos;
	vec_t screen_pos;
	uint32_t data_len;
	void* data;
};
typedef struct gui_object gui_object;

struct bitmap_font;

struct gui_label
{
	struct bitmap_font *font;
	const char *text;
	uint32_t scale;
};
typedef struct gui_label gui_label;


struct gui_pane
{
	vec_t size;
        uint32_t color;
	struct list *items;
};
typedef struct gui_pane gui_pane;

gui_object *gui_new_label(struct bitmap_font *font, const char *text, uint16_t x, uint16_t y);
gui_object *gui_new_pane(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
void gui_render_label(gui_object *label);
void gui_render_pane(gui_object *pane);
void gui_render(gui_object *o);
void gui_pane_add_item(gui_object *pane, gui_object *sub);
void gui_layout(gui_object *o);
void gui_layout_pane(gui_object *pane);
