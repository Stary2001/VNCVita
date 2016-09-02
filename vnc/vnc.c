#include "vnc.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <psp2/net/net.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>

#ifdef DEBUGGER_IP
#include <debugnet.h>
#define debugOut(...) \
    debugNetPrintf(DEBUG, __VA_ARGS__);
#else
#define debugOut(...)
#endif

#define BLOCK_SIZE 1024

int read_chunked(vnc_client *c, void* buff, int len)
{
	int read_len = 0;
	int ret = 0;

	while(len != 0)
	{
		ret = sceNetRecv(c->client_fd, (char*)buff + read_len, len < BLOCK_SIZE ? len : BLOCK_SIZE, 0);
		if(ret < 0)
		{
			debugOut("read failed, %d!", ret); // rip.
			sceKernelDelayThread(5000000);
			vnc_close(c);
			return -1;
		}
		read_len += ret;
		len -= ret;
	}

	return read_len;
}

int read_from_server(vnc_client *c, void* buff, int len)
{
	int buffered = c->buffer_front - c->buffer_back;
	if(len <= buffered)
	{
		memcpy(buff, c->buffer + c->buffer_back, len);
		c->buffer_back += len;
	}
	else // Buffer underflow!
	{
		if(buffered != 0)
		{
			memmove(c->buffer, c->buffer + c->buffer_back, buffered);
			c->buffer_front = buffered;
			c->buffer_back = 0;
		}
		else
		{
			c->buffer_front = 0;
			c->buffer_back = 0;
		}

		int r = sceNetRecv(c->client_fd, c->buffer + c->buffer_front, VNC_BUFFER_SIZE - c->buffer_front, 0); // Expand at the front.
		if(r < 0)
		{
			sceKernelDelayThread(5000000);
			sceKernelExitProcess(0);
		}

		if(r == len) // we got EXACTLY the amount we needed. we wasted the buffer. :(
		{
			// this means we don't update either position, as the buffer 'didnt change'
			debugOut("success, exact amount!\n", len);
			memcpy(buff, c->buffer + c->buffer_front, len);
			return r;
		}
		else if(r + buffered >= len)
		{
			memcpy(buff, c->buffer + c->buffer_back, len);
			c->buffer_back += len;
			c->buffer_front += r;
		}
		else
		{
			debugOut("underflow failed (need %i have %i)!\n", len, r + buffered);
			sceKernelDelayThread(5000000);
			sceKernelExitProcess(0);
		}
	}

	return 0;
}

vnc_client *vnc_create(const char *host, int port)
{
	vnc_client *c = (vnc_client*)malloc(sizeof(vnc_client));
	c->buffer = malloc(VNC_BUFFER_SIZE);
	c->buffer_front = 0;
	c->buffer_back = 0;

	c->state = HANDSHAKE;
	c->shared = 1;
	c->draw = 0;

	c->epoll_fd = sceNetEpollCreate("epoll", 0);
	if(c->epoll_fd == -1)
	{
		free(c);
		return NULL;
	}

	c->client_fd = sceNetSocket("vnc", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
	if(c->client_fd == -1)
	{
		free(c);
		return NULL;
	}

	SceNetInAddr addr;
	SceNetSockaddrIn sockaddr;
	sceNetInetPton(SCE_NET_AF_INET, host, &addr);

	sockaddr.sin_family = SCE_NET_AF_INET;
	sockaddr.sin_addr = addr;
	sockaddr.sin_port = sceNetHtons(port);

	if(sceNetConnect(c->client_fd, (SceNetSockaddr *)&sockaddr, sizeof(sockaddr)))
	{
		debugOut("connect failed\n");
		sceNetSocketClose(c->client_fd);
		free(c);
		return NULL;
	}

	SceNetEpollEvent ev = {0};
	ev.events = SCE_NET_EPOLLIN | SCE_NET_EPOLLHUP;
	ev.data.fd = c->client_fd;

	int i = 0;
	if((i = sceNetEpollControl(c->epoll_fd, SCE_NET_EPOLL_CTL_ADD, c->client_fd, &ev)))
	{
		debugOut("epoll failed return %i %i %x", c->epoll_fd, c->client_fd, i);
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
	read_from_server(c, &len, 4);
	len = sceNetNtohl(len);
	char *buf = (char*) malloc(len);
	read_from_server(c, &buf, len);
	return buf;
}

struct vnc_pixelformat vnc_read_pixel_format(vnc_client *c)
{
	struct vnc_pixelformat p;
	read_from_server(c, &p, sizeof(p));
	p.red_max = sceNetNtohs(p.red_max);
	p.green_max = sceNetNtohs(p.green_max);
	p.blue_max = sceNetNtohs(p.blue_max);
	return p;
}

struct vnc_serverinit vnc_read_serverinit(vnc_client *c)
{
	struct vnc_serverinit packet;
	read_from_server(c, &packet.width, 2);
	read_from_server(c, &packet.height, 2);
	packet.width = sceNetNtohs(packet.width);
	packet.height = sceNetNtohs(packet.height);
	packet.format = vnc_read_pixel_format(c);
	read_from_server(c, &packet.name_len, 4);
	packet.name_len = sceNetNtohl(packet.name_len);
	packet.name = (char*)malloc(packet.name_len + 1);
	read_from_server(c, packet.name, packet.name_len);
	packet.name[packet.name_len] = 0;
	return packet;
}

uint8_t vnc_read_pixel_8bpp(vnc_client *c)
{
	uint8_t x;
	read_from_server(c, &x, 1);
	return x;
}

uint16_t vnc_read_pixel_16bpp(vnc_client *c)
{
	uint16_t x;
		read_from_server(c, &x, 2);
		return x;
}

uint32_t vnc_read_pixel_32bpp(vnc_client *c)
{
	uint32_t x;
	read_from_server(c, &x, 4);
	return x;
}

void vnc_close(vnc_client *c)
{
	sceNetEpollControl(c->epoll_fd, SCE_NET_EPOLL_CTL_DEL, c->client_fd, NULL);
	sceNetSocketClose(c->client_fd);
}

void vnc_handle_message(vnc_client *c);

SceGxmTextureFormat get_gxm_format(struct vnc_pixelformat pix)
{
	if(!pix.true_colour)	return 0;
	if(pix.bpp == 32)
	{
		if(pix.blue_shift == 0)
		{
			return SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB;
		}
		else if(pix.red_shift == 0)
		{
			return SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
		}
	}
	else if(pix.bpp == 16)
	{
		if(pix.blue_shift == 0)
		{
			return SCE_GXM_COLOR_FORMAT_U5U6U5_RGB;
		}
		else if(pix.red_shift == 0)
		{
			return SCE_GXM_COLOR_FORMAT_U5U6U5_BGR;
		}
	}

	return 0;
}

void vnc_handle(vnc_client *c)
{
	SceNetEpollEvent ev = {0};
	sceNetEpollWait(c->epoll_fd, &ev, 1, 1000/60);
	if(ev.events & SCE_NET_EPOLLIN)
	{
		switch(c->state)
		{
			case HANDSHAKE:
			{
				char proto[13];
				read_from_server(c, proto, 12);
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
					read_from_server(c, &num_types, 1);
					if(num_types == 0)
					{
						char *r = vnc_read_reason(c);
						debugOut("%s", r);
						free(r);
						vnc_close(c);
						return;
					}
					else
					{
						char *types = (char*)malloc(num_types);
						read_from_server(c, types, num_types);
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
					read_from_server(c, &type, 1);
					if(type == 0)
					{
						char *r = vnc_read_reason(c);
						debugOut(r);
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
								read_from_server(c, &status, 4);
				status = sceNetNtohl(status);
				if(status == 1)
				{
					if(c->minor == 8)
					{
						char *r = vnc_read_reason(c);
						debugOut(r);
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
				c->framebuffer_tex = vita2d_create_empty_texture_format(c->width, c->height, gxm_format);
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
	read_from_server(c, &message_type, 1);
	debugOut("message type %i\n", message_type);
	switch(message_type)
	{
		case 0: // framebuffer update
		{
			char padding;
			read_from_server(c, &padding, 1);
			uint16_t num_rects = 0;
			read_from_server(c, &num_rects, 2);
			num_rects = sceNetNtohs(num_rects);

			//debugOut("!!!!fbupdate with %i rects!!!!\n", num_rects);
			int i = 0;
			for(; i < num_rects; i++)
			{
				uint16_t x,y,w,h;
				x = y = w = h = 0;
				read_from_server(c, &x, 2);
				read_from_server(c, &y, 2);
				read_from_server(c, &w, 2);
				read_from_server(c, &h, 2);
				x = sceNetNtohs(x);
				y = sceNetNtohs(y);
				w = sceNetNtohs(w);
				h = sceNetNtohs(h);
				int encoding = 0;
				read_from_server(c, &encoding, 4);
				encoding = sceNetNtohl(encoding);
				//debugOut("rect %i, %i,%i %ix%i enc %i\n", i, x, y, w, h, encoding);

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

					case -239: // cursor
						do_cursor(c, x, y, w, h);
					break;

					default:
						debugOut("unknown encoding %i, exiting.\n", encoding);
						sceKernelExitProcess(1);
					break;
				}
			}
			c->draw = 1;
			vnc_send_update_request(c, 1, 0, 0, c->width, c->height);
		}
		break;

		case 2: // bell
			debugOut("vnc bell\x07\n");
		break;

		case 3: // cut text
		{
			char padding[3];
			read_from_server(c, &padding, 3);
			int len = 0;
			read_from_server(c, &len, 4);
			len = sceNetNtohl(len);
			debugOut("clipboard is %i bytes long\n", len);
			char *buff = (char*)malloc(len+1);
			read_from_server(c, buff, len);
			buff[len] = 0;
			debugOut("clipboard is now '%s'\n", buff);
			free(buff);
		}
		break;

		default:
			debugOut("unknown message type %i, exiting\n", message_type);
			sceKernelExitProcess(1);
		break;
	}
}

void vnc_send_encodings(vnc_client *c)
{
	struct vnc_set_encodings packet = {0};
	uint16_t num_encodings = 4;
	int encodings[4];

	encodings[0] = 5; // hextile
	encodings[1] = 2; // rre
	encodings[2] = 1; // copyrect
	encodings[3] = 0; // raw
	//encodings[4] = -239; // cursor
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
