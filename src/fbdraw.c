#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <wchar.h>
#include <math.h>

#include "fbdraw.h"

#define SWAP(T,x,y) {T *p = &(x); T *q = &(y); T z = *p; *p = *q; *q = z;}

int memset8(void *dest, unsigned int data, size_t len)
{
	return memset(dest, data, len) == NULL ? -1 : 0;
}
int memset32(void *dest, unsigned int data, size_t len)
{
	return wmemset(dest, data, len) == NULL ? -1 : 0;
}
int set_pixel_32(struct fb *fb, int offset, int color)
{
	if (offset < 0 || offset > fb->fb_var_info.xres * fb->fb_var_info.yres) {
		return -1;
	}
	*(((unsigned int*) fb->screen) + offset) = color;
	return 0;
}
int set_pixel_8(struct fb *fb, int offset, int color)
{
	if (offset < 0) {
		offset = 0;
	} else if (offset > fb->fb_var_info.xres * fb->fb_var_info.yres) {
		offset = fb->fb_var_info.xres * fb->fb_var_info.yres;
	}
	*(fb->screen + offset) = (char)color;
	return 0;
}

int fb_uninit(struct fb *fb)
{
	munmap(fb->screen, fb->fb_fix_info.smem_len);
	close(fb->fd);
	return 0;
}

int fb_init(struct fb *fb)
{
	fb->fd = open("/dev/fb0", O_RDWR);
	if (fb->fd < 0) {
		fb->fd = open("/dev/graphics/fb0", O_RDWR);
		if (fb->fd < 0) {
			return -1;
		}
	}
	ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->fb_var_info);
	ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fb_fix_info);
	switch (fb->fb_var_info.bits_per_pixel) {
	case 8: {
		fb->memset = &memset8;
		fb->set_pixel = &set_pixel_8;
		break;
	}
	case 16:
	case 24:
	case 32: {
		fb->memset = &memset32;
		fb->set_pixel = &set_pixel_32;
		break;
	}
	}
	fb->screen = mmap(0, fb->fb_fix_info.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
	fb->screen += (fb->fb_var_info.xoffset + fb->fb_var_info.yoffset * fb->fb_var_info.xres_virtual) * (fb->fb_var_info.bits_per_pixel >> 3);
	return 0;
}

int fill_screen(struct fb *fb, int color)
{
	return fb->memset(fb->screen, color, fb->fb_var_info.xres * fb->fb_var_info.yres);
}

int fill_rect(struct fb *fb, struct rect r, int color)
{
	int cx, cy;
	if (r.x + r.w > fb->fb_var_info.xres) {
		r.w = fb->fb_var_info.xres - r.x;
	}
	if (r.y + r.h > fb->fb_var_info.yres) {
		r.h = fb->fb_var_info.yres - r.y;
	}
	for (cy = r.y; cy < r.y + r.h; cy++) {
		fb->memset(((unsigned int*)fb->screen) + fb->fb_var_info.xres * cy + r.x, color, r.w);
	}
	return 0;
}

int draw_straight_line(struct fb *fb, struct pt from, struct pt to, int width, int color)
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
	return fill_rect(fb, r, color);
}

int refresh(struct fb *fb)
{
	fb->fb_var_info.activate |= FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;
	fb->fb_var_info.yres_virtual = fb->fb_var_info.yres * 2;
	fb->fb_var_info.yoffset = fb->fb_var_info.yres;
	return ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->fb_var_info);
}

int draw_line(struct fb *fb, struct pt from, struct pt to, int width, int color)
{
	if (from.y == to.y || from.x == to.x) {
		return draw_straight_line(fb, from, to, (int)width, color);
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
			fb->set_pixel(fb, y + fb->fb_var_info.xres * x, color);
		} else {
			fb->set_pixel(fb, x + fb->fb_var_info.xres * y, color);
		}
		error -= dy;
		if (error < 0) {
			y += ystep;
			error += dx;
		}
	}
	return 0;
}
