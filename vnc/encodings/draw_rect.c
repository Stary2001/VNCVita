#define draw_rectBPP concat(draw_rect_, BPP)

void draw_rectBPP(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h, PIXEL colour)
{
	#if BPP == 32
		colour |= 0xff << 24;
	#endif

	PIXEL *fb = (PIXEL*) c->framebuffer;

	int i = 0;
	int j = 0;
	int idx = x + c->width * y;

	for(; i < h; i++)
	{
		for(j=0; j < w; j++)
		{
			fb[idx++] = colour;
		}
		idx += c->width;
	}
}
