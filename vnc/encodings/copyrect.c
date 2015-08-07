#define do_copyrectBPP concat(do_copyrect_, BPP)

void do_copyrectBPP(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	uint16_t copy_x = 0;
	uint16_t copy_y = 0;
	sceNetRecv(c->client_fd, &copy_x, 2, 0);
	sceNetRecv(c->client_fd, &copy_y, 2, 0);
	copy_x = sceNetNtohs(copy_x);
	copy_y = sceNetNtohs(copy_y);

	uint32_t src_idx = x + y * c->width;
	uint32_t dst_idx = copy_x + copy_y * c->width;
	
        int i = 0;
        for(; i < h; i++)
        {
                memcpy(((PIXEL*)c->framebuffer) + dst_idx, ((PIXEL*)c->framebuffer) + src_idx, w * BYTES_PER_PIXEL);
		src_idx += c->width;
		dst_idx += c->width;
        }
}
