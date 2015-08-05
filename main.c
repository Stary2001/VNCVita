/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/moduleinfo.h>
#include <psp2/kernel/processmgr.h>

#include <vita2d.h>
#include "gui/gui.h"
#include "gui/font.h"
#include "vnc/vnc.h"
#include <stdio.h>

PSP2_MODULE_INFO(0, 0, "gui");

extern struct vec_t *map;
extern uint32_t map_len;

/*void print( const char *c )
{
	vita2d_start_drawing();
	font_draw_string(10, 10, c);
	vita2d_end_drawing();
	vita2d_swap_buffers();
}*/

 bitmap_font *font;

int main()
{
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0xaa, 0xaa, 0xaa, 0xff));

	vita2d_texture *tex = vita2d_load_PNG_file("cache0:/VitaDefilerClient/Documents/font.png");
	font = font_load(tex, map, map_len, 8, 8);

/*	gui_object *p = gui_new_pane(100, 100, 336, 48, RGBA8(0xff, 0, 0, 0xff)); // x y w h color
	gui_object *l = gui_new_label(font, "testing testing 123", 16, 16);
	((gui_label*)l->data)->scale = 2;
	gui_pane_add_item(p, l); */

	/* Input variables */
	SceCtrlData pad;
	
	vnc_client *vnc = NULL;

	while (1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);

                if (pad.buttons & PSP2_CTRL_START) break;
		if (pad.buttons & PSP2_CTRL_SELECT && !vnc)
		{
			vnc = vnc_create("192.168.43.166", 5900);
		}
		
		if(vnc)
		{
			sceKernelDelayThread(1000000/120);
			vnc_handle(vnc);
		}

		/*if(vnc && vnc->draw)
		{
			vnc_send_update_request(vnc, 1, 0, 0, vnc->width, vnc->height);
		}*/

		vita2d_start_drawing();
		vita2d_clear_screen();
		if(vnc && vnc->draw)
		{
			vita2d_draw_texture_scale(vnc->framebuffer_tex, 0, 0, 950/(float)vnc->width, 544/(float)vnc->height);
		}

		vita2d_end_drawing();
		vita2d_swap_buffers();
	}
	vita2d_fini();
	vita2d_free_texture(tex);

	sceKernelExitProcess(0);
	return 0;
}
