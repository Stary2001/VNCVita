#include "gui.h"

void gui_render(gui_object *o)
{
	switch(o->type)
	{
		case GUI_PANE:
			gui_render_pane(o);
		case GUI_LABEL:
			gui_render_label(o);
	}
}

void gui_layout(gui_object *o)
{
	switch(o->type)
	{
		case GUI_PANE:
			gui_layout_pane(o);
		break;
		case GUI_LABEL:
			// doesn't need layout
		break;
	}
}
