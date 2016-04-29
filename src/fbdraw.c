#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <wchar.h>
#include <math.h>

#include "fbdraw.h"

#define SWAP(T,x,y) {T *p = &(x); T *q = &(y); T z = *p; *p = *q; *q = z;}

static inline int _location(struct fb *fb, int x, int y)
{
	return x + fb->vinfo.xoffset * (fb->vinfo.bits_per_pixel >> 3) + (y + fb->vinfo.yoffset) * fb->finfo.line_length;
}

extern inline void buffer_swap(struct fb *fb)
{
	if (fb->vinfo.yoffset == 0) {
		fb->vinfo.yoffset = fb->screensize;
	} else {
		fb->vinfo.yoffset = 0;
	}
	ioctl(fb->fd, FBIOPAN_DISPLAY, &fb->vinfo);
	SWAP(uint8_t*, fb->front, fb->back);
}
int set_color(struct fb *fb, uint32_t color)
{
	fb->color = color;
	return 0;
}
int set_pixel(struct fb *fb, int x, int y)
{
	if (x < 0 || x > fb->vinfo.xres || y < 0 || y > fb->vinfo.yres) {
		return -1;
	}
	int offset = _location(fb, x, y);
	memcpy(fb->back + offset, &fb->color, sizeof(fb->color));
	return 0;
}

int fb_uninit(struct fb *fb)
{
	munmap(fb->front, fb->screensize * 2);
	close(fb->fd);
	return 0;
}

int fb_init(struct fb *fb)
{
	fb->fd = open("/dev/fb0", O_RDWR);
	if (fb->fd < 0) {
		fb->fd = open("/dev/graphics/fb0", O_RDWR);
		if (fb->fd < 0) {
			perror("open");
			return -1;
		}
	}
	ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo);
	fb->vinfo.grayscale = 0;
	fb->vinfo.bits_per_pixel = 32;
	ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo);
	ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->finfo);
	fb->screensize = fb->vinfo.yres_virtual * fb->finfo.line_length;
	fb->front = mmap(0, fb->screensize * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
	if (fb->front == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
	fb->back = fb->front + fb->screensize;
	fb->color = pixel_color(0, 0, 0, &fb->vinfo);
	return 0;
}

static inline uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo *vinfo)
{
	return (r<<vinfo->red.offset) | (g << vinfo->green.offset) | (b << vinfo->blue.offset);
}

int fill_screen(struct fb *fb)
{
	/*
	size_t block_size = sizeof(fb->color);
	uint8_t offset = fb->vinfo.xoffset * (fb->vinfo.bits_per_pixel >> 3) + fb->vinfo.yoffset * fb->finfo.line_length;
	memmove(fb->back + offset, &fb->color, block_size);
	uint8_t *start = fb->back + offset;
	uint8_t *current = start + block_size;
	uint8_t *end = fb->back + fb->screensize;
	while (current + block_size < end) {
		memmove(current, start, block_size);
		current += block_size;
		block_size *= 2;
	}
	return memmove(current, start, (size_t)(end-current)) == NULL ? -1 : 0;
	*/
	return wmemset((wchar_t *)fb->back, fb->color, fb->screensize) == NULL ? -1 : 0;
}

int fill_rect(struct fb *fb, struct rect r)
{
	int cx, cy;
	if (r.x + r.w > fb->vinfo.xres) {
		r.w = fb->vinfo.xres - r.x;
	}
	if (r.y + r.h > fb->vinfo.yres) {
		r.h = fb->vinfo.yres - r.y;
	}
	for (cy = r.y; cy < r.y + r.h; cy++) {
		for (cx = r.x; cx < r.x + r.w; cx++) {
			set_pixel(fb, cx, cy);
		}
	}
	return 0;
}

int draw_straight_line(struct fb *fb, struct pt from, struct pt to, int width)
{
	struct rect r;
	if (from.y == to.y) {
		r.w = abs(from.x - to.x);
		r.h = width;
		if (to.x < from.x) {
			r.x = to.x;
			r.y = to.y - (width >> 1);
		} else {
			r.x = from.x;
			r.y = from.y - (width >> 1);
		}
	} else if (from.x == to.x) {
		r.w = width;
		r.h = abs(from.y - to.y);
		if (to.y < from.y) {
			r.x = to.x - (width >> 1);
			r.y = to.y;
		} else {
			r.x = from.x - (width >> 1);
			r.y = from.y;
		}
	} else {
		return -1;
	}
	return fill_rect(fb, r);
}

int refresh(struct fb *fb)
{
	fb->vinfo.activate |= FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;
	fb->vinfo.yres_virtual = fb->vinfo.yres * 2;
	fb->vinfo.yoffset = fb->vinfo.yres;
	return ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo);
}

int fill_circle(struct fb *fb, struct pt origin, int radius)
{
	for (int y = -radius; y < radius; y++) {
		for (int x = -radius; x < radius; x++) {
			if (x*x + y*y <= radius * radius) {
				set_pixel(fb, origin.x + x, origin.y + y);
			}
		}
	}
	return 0;
}
int draw_line(struct fb *fb, struct pt from, struct pt to, int width)
{
	if (from.y == to.y || from.x == to.x) {
		return draw_straight_line(fb, from, to, width);
	}
	int steep = abs(from.y - to.y) > abs(from.x - to.x);
	if (steep) {
		SWAP(int, from.x, from.y);
		SWAP(int, to.x, to.y);
	}
	if (from.x > to.x) {
		SWAP(int, from.x, to.x);
		SWAP(int, from.y, to.y);
	}
	int dx = to.x - from.x;
	int dy = abs(from.y - to.y);
	double error = dx / 2.0;
	int ystep = (from.y < to.y) ? 1 : -1;
	int y = from.y;
	for (int x = from.x; x < to.x; x++) {
		if (steep) {
			set_pixel(fb, y, x);
		} else {
			set_pixel(fb, x, y);
		}
		error -= dy;
		if (error < 0) {
			y += ystep;
			error += dx;
		}
	}
	return 0;
}
