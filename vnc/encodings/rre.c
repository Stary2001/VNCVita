#undef vnc_rre_tile
#define vnc_rre_tile concat(vnc_rre_tile,BPP)
#define do_rreBBP concat(do_rre_, BPP)

struct vnc_rre_tile
{
        PIXEL colour;
        uint16_t x;
        uint16_t y;
        uint16_t w;
        uint16_t h;
};

void do_rreBBP(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	uint32_t num_subrects = 0;
	read_from_server(c, &num_subrects, 4);
	num_subrects = sceNetNtohl(num_subrects);
	debugNetPrintf(DEBUG, "%i subrects", num_subrects);

	PIXEL bg = vnc_read_pixel(c);

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
                        ((PIXEL*)c->framebuffer)[(c->width * (i + y)) + j + x] = bg;
                }
        }

        if(c->format.bpp == 32 && num_subrects != 0)
        {
                struct vnc_rre_tile *tiles = malloc(sizeof(struct vnc_rre_tile) * num_subrects);
                read_from_server(c, tiles, sizeof(struct vnc_rre_tile) * num_subrects);
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
                                        ((PIXEL*)c->framebuffer)[(c->width * (tiles[i].y+y+j)) + tiles[i].x + x + k] = t;
                                }
                        }
                }
        }
}
