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

int main(int argc, char **argv)
{
	struct fb fb;
	struct pt p;
	struct sigaction sa;
	sa.sa_handler = &sighandler;
	sigaction(SIGINT, &sa, NULL);
	srand(time(NULL));
	fb_init(&fb);
	int w = fb.vinfo.xres;
	int h = fb.vinfo.yres;
	int min_r = 5;
	int max_r = 200;
	while (running) {
		set_color(&fb, rand());
		p.x = rand() % w;
		p.y = rand() % h;
		double radius = rand() % (max_r - min_r) + min_r;
		if (p.x - radius < 0 || p.x + radius >= w ||
		    p.y - radius < 0 || p.y + radius >= h) {
			continue;
		}
		fill_circle(&fb, p, radius);
		buffer_swap(&fb);
		refresh(&fb);
		usleep(100);
	}
	fb_uninit(&fb);
	return 0;
}
