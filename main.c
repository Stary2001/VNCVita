#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/sysmodule.h>
#include <psp2/common_dialog.h>
#include <psp2/ime_dialog.h>
#include <psp2/net/net.h>
#include <psp2/apputil.h>

#include <vita2d.h>
#include "vnc/vnc.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUGGER_IP
#include <debugnet.h>
#endif

static uint16_t title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t initial_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t input_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];

#define IME_TEXT SCE_IME_TYPE_BASIC_LATIN
#define IME_NUM SCE_IME_TYPE_NUMBER

void ascii2utf(uint16_t* dst, const char* src)
{
	if(!src || !dst)return;
	while(*src)*(dst++)=(*src++);
	*dst=0x00;
}

void utf2ascii(char* dst, const uint16_t* src)
{
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

int ime(const char *title_ascii, int type, char *inp, int inp_len, int carry)
{
	ascii2utf(title, title_ascii);
	memset(initial_text, 0, SCE_IME_DIALOG_MAX_TEXT_LENGTH*2);
	memset(input_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH+1)*2);
	if(carry) ascii2utf(initial_text, inp);

	SceImeDialogParam param;
	sceImeDialogParamInit(&param);
	param.supportedLanguages = 0x0001FFFF;
	param.languagesForced = SCE_TRUE;
	param.type = type;
	param.title = title;
	param.maxTextLength = inp_len-1;
	param.initialText = initial_text;
	param.inputTextBuffer = input_text;
	sceImeDialogInit(&param);

	while(1)
	{
		vita2d_start_drawing();
		vita2d_end_drawing();
		vita2d_common_dialog_update();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers();

		SceCommonDialogStatus status = sceImeDialogGetStatus();
		if (status == SCE_COMMON_DIALOG_STATUS_FINISHED)
		{
			SceImeDialogResult result;
			memset(&result, 0, sizeof(SceImeDialogResult));
			sceImeDialogGetResult(&result);

			if (result.button == SCE_IME_DIALOG_BUTTON_ENTER)
			{
				// Yay.
				utf2ascii(inp, input_text);
				sceImeDialogTerm();
				return 0;
			}

			sceImeDialogTerm();
			return -1;
		}
	}
}

char vnc_host[256];

int host_entered = 0;
int vnc_port = 5900;

char disconnected_text[256];
void update_text()
{
	snprintf(disconnected_text, 256, "not connected\npress START to quit, SELECT to connect to %s:%i\npress X to enter VNC host\npress O to enter VNC port", vnc_host, vnc_port);
}

#define PAD_DEAD 20

int main()
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    SceNetInitParam netInitParam;
    int size = 1024 * 512;
    netInitParam.memory = malloc(size);
    netInitParam.size = size;
    netInitParam.flags = 0;
    sceNetInit(&netInitParam);

#ifdef DEBUGGER_IP
	debugNetInit(DEBUGGER_IP, DEBUGGER_PORT, DEBUG);
#endif

	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});

	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x88, 0x88, 0x88, 0xff));

	vita2d_font *font = vita2d_load_font_file("app0:font.ttf");

	SceCtrlData pad;

	vnc_client *vnc = NULL;

	uint32_t last = 0;
	uint32_t pressed = 0;

	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
	sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam){});

	strcpy(vnc_host, "(none)");
	update_text();

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

	while (1)
	{
		sceCtrlPeekBufferPositive(0, &pad, 1);
		pressed = (last ^ pad.buttons) & pad.buttons; // xor gives us both edges, we only want the rising edge

        if (pressed & SCE_CTRL_START) break;
		if (pressed & SCE_CTRL_SELECT && host_entered)
		{
			if(!vnc)
			{
				vnc = vnc_create(vnc_host, vnc_port);
			}
			else
			{
				vnc_close(vnc);
				free(vnc);
				vnc = NULL;
			}
		}

		// Input
		if(vnc)
		{
			int pad_x = pad.lx - 127;
			int pad_y = pad.ly - 127;
			if(abs(pad_x) > PAD_DEAD || abs(pad_y) > PAD_DEAD)
			{
				int frames = 60;
				// Movement!
				vnc->cursor_x += (pad_x * (vnc->width / 128.0)) / frames;
				vnc->cursor_y += (pad_y * (vnc->height / 128.0)) / frames;
				debugNetPrintf(DEBUG, "%i %i\n", vnc->cursor_x, vnc->cursor_y);

				// Clamp.
				if(vnc->cursor_x < 0) { vnc->cursor_x = 0; }
				if(vnc->cursor_x > vnc->width) { vnc->cursor_x = vnc->width; }

				if(vnc->cursor_y < 0) { vnc->cursor_y = 0; }
				if(vnc->cursor_y > vnc->height) { vnc->cursor_y = vnc->height; }
			}

			if((last ^ pad.buttons) & SCE_CTRL_CROSS)
			{
				vnc->buttons ^= 1;
			}

			if((last ^ pad.buttons) & SCE_CTRL_CIRCLE)
			{
				vnc->buttons ^= 2;
			}
		}
		else
		{
			if(pressed & SCE_CTRL_CROSS)
			{
				if(ime("Enter hostname", IME_TEXT, vnc_host, 256, strcmp(vnc_host, "(none)") != 0) < 0)
				{
					// oh no?
				}
				else
				{
					update_text();
					host_entered = 1;
				}
			}

			if(pressed & SCE_CTRL_CIRCLE)
			{
				char vnc_port_buf[256];
				itoa(vnc_port, vnc_port_buf, 10);

				if(ime("Enter port", IME_NUM, vnc_port_buf, 256, 1) < 0)
				{
					// oh no?
				}
				else
				{
					vnc_port = atoi(vnc_port_buf);
					update_text();
				}
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
			float xs = 960/(float)vnc->width;
			float ys = 544/(float)vnc->height;

			vita2d_draw_texture_scale(vnc->framebuffer_tex, 0, 0, xs, ys);
			if(vnc->cursor_tex)
			{
				float c_x = vnc->cursor_x * xs;
				float c_y = vnc->cursor_y * ys;
				vita2d_draw_texture_scale(vnc->cursor_tex, c_x, c_y, xs, ys);
			}
		}

		if(!vnc)
		{
			vita2d_font_draw_text(font, 0, 0, RGBA8(0, 0, 0, 0xff), 20, disconnected_text);
			vita2d_font_draw_text(font, 0, 0, RGBA8(0, 0, 0, 0xff), 20, disconnected_text);
		}
		vita2d_end_drawing();
		vita2d_swap_buffers();

		last = pad.buttons;
	}
	vita2d_fini();

#ifdef DEBUGGER_IP
	debugNetFinish();
#endif
	sceKernelExitProcess(0);
	return 0;
}
