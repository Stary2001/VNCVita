#define do_cursorBPP concat(do_cursor_, BPP)
#define do_rawBPP concat(do_raw_, BPP)

int do_cursorBPP(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	PIXEL *pixels = (PIXEL*) malloc(w * h * BYTES_PER_PIXEL);
	if(read_from_server(c, pixels, w * h * BYTES_PER_PIXEL) < 0) { free(pixels); return -1; }

	int mask_w = (w+7)/8;
	uint32_t mask_len = mask_w * h;

	char *mask = malloc(mask_len);
	if(read_from_server(c, mask, mask_len) < 0) { free(pixels); free(mask); return -1; }

	c->cursor_tex = vita2d_create_empty_texture_format(w, h, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_RGBA);
	
	unsigned int s = vita2d_texture_get_stride(c->cursor_tex) / 4;
	uint32_t *tex_data = vita2d_texture_get_datap(c->cursor_tex);
	memset(tex_data, 0, w * h * 4);

	int xx, yy;

	unsigned int red_mask = c->format.red_max << c->format.red_shift;
	unsigned int green_mask = c->format.green_max << c->format.green_shift;
	unsigned int blue_mask = c->format.blue_max << c->format.blue_shift;

	for(yy = 0; yy < h; yy++)
	{
		for(xx = 0; xx < w; xx++)
		{
			int off = 7-(xx % 8);
			int a = (mask[xx/8 + yy * mask_w] & (1 << off)) >> off;
			a *= 0xff;

			PIXEL p = pixels[xx + yy * w];
			int r = (p & red_mask)   >> c->format.red_shift;
			int g = (p & green_mask) >> c->format.green_shift;
			int b = (p & blue_mask)  >> c->format.blue_shift;

			tex_data[xx + yy * s] = (r&0xff) << 24 | (g&0xff) << 16 | (b&0xff) << 8 | (a&0xff);
		}
	}

	free(pixels);
	free(mask);

	return 0;
}
