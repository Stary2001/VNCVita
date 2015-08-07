#define do_rawBPP concat(do_raw_, BPP)

void do_rawBPP(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	 uint32_t len = w * h * BYTES_PER_PIXEL;
         PIXEL *pixels = (PIXEL*) malloc(len);
         int ret = read_from_server(c, pixels, len);
         if(ret == -1)
         {
                free(pixels);
         	return;
         }

        int i = 0;
	int j = 0;
	int fb_index = x + y * c->width;
	int pix_index = 0;

	PIXEL *fb = (PIXEL*) c->framebuffer;

        for(; i < h; i++)
        {
		j = 0;
		for(; j < w; j++)
		{
			#if BPP == 32
				pixels[pix_index + j] |= (0xff << 24); // set alpha
			#endif
			fb[fb_index + j] = pixels[pix_index + j];
		}
		fb_index += c->width;
		pix_index += w;
        }
	free(pixels);
}
