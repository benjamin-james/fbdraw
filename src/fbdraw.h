#ifndef FBDRAW_H
#define FBDRAW_H

#include <inttypes.h>
#include <linux/fb.h>
#include <stdlib.h>

struct pt { int x, y; };
struct rect { int x, y, w, h; };

struct fb {
	int fd;
	uint8_t *front, *back;
	uint32_t color;
	long screensize;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
};

int fb_init(struct fb *fb);
int fb_uninit(struct fb *fb);
int refresh(struct fb *fb);
int set_color(struct fb *fb, uint32_t color);
int fill_screen(struct fb *fb);
int fill_rect(struct fb *fb, struct rect r);
int draw_straight_line(struct fb *fb, struct pt from, struct pt to, int width);
int draw_line(struct fb *fb, struct pt from, struct pt to, int width);
int fill_circle(struct fb *fb, struct pt origin, int radius);
int set_pixel(struct fb *fb, int x, int y);

static inline uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo *vinfo);
inline void buffer_swap(struct fb *fb);
#endif
