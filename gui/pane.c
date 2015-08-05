#include <stdint.h>
#include <stdlib.h>
#include <vita2d.h>
#include "gui.h"

gui_object *gui_new_pane(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)
{
	gui_object *ptr = (gui_object*)malloc(sizeof(gui_object) + sizeof(gui_pane));
        ptr->type = GUI_PANE;
	ptr->data_len = sizeof(gui_pane);
	ptr->data = ptr + sizeof(gui_object);
	ptr->pos.x = x;
	ptr->pos.y = y;
	ptr->screen_pos.x = x;
	ptr->screen_pos.y = y;

	gui_pane *pane = (gui_pane*) ptr->data;

	pane->size.x = w;
	pane->size.y = h;
	pane->color = color;
	pane->items = list_create();
	
	return ptr;
}

void gui_pane_add_item(gui_object *pane, gui_object *sub)
{
	if(pane->type != GUI_PANE) return;
	gui_pane *p = (gui_pane*)pane->data;
	list_add(p->items, sub);
	gui_layout_pane(pane);
}

void gui_render_pane(gui_object *pane)
{
	if(pane->type != GUI_PANE) return;
	gui_pane *p = (gui_pane*)pane->data;
	// render children...
	vita2d_draw_rectangle(pane->screen_pos.x, pane->screen_pos.y, p->size.x, p->size.y, p->color);
	struct list_item *i = p->items->head;
	while(i != NULL)
	{
		gui_render((gui_object*)i->ptr);
		i = i->next;
		if( i == NULL ) break;
	}
}

void gui_layout_pane(gui_object *pane)
{
	if(pane->type != GUI_PANE) return;
	gui_pane *p = (gui_pane*)pane->data;

	struct list_item *i = p->items->head;
	while(i != NULL)
        {
		gui_object *o = (gui_object*) i->ptr;
		if( o == NULL ) break;
		o->screen_pos.x = pane->screen_pos.x + o->pos.x;
		o->screen_pos.y = pane->screen_pos.y + o->pos.y;
		gui_layout(o);

                i = i->next;
                if( i == NULL ) break;
        }
}
