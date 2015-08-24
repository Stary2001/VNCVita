#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vnc.h"
#include <psp2/net/net.h>

#include <debugnet.h>

#define real_concat(a,b) a ## b
#define concat(a,b) real_concat(a,b)

#define real_concat3(a,b,c) a ## b ## c
#define concat3(a,b,c) real_concat3(a,b,c)

#define BYTES_PER_PIXEL (BPP/8)

void draw_rect_8(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t colour);
void draw_rect_16(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour);
void draw_rect_32(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t colour);

extern int read_from_server(vnc_client *c, void* buff, int len);

#define vnc_read_pixel concat3(vnc_read_pixel_,BPP,bpp)
#define PIXEL uint32_t
#define BPP 32
#include "encodings/raw.c"
#include "encodings/copyrect.c"
#include "encodings/rre.c"
#include "encodings/hextile.c"
#include "encodings/draw_rect.c"
#include "encodings/cursor.c"
#undef PIXEL
#undef BPP
#define PIXEL uint16_t
#define BPP 16
#include "encodings/raw.c"
#include "encodings/copyrect.c"
#include "encodings/rre.c"
#include "encodings/hextile.c"
#include "encodings/draw_rect.c"
#include "encodings/cursor.c"
#undef PIXEL
#undef BPP
#define PIXEL uint8_t
#define BPP 8
#include "encodings/raw.c"
#include "encodings/copyrect.c"
#include "encodings/rre.c"
#include "encodings/hextile.c"
#include "encodings/draw_rect.c"
#include "encodings/cursor.c"

void do_copyrect(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
        switch(c->format.bpp)
        {
                case 32:
                        do_copyrect_32(c, x, y, w, h);
                break;
                case 16:
                        do_copyrect_16(c, x, y, w, h);
                break;
                case 8:
                        do_copyrect_8(c, x, y, w, h);
                break;
        }
}

void do_hextile(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
        switch(c->format.bpp)
        {
                case 32:
                        do_hextile_32(c, x, y, w, h);
                break;
                case 16:
                        do_hextile_16(c, x, y, w, h);
                break;
                case 8:
                        do_hextile_8(c, x, y, w, h);
                break;
        }
}

void do_raw(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
        switch(c->format.bpp)
        {
                case 32:
                        do_raw_32(c, x, y, w, h);
                break;
                case 16:
                        do_raw_16(c, x, y, w, h);
                break;
                case 8:
                        do_raw_8(c, x, y, w, h);
                break;
        }
}

void do_rre(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
        switch(c->format.bpp)
        {
                case 32:
                        do_rre_32(c, x, y, w, h);
                break;
                case 16:
                        do_rre_16(c, x, y, w, h);
                break;
                case 8:
                        do_rre_8(c, x, y, w, h);
                break;
        }
}

void do_cursor(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
        switch(c->format.bpp)
        {
                case 32:
                        do_cursor_32(c, x, y, w, h);
                break;
                case 16:
                        do_cursor_16(c, x, y, w, h);
                break;
                case 8:
                        do_cursor_8(c, x, y, w, h);
                break;
        }
}

