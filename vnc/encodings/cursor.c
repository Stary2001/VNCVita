#define do_cursorBPP concat(do_cursor_, BPP)
#define do_rawBPP concat(do_raw_, BPP)

int do_cursorBPP(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	PIXEL *pixels = (PIXEL*) malloc(w * h * BYTES_PER_PIXEL);
	if(read_from_server(c, pixels, w * h * BYTES_PER_PIXEL) < 0) { free(pixels); return -1; }
	free(pixels);

	uint32_t mask_len = (w+7)/8 * h;
	char *mask = malloc(mask_len);
	if(read_from_server(c, mask, mask_len) < 0) { free(pixels); free(mask); return -1; }
	free(mask);

	// todo.
	return 0;
}
