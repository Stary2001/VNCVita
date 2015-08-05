#include "vnc.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <psp2/net/net.h>
#include <psp2/kernel/threadmgr.h>
#include <vita2d.h>
#include "../gui/font.h"

extern bitmap_font *font;

void log_line(const char *format, ...)
{
	char buff[1024];
	va_list args;
	va_start(args, format);
	vsprintf(buff, format, args);

	vita2d_start_drawing();
        vita2d_clear_screen();
        font_draw_string(font, 10, 0, buff);
        vita2d_end_drawing();
        vita2d_swap_buffers();

	va_end(args);
}

#define BLOCK_SIZE 1024
int read_from_server(vnc_client *c, void* buff, int len)
{
        int read_len = 0;
        int ret = 0;
        while(len != 0)
        {
                ret = sceNetRecv(c->client_fd, (char*)buff + read_len, len < BLOCK_SIZE ? len : BLOCK_SIZE, 0);
                if(ret < 0)
                {
                        log_line("read failed, %d!", ret); // rip.
			sceKernelDelayThread(5000000);
                        vnc_close(c);
			return -1;
                }
                read_len += ret;
                len -= ret;
        }
	return read_len;
}

vnc_client * vnc_create(const char *host, int port)
{
	vnc_client *c = (vnc_client*)malloc(sizeof(vnc_client));
	c->state = HANDSHAKE;
	c->shared = 1;
	c->draw = 0;

	c->epoll_fd = sceNetEpollCreate("epoll", 0);
	if(c->epoll_fd == -1)
	{
		free(c);
		return NULL;
	}

	c->client_fd = sceNetSocket("vnc", PSP2_NET_AF_INET, PSP2_NET_SOCK_STREAM, 0);
	if(c->client_fd == -1)
	{
		free(c);
		return NULL;
	}

	
	SceNetInAddr addr;
	SceNetSockaddrIn sockaddr;
	sceNetInetPton(PSP2_NET_AF_INET, host, &addr);
	
	sockaddr.sin_family = PSP2_NET_AF_INET;
        sockaddr.sin_addr = addr;
        sockaddr.sin_port = sceNetHtons(port);

	if(sceNetConnect(c->client_fd, (SceNetSockaddr *)&sockaddr, sizeof(sockaddr)))
	{
		log_line("connect failed");
		sceNetSocketClose(c->client_fd);
		free(c);
		return NULL;
	}
	
	SceNetEpollEvent ev = {0};
	ev.events = PSP2_NET_EPOLLIN | PSP2_NET_EPOLLHUP;
	ev.data.fd = c->client_fd;

	int i = 0;
	if((i = sceNetEpollControl(c->epoll_fd, PSP2_NET_EPOLL_CTL_ADD, c->client_fd, &ev)))
	{
		log_line("epoll failed return %i %i %x", c->epoll_fd, c->client_fd, i);
		sceKernelDelayThread(10000000);
		sceNetSocketClose(c->client_fd);
		free(c);
		return NULL;
	}
	return c;
}

char* vnc_read_reason(vnc_client *c)
{
	uint32_t len = 0;
	sceNetRecv(c->client_fd, &len, 4, 0);
	len = sceNetNtohl(len);
	char *buf = (char*) malloc(len);
	sceNetRecv(c->client_fd, &buf, len, 0);
	return buf;
}

struct vnc_pixelformat vnc_read_pixel_format(vnc_client *c)
{
	struct vnc_pixelformat p;
	sceNetRecv(c->client_fd, &p, sizeof(p), 0);
	p.red_max = sceNetNtohs(p.red_max);
	p.green_max = sceNetNtohs(p.green_max);
	p.blue_max = sceNetNtohs(p.blue_max);
	return p;
}

struct vnc_serverinit vnc_read_serverinit(vnc_client *c)
{
	struct vnc_serverinit packet;
	sceNetRecv(c->client_fd, &packet.width, 2, 0);
	sceNetRecv(c->client_fd, &packet.height, 2, 0);
	packet.width = sceNetNtohs(packet.width);
	packet.height = sceNetNtohs(packet.height);
	packet.format = vnc_read_pixel_format(c);
	sceNetRecv(c->client_fd, &packet.name_len, 4, 0);
	packet.name_len = sceNetNtohl(packet.name_len);
	packet.name = (char*)malloc(packet.name_len + 1);
	sceNetRecv(c->client_fd, packet.name, packet.name_len, 0);
	packet.name[packet.name_len] = 0;
	return packet;
}

void vnc_close(vnc_client *c)
{
	sceNetEpollControl(c->epoll_fd, PSP2_NET_EPOLL_CTL_DEL, c->client_fd, NULL);
	sceNetSocketClose(c->client_fd);
}

void vnc_handle_message(vnc_client *c);

SceGxmTextureFormat get_gxm_format(struct vnc_pixelformat pix)
{
	if(!pix.true_colour)	return 0;
	if(pix.bpp == 24)
	{
		if(pix.red_shift == 16)
		{
			return SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR;
		}
		else
		{
			return SCE_GXM_TEXTURE_FORMAT_U8U8U8_RGB;
		}
	}
	else if(pix.bpp == 32)
	{
		return SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_BGRA;
	}
}

void vnc_handle(vnc_client *c)
{
	SceNetEpollEvent ev = {0};
	sceNetEpollWait(c->epoll_fd, &ev, 1, 1000/60);
	if(ev.events & PSP2_NET_EPOLLIN)
	{
		switch(c->state)
		{
			case HANDSHAKE:
			{
				char proto[13];
				sceNetRecv(c->client_fd, proto, 12, 0);
				proto[12] = 0;
				int major, minor;
				major=minor=0;
				sscanf(proto, "RFB 00%d.00%d", &major, &minor);
				if(major == 3)
				{
					// yay.
					c->state = AUTH;
					c->minor = minor;
					sceNetSend(c->client_fd, proto, 12, 0);
				}
				else
				{
					vnc_close(c);
					return;
				}
			}
			break;
			case AUTH:
			{
				if(c->minor > 7) // rfb 3.7+
				{
					char num_types;
					sceNetRecv(c->client_fd, &num_types, 1, 0);
					if(num_types == 0)
					{
						char *r = vnc_read_reason(c);
						log_line(r);
						free(r);
						vnc_close(c);
						return;
					}
					else
					{
						char *types = (char*)malloc(num_types);
						sceNetRecv(c->client_fd, types, num_types, 0);
						char selected = 0;
						int i = 0;
						for(; i < num_types; i++)
						{
							if(types[i] == 1)
							{
								selected = 1;
								break;
							}
						}
						if(selected == 0)
						{
							vnc_close(c);
							return;
						}
						else
						{
							sceNetSend(c->client_fd, &selected, 1, 0);
							if(selected == 1 && c->minor != 8)
							{
								c->state = INIT;
								sceNetSend(c->client_fd, &c->shared, 1, 0);
							}
							else
							{
								c->state = AUTH_REPLY;
							}
						}
					}
				}
				else // rfb 3.3
				{
					char type;
                                        sceNetRecv(c->client_fd, &type, 1, 0);
                                        if(type == 0)
                                        {
                                                char *r = vnc_read_reason(c);
                                                log_line(r);
                                                free(r);
                                                vnc_close(c);
                                                return;
                                        }
				}
			}
			break;
			case AUTH_REPLY:
			{
				uint32_t status;
                                sceNetRecv(c->client_fd, &status, 4, 0);
				status = sceNetNtohl(status);
				if(status == 1)
				{
					if(c->minor == 8)
					{
						char *r = vnc_read_reason(c);
						log_line(r);
	                                        free(r);
					}
                                        vnc_close(c);
                                        return;
				}
				c->state = INIT;
				sceNetSend(c->client_fd, &c->shared, 1, 0);
			}
			break;

			case INIT:
			{
				struct vnc_serverinit p = vnc_read_serverinit(c);
				c->width = p.width;
				c->height = p.height;
				c->format = p.format;
				c->server_name = malloc(p.name_len + 1);
				strcpy(p.name, c->server_name);
				free(p.name);
				c->state = NORMAL;
				SceGxmTextureFormat gxm_format = get_gxm_format(c->format);
				c->framebuffer_tex = vita2d_create_empty_texture(c->width, c->height);
				c->framebuffer = (uint32_t*) vita2d_texture_get_datap(c->framebuffer_tex);
				vnc_send_encodings(c);
				vnc_send_update_request(c, 0, 0, 0, c->width, c->height);
			}
			break;
			case NORMAL:
				vnc_handle_message(c);
			break;
		}
	}
}


void do_raw(vnc_client *c, char *pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	int i = 0;
	int bpp = c->format.bpp / 8; // bytes per pixel

	int r = c->format.red_shift / 8;
	int g = c->format.green_shift / 8;
	int b = c->format.blue_shift / 8;

	for(; i < h; i++)
	{
		int j = 0;
		for(; j < w; j++)
		{
			c->framebuffer[(c->width * (i + y)) + j + x] = RGBA8(pixels[(i*w + j) * bpp + r], pixels[(i*w + j) * bpp + g], pixels[(i*w + j) * bpp + b], 0xff);
		}
	}
}

void do_copyrect(vnc_client *c, uint16_t copy_x, uint16_t copy_y, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	int i = 0;
	for(; i < h; i++)
	{
		memcpy(c->framebuffer + x + (y * c->width), c->framebuffer + copy_x + ( (copy_y + i) * c->width), w);
	}
}

void do_rre(vnc_client *c, uint32_t bg, uint32_t num_subrects, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	/*if(num_subrects != 0)
	{
		log_line("rre, bg %d with %d subrects, x %d y %d w %d h %d", bg, num_subrects, x, y, w, h);
		sceKernelDelayThread(750000);
	}*/

	if(c->format.bpp == 32)
	{
		uint32_t t = RGBA8(bg >> c->format.red_shift & 0xff, bg >> c->format.green_shift & 0xff, bg >> c->format.blue_shift & 0xff, 0xff);
		bg = t;
	}

	int i = 0;
	int j = 0;
	int k = 0;

	for(; i < h; i++)
	{
		j = 0;
		for(; j < w; j++)
		{
			c->framebuffer[(c->width * (i + y)) + j + x] = bg;
		}
	}

	if(c->format.bpp == 32 && num_subrects != 0)
	{
		struct vnc_rre_tile_32bpp *tiles = malloc(sizeof(struct vnc_rre_tile_32bpp) * num_subrects);
		read_from_server(c, tiles, sizeof(struct vnc_rre_tile_32bpp) * num_subrects);
		uint32_t t = 0;
		i = 0;
		for(; i < num_subrects; i++)
		{
			tiles[i].x = sceNetNtohs(tiles[i].x);
			tiles[i].y = sceNetNtohs(tiles[i].y);
			tiles[i].w = sceNetNtohs(tiles[i].w);
			tiles[i].h = sceNetNtohs(tiles[i].h);
			t = RGBA8(tiles[i].colour >> c->format.red_shift & 0xff, tiles[i].colour >> c->format.green_shift & 0xff, tiles[i].colour >> c->format.blue_shift, 0xff);
			// draw..
			for(j=0; j < tiles[i].h; j++)
			{
				for(k=0; k < tiles[i].w; k++)
				{
					c->framebuffer[(c->width * (tiles[i].y+y+j)) + tiles[i].x + x + k] = t;
				}
			}
		}
	}
}

void vnc_handle_message(vnc_client *c)
{
	char message_type = -1;
	sceNetRecv(c->client_fd, &message_type, 1, 0);
	switch(message_type)
	{
		case 0: // framebuffer update
		{
			char padding;
			sceNetRecv(c->client_fd, &padding, 1, 0);
			uint16_t num_rects = 0;
			sceNetRecv(c->client_fd, &num_rects, 2, 0);
			num_rects = sceNetNtohs(num_rects);
			sceKernelDelayThread(5000000);
			int i = 0;
			for(; i < num_rects; i++)
			{
				//log_line("rect %d", i);
				uint16_t x,y,w,h;
				x = y = w = h = 0;
				sceNetRecv(c->client_fd, &x, 2, 0);
				sceNetRecv(c->client_fd, &y, 2, 0);
				sceNetRecv(c->client_fd, &w, 2, 0);
				sceNetRecv(c->client_fd, &h, 2, 0);
				x = sceNetNtohs(x);
				y = sceNetNtohs(y);
				w = sceNetNtohs(w);
				h = sceNetNtohs(h);
				int encoding = 0;
				sceNetRecv(c->client_fd, &encoding, 4, 0);
				encoding = sceNetNtohl(encoding);
				switch(encoding)
				{
					case 0:
					{
						uint32_t len = w * h * (c->format.bpp / 8);
						char *update = (char*) malloc(len);
						int ret = read_from_server(c, update, len);
						if(ret == -1)
						{
							free(update);
							return;
						}
						//sceKernelDelayThread(1000000);
						// stuff into texture..
						do_raw(c, update, x, y, w, h);
						free(update);
					}
					break;

					case 1: // copyrect
					{
						uint16_t copy_x = 0;
						uint16_t copy_y = 0;
						sceNetRecv(c->client_fd, &copy_x, 2, 0);
						sceNetRecv(c->client_fd, &copy_y, 2, 0);
						copy_x = sceNetNtohs(copy_x);
						copy_y = sceNetNtohs(copy_y);
						//log_line("copyrekt %d %d -> %d %d", copy_x, copy_y, x, y);
						sceKernelDelayThread(5000000);
						do_copyrect(c, copy_x, copy_y, x, y, w, h);
					}
					break;
					
					case 2: // rre
					{
						uint32_t num_subrects = 0;
						uint32_t bg = 0;
						sceNetRecv(c->client_fd, &num_subrects, 4, 0);
						if(c->format.bpp == 32)
						{
							sceNetRecv(c->client_fd, &bg, 4, 0);
						}
						num_subrects = sceNetNtohl(num_subrects);
						do_rre(c, bg, num_subrects, x, y, w, h);
					}
					break;

					default:
						log_line("unknown encoding %i", encoding);
						sceKernelDelayThread(10000000);
					break;
				}
			}
			c->draw = 1;
			vnc_send_update_request(c, 1, 0, 0, c->width, c->height);
		}
		break;
	}
}

void vnc_send_encodings(vnc_client *c)
{
	struct vnc_set_encodings packet = {0};
	uint16_t num_encodings = 3;
	int encodings[3];

	encodings[0] = 2; // rre
	encodings[1] = 1; // copyrect
	encodings[2] = 0; // raw
	packet.type = 2;
	packet.num_encodings = sceNetHtons(num_encodings);

	int i = 0;
	for(; i < num_encodings; i++)
	{
		encodings[i] = sceNetHtonl(encodings[i]);
	}
	sceNetSend(c->client_fd, &packet, sizeof(packet), 0);
	sceNetSend(c->client_fd, &encodings, num_encodings * sizeof(int), 0);
}

void vnc_send_update_request(vnc_client *c, uint8_t inc, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	struct vnc_fb_update_request r;
	r.type = 3;
	r.incremental = inc;
	r.x = sceNetHtons(x);
	r.y = sceNetHtons(y);
	r.w = sceNetHtons(w);
	r.h = sceNetHtons(h);
	sceNetSend(c->client_fd, &r, sizeof(r), 0);
}
