#include <stdint.h>
#include <vita2d.h>

typedef enum
{
	HANDSHAKE,
	AUTH,
	AUTH_REPLY,
	INIT,
	NORMAL
} vnc_state;

struct vnc_pixelformat
{
	uint8_t bpp;
	uint8_t depth;
	uint8_t big_endian;
	uint8_t true_colour;
	uint16_t red_max;
	uint16_t green_max;
	uint16_t blue_max;
	uint8_t red_shift;
	uint8_t green_shift;
	uint8_t blue_shift;
	char padding[3];
};

struct vnc_serverinit
{
	uint16_t width;
	uint16_t height;
	struct vnc_pixelformat format;
	uint32_t name_len;
	char* name;
};

struct vnc_fb_update_request
{
	uint8_t type;
	uint8_t incremental;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
};

struct vnc_set_encodings
{
	uint8_t type;
	uint8_t padding;
	uint16_t num_encodings;
};

struct vnc_pointer_event
{
	uint8_t type;
	uint8_t buttons;
	uint16_t x;
	uint16_t y;
};

#define VNC_BUFFER_SIZE 0x100000

struct vnc_client
{
	int epoll_fd;
	int client_fd;
	int minor;
	char shared;

	uint16_t width;
	uint16_t height;
	char *server_name;
	struct vnc_pixelformat format;

	vita2d_texture *framebuffer_tex;
	void *framebuffer;
	uint8_t draw;
	vnc_state state;

	int buffer_front;
	int buffer_back;
	char *buffer;

	vita2d_texture *cursor_tex;
	uint8_t buttons;
	int cursor_x;
	int cursor_y;

	uint8_t old_buttons;
	int old_cursor_x;
	int old_cursor_y;
};
typedef struct vnc_client vnc_client;

int vnc_read_pixel_8bpp(vnc_client *c, uint8_t *pix);
int vnc_read_pixel_16bpp(vnc_client *c, uint16_t *pix);
int vnc_read_pixel_32bpp(vnc_client *c, uint32_t *pix);

vnc_client *vnc_create(const char *host, int port);
void vnc_close(vnc_client *c);
int vnc_handle(vnc_client *c);
void vnc_send_update_request(vnc_client *c, uint8_t inc, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void vnc_send_encodings(vnc_client *c);
SceGxmTextureFormat vnc_get_gxm_format(struct vnc_pixelformat pix);

int read_from_server(vnc_client *c, void* buff, int len);

int do_raw(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
int do_copyrect(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
int do_rre(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
int do_hextile(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
int do_cursor(vnc_client *c, uint16_t x, uint16_t y, uint16_t w, uint16_t h);