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
#include <unistd.h>

const int MAX_PARTICLES = 20000;
const int MAX_BOXES = 5;
const float GRAVITY = -0.1;

//defined types
typedef double Vec2[3];

//marcros
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)

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

class Image {
public:
	int width, height;
	unsigned char *data;
	~Image() { delete [] data; }
	Image(const char *fname) {
		if (fname[0] == '\0')
			return;
		//printf("fname **%s**\n", fname);
		int ppmFlag = 0;
		char name[40];
		strcpy(name, fname);
		int slen = strlen(name);
		char ppmname[80];
		if (strncmp(name+(slen-4), ".ppm", 4) == 0)
			ppmFlag = 1;
		if (ppmFlag) {
			strcpy(ppmname, name);
		} else {
			name[slen-4] = '\0';
			//printf("name **%s**\n", name);
			sprintf(ppmname,"%s.ppm", name);
			//printf("ppmname **%s**\n", ppmname);
			char ts[100];
			//system("convert eball.jpg eball.ppm");
			sprintf(ts, "convert %s %s", fname, ppmname);
			system(ts);
		}
		//sprintf(ts, "%s", name);
		FILE *fpi = fopen(ppmname, "r");
		if (fpi) {
			char line[200];
			fgets(line, 200, fpi);
			fgets(line, 200, fpi);
			//skip comments and blank lines
			while (line[0] == '#' || strlen(line) < 2)
				fgets(line, 200, fpi);
			sscanf(line, "%i %i", &width, &height);
			fgets(line, 200, fpi);
			//get pixel data
			int n = width * height * 3;			
			data = new unsigned char[n];			
			for (int i=0; i<n; i++)
				data[i] = fgetc(fpi);
			fclose(fpi);
		} else {
			printf("ERROR opening image: %s\n",ppmname);
			exit(0);
		}
		if (!ppmFlag)
			unlink(ppmname);
	}
};
Image img = "./images/bigfootSmall.png";

class Bigfoot {
public:
	Vec2 pos;
	Vec2 vel;
} bigfoot;

class Global {
public:
	int xres, yres;
	GLuint bigfootTexture;
	Shape box[MAX_BOXES];
	Shape mouth;
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
		//define mouth shape
		mouth.width = 25;
		mouth.height = 25;
		mouth.center.x = 535;
		mouth.center.y = 100;
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
void init();
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
	init();
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

void init() {
	MakeVector(145.0, -300.0, 0.0, bigfoot.pos);
	MakeVector(0.0, 0.0, 0.0, bigfoot.vel);
}

unsigned char *buildAlphaData(Image *img)
{
	//add 4th component to RGB stream...
	int i;
	int a,b,c;
	unsigned char *newdata, *ptr;
	unsigned char *data = (unsigned char *)img->data;
	newdata = (unsigned char *)malloc(img->width * img->height * 4);
	ptr = newdata;
	for (i=0; i<img->width * img->height * 3; i+=3) {
		a = *(data+0);
		b = *(data+1);
		c = *(data+2);
		*(ptr+0) = a;
		*(ptr+1) = b;
		*(ptr+2) = c;
		//-----------------------------------------------
		//get largest color component...
		//*(ptr+3) = (unsigned char)((
		//		(int)*(ptr+0) +
		//		(int)*(ptr+1) +
		//		(int)*(ptr+2)) / 3);
		//d = a;
		//if (b >= a && b >= c) d = b;
		//if (c >= a && c >= b) d = c;
		//*(ptr+3) = d;
		//-----------------------------------------------
		//this code optimizes the commented code above.
		*(ptr+3) = (a|b|c);
		//-----------------------------------------------
		ptr += 4;
		data += 3;
	}
	return newdata;
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
	glGenTextures(1, &g.bigfootTexture);

	// Adding Bigfoot
	int w = img.width;
	int h = img.height;
	unsigned char *ftData = buildAlphaData(&img);
	glBindTexture(GL_TEXTURE_2D, g.bigfootTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, ftData);
	free(ftData);

	initialize_fonts();
}

void makeParticle(int x, int y)
{
	if (g.n >= MAX_PARTICLES)
		return;
	cout << "makeParticle() " << x << " " << y << endl;
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
		int y = g.yres - e->xbutton.y;
		for (int i = 0; i < 10; ++i) {
		    makeParticle(e->xbutton.x, y);
		}
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

		//check for collision with mouth

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
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
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

	//Bigfoot
	glPushMatrix();
	glTranslatef(bigfoot.pos[0], bigfoot.pos[1], bigfoot.pos[2]);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
	glBindTexture(GL_TEXTURE_2D, g.bigfootTexture);
	glBegin(GL_QUADS);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(0, 0);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(0, g.yres);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(g.xres, g.yres);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(g.xres, 0);
	glEnd();
	glDisable(GL_ALPHA_TEST);
	glPopMatrix();
	//glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

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
