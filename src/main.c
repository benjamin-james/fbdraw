#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include "fbdraw.h"

volatile sig_atomic_t running = 1;

void sighandler(int signum)
{
	running = 0;
}
void do_ray(struct fb *fb)
{
	int x = rand() % fb->fb_var_info.xres;
	int y = rand() % fb->fb_var_info.yres;
	struct pt to, from = {x, y};
	double radius = 0.0;
	double r_max = rand() % (x * x + y * y);
	double theta = 2 * M_PI * rand() / RAND_MAX;
	int color = rand();
	int dir = rand() % 2 == 0 ? 1 : -1;
	for (radius = 0.0; radius <= r_max; radius += 0.1) {
		to.x = x + radius * cos(theta);
		to.y = y + radius * sin(theta);
		if (to.x > fb->fb_var_info.xres || to.x < 0 ||
			to.y > fb->fb_var_info.yres || to.y < 0) {
			break;
		}
		theta += 0.001 * dir;
		draw_line(fb, from, to, 1, color);
		refresh(fb);
		usleep(10);
	}
}
int main(int argc, char **argv)
{
	struct fb fb;
	struct pt from, to;
	struct sigaction sa;
	sa.sa_handler = &sighandler;
	sigaction(SIGINT, &sa, NULL);
	srand(time(NULL));
	fb_init(&fb);
	fill_screen(&fb, rand());
	int w = fb.fb_var_info.xres;
	int h = fb.fb_var_info.yres;
	while (running) {
		from.x = rand() % w;
		from.y = rand() % h;
		to.x = rand() % w;
		to.y = rand() % h;
		draw_line(&fb, from, to, 1, rand());
		refresh(&fb);
		usleep(100);
	}
	fb_uninit(&fb);
	return 0;
}
