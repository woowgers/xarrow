#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iso646.h>
#include <err.h>

#include <sys/types.h>

#include "arrow.h"


#define PROGRAM_NAME "Xarrow"
#define WIDTH_MIN 100
#define HEIGHT_MIN 100
#define FPS 60.f
#define MS_TO_SLEEP (1 / FPS * 1e6)

#define RESOURCE_FILENAME ".Xresources"
#define BACKGROUND_DEFAULT "Antique White"
#define FOREGROUND_DEFAULT "Midnight Blue"


static Atom WM_PROTOCOLS, WM_DELETE_WINDOW;
static bool keep_event_loop = true;
static XWindowAttributes window_attrs;
static XPoint cursor_pos;

static XrmDatabase xdb;
static XColor foreground, background;
static Colormap colormap;
static struct arrow * arrow;
static unsigned long event_mask;

bool draw_needed = true;


XrmOptionDescRec options[] = {
  {"-fg",  "*foreground", XrmoptionSepArg, NULL},
  {"-bg",  "*background", XrmoptionSepArg, NULL},
  {"-transparency", "*transparent", XrmoptionSepArg, NULL}
};


XPoint get_pointer_position(Display * d, Window w)
{
  Window root, child;
  int x, y, x_root, y_root;
  unsigned int mask;

  XQueryPointer(d, w, &root, &child, &x_root, &y_root, &x, &y, &mask);

  return (XPoint){x, y};
}

bool resource_specified(const char * resource_value)
{
  return resource_value != NULL and strncmp("None", resource_value, 5) != 0;
}

bool resource_not_specified(const char * resource_value)
{
  return resource_value == NULL or strncmp("None", resource_value, 5) == 0;
}

bool resource_bool_get_value(const char * resource_value)
{
  enum {
    n_true_values = 4
  };
  const char * true_values[] = {"on", "true", "yes", "1"};
  const size_t values_lengths[] = {3, 5, 4, 2};

  for (size_t i = 0; i < n_true_values; i++)
    if (strncasecmp(true_values[i], resource_value, values_lengths[i]) == 0)
      return true;

  return false;
}

void atoms_create(Display * d)
{
  WM_PROTOCOLS = XInternAtom(d, "WM_PROTOCOLS", false);
  WM_DELETE_WINDOW = XInternAtom(d, "WM_DELETE_WINDOW", false);
}

void resources_set(Display * d, GC gc, GC gc_erase, int * argc, char * argv[])
{
  char xdb_path[PATH_MAX];
  const char *background_str, *foreground_str, *transparent_str;
  int s = DefaultScreen(d);
  uid_t uid;
  struct passwd * passwd;

  uid = getuid();
  if ((passwd = getpwuid(uid)) != NULL)
  {
    snprintf(xdb_path, PATH_MAX, "/home/%s/%s", passwd->pw_name, RESOURCE_FILENAME);
    xdb = XrmGetFileDatabase(xdb_path);
  }
  if (xdb == NULL)
    xdb = XrmGetStringDatabase("");
  XrmParseCommand(&xdb, options, sizeof(options) / sizeof(options[0]), PROGRAM_NAME, argc, argv);
  XrmSetDatabase(d, xdb);

  background_str = XGetDefault(d, PROGRAM_NAME, "background");
  foreground_str = XGetDefault(d, PROGRAM_NAME, "foreground");
  transparent_str = XGetDefault(d, PROGRAM_NAME, "transparent");

#if defined(DEBUG)
  fprintf(stderr, "Got following resources:\n\tbackground:\t%s\n\tforeground:\t%s\n\ttransparent:\t%s\n\n",
          background_str, foreground_str, transparent_str);
#endif

  if (resource_specified(transparent_str) and resource_bool_get_value(transparent_str) == true)
    XSetForeground(d, gc_erase, 0x0);
  else
  {
    if (resource_not_specified(background_str))
      background_str = BACKGROUND_DEFAULT;
    XParseColor(d, colormap, background_str, &background);
    XAllocColor(d, colormap, &background);
    XSetForeground(d, gc_erase, background.pixel);
  }

  if (resource_not_specified(foreground_str))
    foreground_str = FOREGROUND_DEFAULT;

  XParseColor(d, colormap, foreground_str, &foreground);
  XAllocColor(d, colormap, &foreground);
  XSetForeground(d, gc, foreground.pixel);
}


Display * display_open(int * argc, char * argv[])
{
  Display * d;

  d = XOpenDisplay(NULL);
  if (d != NULL)
    atoms_create(d);

  return d;
}

GC gc_create_copy(Display * d, GC default_gc, Drawable drawable)
{
  const unsigned long mask = (1L << 23) - 1;
  GC gc;

  gc = XCreateGC(d, drawable, 0, NULL);
  XCopyGC(d, default_gc, mask, gc);

  return gc;
}

Window window_create(Display * d, Window root, int x, int y, unsigned width, unsigned height)
{
  Window w;
  int s = DefaultScreen(d);
  unsigned long attrs_mask = CWEventMask | CWColormap | CWBorderPixel;
  XVisualInfo vinfo;
  XSetWindowAttributes attrs;
  XSizeHints hints = {
    .flags = PMinSize | PMaxSize | PAspect,
    .min_width = WIDTH_MIN,
    .min_height = HEIGHT_MIN,
    .max_width = DisplayWidth(d, s),
    .max_height = DisplayHeight(d, s),
    .min_aspect = {1.0f, 1.0f},
    .max_aspect = {1.0f, 1.0f}
  };

  XMatchVisualInfo(d, s, 32, TrueColor, &vinfo);
  event_mask = attrs.event_mask =
    ExposureMask | PointerMotionMask | KeyPressMask | StructureNotifyMask | LeaveWindowMask | EnterWindowMask;
  colormap = attrs.colormap = XCreateColormap(d, root, vinfo.visual, AllocNone);
  attrs.border_pixel = BlackPixel(d, s);
  XSelectInput(d, root, PointerMotionMask);

  w = XCreateWindow(d, root, x, y, width, height, 0, vinfo.depth, InputOutput, vinfo.visual, attrs_mask, &attrs);
  XSetWMNormalHints(d, w, &hints);
  XStoreName(d, w, PROGRAM_NAME);
  XMapWindow(d, w);
  XSetWMProtocols(d, w, &WM_DELETE_WINDOW, 1);

  return w;
}

Pixmap window_background_pixmap_create(Display * d, Window w)
{
  int s = DefaultScreen(d), depth = 32;
  unsigned width = DisplayWidth(d, s), height = DisplayHeight(d, s);
  Pixmap p;

  p = XCreatePixmap(d, w, width, height, depth);

  return p;
}


void finish_event_loop()
{
  keep_event_loop = false;
}

void empty_event_handler(XEvent * e)
{
}

void on_expose(XEvent * e)
{
  XGetWindowAttributes(e->xexpose.display, e->xexpose.window, &window_attrs);
  arrow_set_center(arrow, e->xexpose.width / 2, e->xexpose.height / 2);
  arrow_set_length(arrow, e->xexpose.width / 2 / M_SQRT2);
  draw_needed = true;
}

void on_configure_notify(XEvent * e)
{
  XGetWindowAttributes(e->xexpose.display, e->xexpose.window, &window_attrs);
  arrow_set_center(arrow, e->xconfigure.width / 2, e->xconfigure.height / 2);
  arrow_set_length(arrow, e->xconfigure.width / 2 / M_SQRT2);
  cursor_pos = get_pointer_position(e->xexpose.display, e->xexpose.window);
  arrow_set_target(arrow, cursor_pos.x, cursor_pos.y);
  draw_needed = true;
}

void on_client_message(XEvent * e)
{
  Atom message_type = e->xclient.message_type;
  long message = e->xclient.data.l[0];

  if (message_type == WM_PROTOCOLS and message == WM_DELETE_WINDOW)
    finish_event_loop();
}

void on_key_press(XEvent * e)
{
  KeySym keysym = XkbKeycodeToKeysym(e->xkey.display, e->xkey.keycode, 0, 0);

  if (keysym == XK_Escape or keysym == XK_q)
    finish_event_loop();
}

void on_motion_notify(XEvent * e)
{
  cursor_pos = (XPoint){e->xmotion.x, e->xmotion.y};
  arrow_set_target(arrow, e->xmotion.x, e->xmotion.y);
  draw_needed = true;
}

void on_root_motion_notify(XEvent * e)
{
  XPoint target;
  cursor_pos = (XPoint){e->xmotion.x, e->xmotion.y};
  target = (XPoint){
    cursor_pos.x - window_attrs.x,
    cursor_pos.y - window_attrs.y
  };
  arrow_set_target(arrow, target.x, target.y);
  draw_needed = true;
}

void draw(Display * d, Window w, Pixmap p, GC gc, GC gc_erase)
{
  XFillRectangle(d, p, gc_erase, 0, 0, window_attrs.width, window_attrs.height);
  arrow_draw(arrow, d, p, gc);
  XSetWindowBackgroundPixmap(d, w, p);
  XClearWindow(d, w);
  draw_needed = false;
}

void event_loop(Display * d, Window root, Window w, Pixmap p, GC gc, GC gc_erase)
{
  XEvent e;
#if defined(__GNUC__)
  void (*event_handlers[LASTEvent])(XEvent *) = {[0 ... LASTEvent - 1] = empty_event_handler};
#else
  void (*event_handlers[LASTEvent])(XEvent *);
  for (size_t i = 0; i < LASTEvent; i++)
    event_handlers[i] = empty_event_handler;
#endif

  event_handlers[Expose] = on_expose;
  event_handlers[MotionNotify] = on_motion_notify;
  event_handlers[ClientMessage] = on_client_message;
  event_handlers[KeyPress] = on_key_press;
  event_handlers[ConfigureNotify] = on_configure_notify;

  do
  {
    if (XCheckWindowEvent(d, w, event_mask, &e) or XCheckTypedWindowEvent(d, w, ClientMessage, &e))
      event_handlers[e.type](&e);
    else if (XCheckTypedWindowEvent(d, root, MotionNotify, &e))
      on_root_motion_notify(&e);
    else
    {
      usleep(MS_TO_SLEEP);
      XPoint target = get_pointer_position(d, w);
      arrow_set_target(arrow, target.x, target.y);
      draw_needed = true;
    }

    if (draw_needed)
      draw(d, w, p, gc, gc_erase);
  }
  while (keep_event_loop);
}


void process_args(int argc, char * argv[])
{
  if (argc > 1)
    fprintf(stderr, "Usage: xarrow [-fg <color>] [-bg <color>] [-transparency <boolean>]\n");
}


int main(int argc, char * argv[])
{
  Display * d;
  Window root, w;
  Pixmap p;
  int s;
  GC gc, gc_erase;

  d = display_open(&argc, argv);
  if (d == NULL)
    errx(EXIT_FAILURE, "Failed to open X display");

  s = DefaultScreen(d);
  root = DefaultRootWindow(d);
  w = window_create(d, root, 0, 0, 600, 600);
  p = window_background_pixmap_create(d, w);
  gc = XCreateGC(d, w, 0, 0);
  gc_erase = XCreateGC(d, w, 0, 0);
  resources_set(d, gc, gc_erase, &argc, argv);
  XGetWindowAttributes(d, w, &window_attrs);

  process_args(argc, argv);

  get_pointer_position(d, w);
  arrow = arrow_create(window_attrs.width / 2 / M_SQRT2, window_attrs.width / 2, window_attrs.height / 2);
  arrow_set_length(arrow, window_attrs.width / 2 / M_SQRT2);
  arrow_set_target(arrow, cursor_pos.x, cursor_pos.y);

  event_loop(d, root, w, p, gc, gc_erase);

  arrow_free(arrow);
  XFreePixmap(d, p);
  XDestroyWindow(d, w);
  XCloseDisplay(d);

  return 0;
}
