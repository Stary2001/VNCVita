#include "vnc.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <psp2/net/net.h>
#include <psp2/kernel/threadmgr.h>
#include <vita2d.h>
#include <gui/font.h>

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

uint8_t vnc_read_pixel_8bpp(vnc_client *c)
{
	uint8_t x;
	sceNetRecv(c->client_fd, &x, 1, 0);
	return x;
}

uint16_t vnc_read_pixel_16bpp(vnc_client *c)
{
	uint16_t x;
        sceNetRecv(c->client_fd, &x, 2, 0);
        return x;
}

uint32_t vnc_read_pixel_32bpp(vnc_client *c)
{
	uint32_t x;
	sceNetRecv(c->client_fd, &x, 4, 0);
	return x;
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
	if(pix.bpp == 32)
	{
		return SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB;
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
				//printf("enc %d", encoding);
				switch(encoding)
				{
					case 0:
					{
						do_raw(c, x, y, w, h);
					}
					break;

					case 1: // copyrect
					{
						do_copyrect(c, x, y, w, h);
					}
					break;
					
					case 2: // rre
					{
						do_rre(c, x, y, w, h);
					}
					break;
			
					case 5: // hextile
					{
						do_hextile(c, x, y, w, h);
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

	//encodings[0] = 5; // hextile
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
