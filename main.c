#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>

#include <vita2d.h>
#include "vnc/vnc.h"
#include <stdio.h>
#include <stdlib.h>
#include <debugnet.h>

#define DEBUGGER_IP "192.168.1.244"
#define DEBUGGER_PORT 18194
#define VNC_IP "192.168.1.244"
#define VNC_PORT 5900
int main()
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    SceNetInitParam netInitParam;
    int size = 1024 * 512;
    netInitParam.memory = malloc(size);
    netInitParam.size = size;
    netInitParam.flags = 0;
    sceNetInit(&netInitParam);

	debugNetInit(DEBUGGER_IP, DEBUGGER_PORT, DEBUG);

	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x88, 0x88, 0x88, 0xff));

	vita2d_font *font = vita2d_load_font_file("app0:font.ttf");

	SceCtrlData pad;

	vnc_client *vnc = NULL;

	char disconnected_text[128];
	snprintf(disconnected_text, 128, "not connected\npress START to quit, SELECT to connect to %s", VNC_IP);

	uint32_t last = 0;
	uint32_t pressed = 0;

	while (1)
	{
		sceCtrlPeekBufferPositive(0, &pad, 1);
		pressed = (last ^ pad.buttons) & pad.buttons; // xor gives us both edges, we only want the rising edge

        if (pressed & SCE_CTRL_START) break;
		if (pressed & SCE_CTRL_SELECT)
		{
			if(!vnc)
			{
				vnc = vnc_create(VNC_IP, VNC_PORT);
			}
			else
			{
				vnc_close(vnc);
				free(vnc);
				vnc = NULL;
			}
		}

		if(vnc)
		{
			vnc_handle(vnc);
		}

		vita2d_start_drawing();
		vita2d_clear_screen();
		if(vnc && vnc->draw)
		{
			vita2d_draw_texture_scale(vnc->framebuffer_tex, 0, 0, 960/(float)vnc->width, 544/(float)vnc->height);
		}

		if(!vnc)
		{
			vita2d_font_draw_text(font, 0, 0, RGBA8(0, 0, 0, 0xff), 20, disconnected_text);
		}
		vita2d_end_drawing();
		vita2d_swap_buffers();

		last = pad.buttons;
	}
	vita2d_fini();

	debugNetFinish();
	sceKernelExitProcess(0);
	return 0;
}
