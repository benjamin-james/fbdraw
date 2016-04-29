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

struct circle {
	struct pt origin, velocity;
	int radius, color;
	int coll;
	struct circle *next;
};

struct circle rand_circle(int w, int h, int max_v, int min_r, int max_r)
{
	struct circle c;
	int v = rand() % max_v;
	double angle = 2 * M_PI * rand() / RAND_MAX;
	c.origin.x = rand() % w;
	c.origin.y = rand() % h;
	c.velocity.x = v * cos(angle);
	c.velocity.y = v * sin(angle);
	c.radius = rand() % (max_r - min_r) + min_r;
	c.color = rand();
	c.coll = 0;
	return c;
}
int check_wall(struct circle *c, int w, int h)
{
	if (c->origin.x - c->radius <= 0 && c->velocity.x < 0) {
		c->origin.x = c->radius;
		c->velocity.x = abs(c->velocity.x);
	}
	if (c->origin.x + c->radius >= w && c->velocity.x > 0) {
		c->origin.x = w - c->radius;
		c->velocity.x = -1 * abs(c->velocity.x);
	}
	if (c->origin.y - c->radius <= 0 && c->velocity.y < 0) {
		c->origin.y = c->radius;
		c->velocity.y = abs(c->velocity.y);
	}
	if (c->origin.y + c->radius >= h && c->velocity.y > 0) {
		c->origin.y = h - c->radius;
		c->velocity.y = -1 * abs(c->velocity.y);
	}
	return 0;
}

int va1(int va0, int vb0, int ma, int mb)
{
	double tot = 0;
	tot += va0 * (double)(ma - mb) / (ma + mb);
	tot += vb0 * (double)(2.0 * mb) / (ma + mb);
	return (int)tot;
}
int check_collision(struct circle *a, struct circle *b)
{
	int dx = a->origin.x - b->origin.x;
	int dy = a->origin.y - b->origin.y;
	int r = a->radius + b->radius;
	int dist2 = dx * dx + dy * dy;
	if (dist2 > r * r) {
		return 0;
	}
	int ma = a->radius * a->radius;
	int mb = b->radius * b->radius;
	int ax = a->velocity.x, ay = a->velocity.y;
	int bx = b->velocity.x, by = b->velocity.y;
	int dot = dx * (bx - ax) + dy * (by - ay);
	if (dot <= 0) {
		return 0;
	}
	double scale = (double)dot / dist2;
	double xc = dx * scale;
	double yc = dy * scale;
	double m = ma + mb;
	double ca = 2 * mb / m;
	double cb = 2 * ma / m;
	a->velocity.x += ca * xc;
	a->velocity.y += ca * yc;
	b->velocity.x += cb * xc;
	b->velocity.y += cb * yc;
/*	a->velocity.x = va1(ax, bx, ma, mb);
	a->velocity.y = va1(ay, by, ma, mb);
	b->velocity.x = va1(bx, ax, mb, ma);
	b->velocity.y = va1(by, ay, mb, ma);*/
	return 1;
}
int insert(struct circle **list, struct circle p)
{
	struct circle *c = malloc(sizeof(*c));
	c->origin = p.origin;
	c->velocity = p.velocity;
	c->radius = p.radius;
	c->color = p.color;
	c->coll = p.coll;
	c->next = *list;
	*list = c;
	return 0;
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
	int w = fb.fb_var_info.xres;
	int h = fb.fb_var_info.yres;
	int min_r = 5;
	int max_r = 200;
	while (running) {
		p.x = rand() % w;
		p.y = rand() % h;
		double radius = rand() % (max_r - min_r) + min_r;
		if (p.x - radius < 0 || p.x + radius >= w ||
		    p.y - radius < 0 || p.y + radius >= h) {
			continue;
		}
		fill_circle(&fb, p, radius, rand());
		refresh(&fb);
		usleep(100);
	}
	fb_uninit(&fb);
	return 0;
}
