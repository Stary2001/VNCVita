#include <stdint.h>
#include <stdlib.h>
#include <vita2d.h>
#include "gui.h"
#include "font.h"

gui_object *gui_new_label(bitmap_font *font, const char *text, uint16_t x, uint16_t y)
{
	gui_object *ptr = (gui_object*)malloc(sizeof(gui_object) + sizeof(gui_label));
        ptr->type = GUI_LABEL;
	ptr->pos.x = x;
	ptr->pos.y = y;
	ptr->screen_pos.x = x;
	ptr->screen_pos.y = y;

	ptr->data_len = sizeof(gui_label);
	ptr->data = ptr + sizeof(gui_object);
	gui_label *label = (gui_label*) ptr->data;
	label->font = font;
	label->text = text;
	label->scale = 1;

	return ptr;
}

void gui_render_label(gui_object *label)
{
	if(label->type != GUI_LABEL) return;
	gui_label *l = (gui_label*)label->data;
	if(l->scale == 1)
		font_draw_string(l->font, label->screen_pos.x, label->screen_pos.y, l->text);
	else
		font_draw_string_scale(l->font, label->screen_pos.x, label->screen_pos.y, l->text, l->scale);
}
