#ifndef FBDRAW_H
#define FBDRAW_H

#include <linux/fb.h>
#include <stdlib.h>

struct pt { int x, y; };
struct rect { int x, y, w, h; };

struct fb {
	int fd;
	int8_t *screen;
	int8_t *buffer;
	int8_t offset;
	struct fb_var_screeninfo fb_var_info;
	struct fb_fix_screeninfo fb_fix_info;
	int (*set_pixel)(struct fb *fb, int offset, int color);
	int (*memset)(void *dest, unsigned int data, size_t len);
};

int fb_init(struct fb *fb);
int fb_uninit(struct fb *fb);
int refresh(struct fb *fb);
int fill_screen(struct fb *fb, int color);
int fill_rect(struct fb *fb, struct rect r, int color);
int draw_straight_line(struct fb *fb, struct pt from, struct pt to, int width, int color);
int draw_line(struct fb *fb, struct pt from, struct pt to, int width, int color);
int fill_circle(struct fb *fb, struct pt origin, int radius, int color);
#endif
