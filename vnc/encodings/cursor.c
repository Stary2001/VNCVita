#define do_cursorBPP concat(do_cursor_, BPP)
#define do_rawBPP concat(do_raw_, BPP)

void do_cursorBPP(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	PIXEL *pixels = (PIXEL*) malloc(w * h * BYTES_PER_PIXEL);
	read_from_server(c, pixels, w * h * BYTES_PER_PIXEL);
	free(pixels);

	uint32_t mask_len = (w+7)/8 * h;
	char *mask = malloc(mask_len);
	read_from_server(c, mask, mask_len);
	free(mask);

	// todo.
}
