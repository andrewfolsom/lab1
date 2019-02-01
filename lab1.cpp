//
//modified by: Andrew Folsom
//date: 1/24/2019
//
//3350 Spring 2018 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
// .general animation framework
// .animation loop
// .object definition and movement
// .collision detection
// .mouse/keyboard interaction
// .object constructor
// .coding style
// .defined constants
// .use of static variables
// .dynamic memory allocation
// .simple opengl components
// .git
//
//elements we will add to program...
//   .Game constructor
//   .multiple particles
//   .gravity
//   .collision detection
//   .more objects
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"

const int MAX_PARTICLES = 20000;
const int MAX_BOXES = 5;
const float GRAVITY = -0.1;

//some structures

struct Vec {
	float x, y, z;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
};

struct Particle {
	Shape s;
	Vec velocity;
};

class Global {
public:
	int xres, yres;
	Shape box[MAX_BOXES];
	Particle particle[MAX_PARTICLES];
	int n;
	bool flow = 0;
	Global() {
		xres = 800;
		yres = 600;
		//define a box shape
		for (int i = 0; i < MAX_BOXES; i++) {
			box[i].width = 100;
			box[i].height = 10;
			box[i].center.x = 120 + i*65;
			box[i].center.y = 500 - i*60;
		}
		n = 0;
	}
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	X11_wrapper() {
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		int w = g.xres, h = g.yres;
		dpy = XOpenDisplay(NULL);
		if (dpy == NULL) {
			cout << "\n\tcannot connect to X server\n" << endl;
			exit(EXIT_FAILURE);
		}
		Window root = DefaultRootWindow(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if (vi == NULL) {
			cout << "\n\tno appropriate visual found\n" << endl;
			exit(EXIT_FAILURE);
		} 
		Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		XSetWindowAttributes swa;
		swa.colormap = cmap;
		swa.event_mask =
			ExposureMask | KeyPressMask | KeyReleaseMask |
			ButtonPress | ButtonReleaseMask |
			PointerMotionMask |
			StructureNotifyMask | SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
			InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	void set_title() {
		//Set the window title bar.
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "3350 Lab1");
	}
	bool getXPending() {
		//See if there are pending events.
		return XPending(dpy);
	}
	XEvent getXNextEvent() {
		//Get a pending event.
		XEvent e;
		XNextEvent(dpy, &e);
		return e;
	}
	void swapBuffers() {
		glXSwapBuffers(dpy, win);
	}
} x11;

//Function prototypes
void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void movement();
void render();
void fountain();



//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
	srand(time(NULL));
	init_opengl();
	//Main animation loop
	int done = 0;
	while (!done) {
		//Process external events.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			check_mouse(&e);
			done = check_keys(&e);
		}
		fountain();
		movement();
		render();
		x11.swapBuffers();
	}
	cleanup_fonts();
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
}

void makeParticle(int x, int y)
{
	if (g.n >= MAX_PARTICLES)
		return;
	//cout << "makeParticle() " << x << " " << y << endl;
	//position of particle
	Particle *p = &g.particle[g.n];
	p->s.center.x = x;
	p->s.center.y = y;
	p->velocity.y = -4.0;
	p->velocity.x =  1.0;
	++g.n;
}

void fountain()
{
	int x = 150;
	int y = 650;
	int num;
	if (g.flow) {
		for (int i = 0; i < 10; i++) {
			num = y + (rand() % 50);
			makeParticle(x, num);
		}
	}
}

void check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed
			int y = g.yres - e->xbutton.y;
			makeParticle(e->xbutton.x, y);
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
		}
	//	int y = g.yres - e->xbutton.y;
	//	for (int i = 0; i < 10; ++i) {
	//	    makeParticle(e->xbutton.x, y);
	//	}
	}
}

int check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_1:
				//Key 1 was pressed
				break;
			case XK_a:
				g.flow ^= 1;
				break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void movement()
{
	if (g.n <= 0)
		return;
	for (int i = 0; i < g.n; i++) {
		Particle *p = &g.particle[i];
		p->s.center.x += p->velocity.x;
		p->s.center.y += p->velocity.y;
		p->velocity.y += GRAVITY;

		//check for collision with shapes...
		for (int i = 0; i < MAX_BOXES; i++) {
			Shape *s = &g.box[i];
			if (p->s.center.y < s->center.y + s->height &&
				p->s.center.y > s->center.y - s->height &&
				p->s.center.x > s->center.x - s->width &&
				p->s.center.x < s->center.x + s->width) {
	    		//bounce
	    		p->velocity.y = -p->velocity.y / 3;
			}
		}

		//check for off-screen
		if (p->s.center.y < 0.0) {
			//cout << "off screen" << endl;
			g.particle[i] = g.particle[--g.n];
		}	
	}
}

void render()
{
	Rect r[MAX_BOXES];
	glClear(GL_COLOR_BUFFER_BIT);
	//Draw shapes...
	//
	//draw a box
	float w, h;
	int col1 = 90;
	int col2 = 140;
	int col3 = 90;
	for(int i = 0; i < MAX_BOXES; i++) {
		Shape *s = &g.box[i];
		glPushMatrix();
		glColor3ub(col1,col2,col3);
		glTranslatef(s->center.x, s->center.y, s->center.z);
		w = s->width;
		h = s->height;
		glBegin(GL_QUADS);
			glVertex2i(-w, -h);
			glVertex2i(-w,  h);
			glVertex2i( w,  h);
			glVertex2i( w, -h);
		glEnd();
		glPopMatrix();
		col1 += 20;
		col2 += 20;
		col3 += 20;
	}
	//
	//Draw the particle here	
	for (int i = 0; i < g.n; i++) {
		glPushMatrix();
		glColor3ub(150,160,220);
		Vec *c = &g.particle[i].s.center;
		w =
		h = 2;
		glBegin(GL_QUADS);
			glVertex2i(c->x-w, c->y-h);
			glVertex2i(c->x-w, c->y+h);
			glVertex2i(c->x+w, c->y+h);
			glVertex2i(c->x+w, c->y-h);
		glEnd();
		glPopMatrix();
	}
	//
	//Draw your 2D text here
	unsigned int c = 0x00000000;

	for (int i = 0; i < MAX_BOXES; i++) {
		r[i].width = g.box[i].width;
		r[i].height = g.box[i].height;
		r[i].center = 10;
		r[i].bot = g.box[i].center.y - 5;
		r[i].left = g.box[i].center.x;
		//r[i].bot = g.yres - 20;
		//r[i].left = 10;
	}

	ggprint8b(&r[0], 16, c, "Requirements");
	ggprint8b(&r[1], 16, c, "Design");
	ggprint8b(&r[2], 16, c, "Implementation");
	ggprint8b(&r[3], 16, c, "Verification");
	ggprint8b(&r[4], 16, c, "Maintenance");
}
