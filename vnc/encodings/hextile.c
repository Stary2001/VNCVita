#include <stdio.h>

#define do_hextileBPP concat(do_hextile_, BPP)
#define do_rawBPP concat(do_raw_, BPP)
#define draw_rectBPP concat(draw_rect_, BPP)

int do_hextileBPP(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	PIXEL bg = 0;
	PIXEL fg = 0;

	uint16_t num_tiles_x = w%16 == 0 ? w/16 : (w/16)+1;
	uint16_t num_tiles_y = h%16 == 0 ? h/16 : (h/16)+1;

	uint16_t tile_x = 0;
	uint16_t tile_y = 0;
	uint16_t tile_w = 16;
	uint16_t tile_h = 16;
	uint8_t subrect_count = 0;
	uint8_t subrects_coloured = 0;

	uint16_t orig_x = x;

	uint32_t i = 0;

	for(; tile_y < num_tiles_y; tile_y++,y+=tile_h)
	{
		if(tile_y == num_tiles_y - 1)
		{
			tile_h = (h % 16) != 0 ? h % 16 : 16;	
		}
		else
		{
			tile_h = 16;
		}

		for(x=orig_x,tile_x = 0; tile_x < num_tiles_x; tile_x++,x+=tile_w)
		{
			if(tile_x == num_tiles_x - 1)
			{
				tile_w = (w % 16) != 0 ? w % 16 : 16;
			}
			else
			{
				tile_w = 16;
			}

			char subenc = 0;
			if(read_from_server(c, &subenc, 1) < 0) { return -1; }
			if(subenc & 1)
			{
				if(do_rawBPP(c, x, y, tile_w, tile_h) < 0) { return -1; }
				bg = 0;
				fg = 0;
			}
			else
			{
				if(subenc & 2) // background
				{
					if(vnc_read_pixel(c, &bg) < 0) { return -1; }
				}

				if(subenc & 4)
				{
					if(vnc_read_pixel(c, &fg) < 0) { return -1; }
				}

				if(subenc & 8)
				{
					if(read_from_server(c, &subrect_count, 1) < 0) { return -1; }
				}

				if(subenc & 16)
				{
					fg = 0;
					subrects_coloured = 1;
				}

				draw_rectBPP(c, x, y, tile_w, tile_h, bg);

				if(subrect_count != 0)
				{
					uint8_t xy = 0;
					uint8_t wh = 0;
					if(subrects_coloured)
					{
						for(i = 0; i < subrect_count; i++)
						{
							PIXEL p;
							if(vnc_read_pixel(c, &p) < 0) { return -1; }
							if(read_from_server(c, &xy, 1) < 0) { return -1; }
							if(read_from_server(c, &wh, 1) < 0) { return -1; }
							draw_rectBPP(c, x + ((xy >> 4) & 0xf), y + (xy & 0xf), ((wh >> 4) & 0xf) + 1, (wh & 0xf) + 1, p);
						}
						subrects_coloured = 0;
					}
					else
					{
						for(i = 0; i < subrect_count; i++)
						{
							if(read_from_server(c, &xy, 1) < 0) { return -1; }
							if(read_from_server(c, &wh, 1) < 0) { return -1; }
							draw_rectBPP(c, x + ((xy >> 4) & 0xf), y + (xy & 0xf), ((wh >> 4) & 0xf) + 1, (wh & 0xf) + 1, fg);
						}
					}
					subrect_count = 0;
				}
			}
		}
	}

	return 0;
}
