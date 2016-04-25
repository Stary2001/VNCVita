#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/moduleinfo.h>
#include <psp2/kernel/processmgr.h>

#include <vita2d.h>
#include "vnc/vnc.h"
#include <stdio.h>

PSP2_MODULE_INFO(0, 0, "VNCVita");

#include <debugnet.h>

#define DEBUGGER_IP "192.168.0.13"
#define DEBUGGER_PORT 18194
#define VNC_IP "192.168.0.13"
#define VNC_PORT 5900
int main()
{
	debugNetInit(DEBUGGER_IP, DEBUGGER_PORT, DEBUG);
	debugNetPrintf(DEBUG, "MAGIC!!\n");

	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x88, 0x88, 0x88, 0xff));

	SceCtrlData pad;
	
	vnc_client *vnc = NULL;

	while (1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);

                if (pad.buttons & SCE_CTRL_START) break;
		if (pad.buttons & SCE_CTRL_SELECT && !vnc)
		{
			vnc = vnc_create(VNC_IP, VNC_PORT);
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
		vita2d_end_drawing();
		vita2d_swap_buffers();
	}
	vita2d_fini();
	
	debugNetFinish();
	sceKernelExitProcess(0);
	return 0;
}
