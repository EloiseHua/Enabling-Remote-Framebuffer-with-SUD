// A simple bouncing lines animation similar to that seen in many early
// screen-saver programs.

#define _GNU_SOURCE
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "common.h"

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>


int randint(int lo, int hi){
  return rand() % (hi-lo+1) + lo;
}


typedef struct
{
  int x, y;
} Pt;

typedef struct
{
  Pt p1;
  Pt p2;
} Line;

//#define W 320
//#define H 240
int W = 320;
int H = 240;
int stride = 320;

int nlines = 79; // Actual number of lines to use
#define MAXLINES 200

static Line lines[MAXLINES];
static int curline = 0;
static Pt vel1;
static Pt vel2;

static uint32_t * pixels;

static uint32_t bg_color = 0xff000000;
static uint32_t fg_color = 0xFFffFFff;

static bool undraw = false;

static void init_pt (Pt * p, Pt * v)
{
  p->x = randint(10, W-10-1);
  p->y = randint(10, H-10-1);
  v->x = randint(1, 6) * (randint(0,1) ? -1 : 1);
  v->y = randint(1, 6) * (randint(0,1) ? -1 : 1);
}

static int cchan[3];
static int ccount[3];
static int cstep[3];

void color_step ()
{
  uint32_t o = 0xff;
  int total = 0;
  for (int i = 0; i < 3; ++i)
  {
    cchan[i] += cstep[i];
    /**/ if (cchan[i] > 255) cchan[i] = 255;
    else if (cchan[i] <  20) cchan[i] =  20;
    ccount[i] -= 1;
    if (ccount[i] < 0)
    {
      cstep[i] = randint(-3,3);
      ccount[i] = randint(5, 148);
    }
    o <<= 8;
    o |= cchan[i] & 0xff;
    total += cchan[i];
  }
  fg_color = o;
  if (total < 85)
  {
    static int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    int prime = primes[randint(0, 9)];
    for (int i = 0; i < 3; ++i)
    {
      int z = (i * prime) % 3;
      if (cstep[z] < 0)
      {
        cstep[z] = randint(2, 6);//*= -1;
        break;
      }
    }
  }
}




static int reflect (int x)
{
  //return -x;

  int s = (x < 0) ? -1 : 1;
  x *= s;
  s *= -1;
  int lo = x;
  int hi = x;
  if (x > 4) lo = x-3;
  if (x < 8) hi = x+3;
  return randint(lo, hi) * s;
}

static Pt move (Pt p, Pt * vel)
{
  Pt nxt = p;
  nxt.x += vel->x;
  if (nxt.x < 0 || nxt.x >= W)
  {
    vel->x = reflect(vel->x);
    nxt.x = p.x;
    nxt.x += vel->x;
  }
  nxt.y += vel->y;
  if (nxt.y < 0 || nxt.y >= H)
  {
    vel->y = reflect(vel->y);
    nxt.y = p.y;
    nxt.y += vel->y;
  }
  return nxt;
}

// An implementation of Bresenham's line drawing algorithm.
// See: https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
static void line (int x0, int y0, int x1, int y1, uint32_t color)
{
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int err = (dx>dy ? dx : -dy)/2, e2;

  while (true)
  {
    pixels[x0 + y0 * stride] = color;
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}

static void drawline (Line l, uint32_t color)
{
  line(l.p1.x, l.p1.y, l.p2.x, l.p2.y, color);
}

static void step ()
{
  Line nxt;
  nxt.p1 = move(lines[curline].p1, &vel1);
  nxt.p2 = move(lines[curline].p2, &vel2);

  curline += 1;
  curline %= nlines;
  lines[curline] = nxt;
  if (curline == 0) undraw = true;

  if (undraw) drawline(lines[(curline + 1) % nlines], bg_color);

  drawline(lines[curline], fg_color);
}

static bool initialized = false;
static void init ()
{
  init_pt(&lines[0].p1, &vel1);
  init_pt(&lines[0].p2, &vel2);
  initialized = true;
}



int * mystery;

int main (int argc, char * argv[])
{
  srand(getpid());

  // Command line arguments:
  // -bX   busy loop X times between frames
  // -sX   sleep X milliseconds between frames
  // -nX   use X lines
  // -dWxH set window to W pixels wide and H pixels tall
  int busy = 0;
  int sleeptime = 17;
  int opt;
  while ((opt = getopt(argc, argv, "d:b:s:n:")) != -1)
  {
    switch (opt)
    {
      case 'b':
        busy = atoi(optarg);
        break;
      case 's':
        sleeptime = atoi(optarg);
        break;
      case 'n':
        nlines = atoi(optarg) - 1;
        if (nlines > MAXLINES) nlines = MAXLINES;
        if (nlines < 2) nlines = 2;
        break;
      case 'd':
        parse_dimensions(optarg, &W, &H);
        break;
      default:
        fprintf(stderr, "Unrecognized argument.\n");
        return 1;
    }
  }

  int fbfd = open("/dev/fb0", O_RDWR);
  if (fbfd < 0)
  {
    fprintf(stderr, "Couldn't open framebuffer.\n");
    return 1;
  }

  struct fb_var_screeninfo si;
  ioctl(fbfd, FBIOGET_VSCREENINFO, &si);
  if (si.xres<W || si.xres<H || si.bits_per_pixel!=32 || si.nonstd)
  {
    fprintf(stderr, "Incompatible video settings.\n");
    return 1;
  }

  struct fb_fix_screeninfo fsi;
  ioctl(fbfd, FBIOGET_FSCREENINFO, &fsi);
  stride = fsi.line_length / 4;

  pixels = mmap(0, 4*stride*si.yres, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);

  for (int i = 0; i < H; ++i)
    for (int j = 0; j < W; ++j)
      pixels[i*stride+j] = 0xFF000000;
  init();

  // Create window
  //init_graphics("Mystify", pixels, W, H, 0);


  while (1)
  {
    color_step();
    step();

    // We may add a "busy" loop which doesn't do anything useful.  It's just
    // to waste time and make it run slower.  You normally wouldn't do it like
    // this and would, e.g., use the usleep() syscall.
    for (int i = 0; i < busy; ++i) if (*mystery) *mystery = 0;
    usleep(1000*sleeptime);
  }

  return 0;
}
