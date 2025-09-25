#include "ewmh.h" // **** See LICENSE.txt file for copyright and license details. ********
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//====================== EDIT THIS FOR CHANGING SETTINGS ==========================
#define MODKEY Mod4Mask // Mod1Mask = Alt/Option (Meta); Mod4Mask = Win/Command (Super)
#define RESIZE_BUTTON Button3 // Button1 = Left Mouse Button; Button2 = Middle; Button3 = Right
#define MOVE_BUTTON Button1
#define DEFAULT_NMASTER 1 // Default count of master windows for tile mode
#define DEFAULT_MWFACTOR 60 // Default size of master windows in width of the screen (5-95)
#define DEFAULT_HEIGHT 480 // Default height of window in floating (px)
#define DEFAULT_WIDTH 640  // Default width of window in floating (px)
#define MAX_TAGS 9 // Maximum count of tags
#define MIN_SIZE 2 // Minimal size of window (less then 2 is not recomended!)
// =================================================================================
typedef enum { LAYOUT_TILE, LAYOUT_MONOCLE, LAYOUT_FLOATING } LayoutType;
typedef struct Client { // Client is window
  Window win;
  int x, y, w, h;
  int isFloating;
  int isFullscreen;
  int supportsWMDelete;
  unsigned int tags;
  struct Client *next;
} Client;
typedef struct Monitor { // Monitor is root window (workspace)
  Window root;
  Client *clients;
  Client *sel;
  int x, y, w, h;
  int nmaster;
  unsigned int view;
  LayoutType layout;
  int mwfactor;
} Monitor;
Display *dpy;
Monitor *mon;
Window wmcheckwin;
Atom WM_COMMAND_ATOM;
EWMH ewmh;
static int moving = 0, resizing = 0;
static int start_x, start_y, start_w, start_h;
static int start_cx, start_cy;
static Client *grabbed = NULL;
void
resize(Client *c, int x, int y, int w, int h) { // resize window
  if (!c)
    return;
  if (w < MIN_SIZE)
    w = MIN_SIZE;
  if (h < MIN_SIZE)
    h = MIN_SIZE;
  c->x = x;
  c->y = y;
  c->w = w;
  c->h = h;
  XMoveResizeWindow(dpy, c->win, x, y, w, h);
}
int // function for tags
count_visible(Monitor *m) {
  unsigned int n = 0;
  for (Client *c = m->clients; c; c = c->next)
    if (c->tags & m->view)
      n++;
  return n;
}
Client 
*winToclient(Window w) {  // function for convert
  if (!mon)
    return NULL;
  for (Client *c = mon->clients; c; c = c->next)
    if (c->win == w)
      return c;
  return NULL;
} 
int
is_visible(Client *c, Monitor *m) {  // function for tags
  if (!c || !m)
    return 0;
  return (c->tags & m->view);
}
void 
Focus(Client *c, Monitor *m) {  // Determines which window is currently active
  if (!m)
    return;
  if (!c || !is_visible(c, m)) {
    m->sel = NULL;
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    Window none = None;
    XChangeProperty(dpy, m->root, ewmh.NET_ACTIVE_WINDOW, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&none, 1);
    XFlush(dpy);
    return;
  }
  m->sel = c;
  XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
  XRaiseWindow(dpy, c->win);
  Window w = c->win;
  XChangeProperty(dpy, m->root, ewmh.NET_ACTIVE_WINDOW, XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&w, 1);
  XFlush(dpy);
}

void 
arrange(Monitor *m) {  // Tile
  if(!m) return;
  int n = 0;
  for (Client *c = m->clients; c; c = c->next)
    if ((c->tags & m->view) && !c->isFloating)
      n++;
  if (n == 0)
    return;
   if (m->layout == LAYOUT_MONOCLE) {
     for (Client *c = m->clients; c; c = c->next)
       if ((c->tags & m->view) && !c->isFloating)
         resize(c, m->x, m->y, m->w, m->h);
     return;
   }     
  int nmaster = (m->nmaster < n) ? m->nmaster : n;
  int nstack = n - nmaster;
  int mw = (nmaster > 0) ? (m->w * m->mwfactor / 100) : 0;
  int sx = m->x + mw;
  int mi = 0, si = 0;
  int mh_acc = 0, sh_acc = 0;
  for (Client *c = m->clients; c; c = c->next) {
    if (!(c->tags & m->view)) 
      continue;
    if (c->isFloating) {
      XRaiseWindow(dpy, c->win);
      continue;
    }

 if (mi < nmaster) {
   int mh = (mi == nmaster - 1) ? m->h - mh_acc : m->h / nmaster;
   mw = (nstack == 0) ? mw+(m->w - mw) : mw;
   resize(c, m->x, m->y + mh_acc, mw, mh);
        mi++;
	mh_acc += mh;
      } else {
        int sh = (si == nstack - 1) ? m->h - sh_acc : m->h / nstack;
        resize(c, sx, m->y + sh_acc, m->w - mw, sh);
        sh_acc += sh;
	si++;
     }
  }      
}
void
master_factor(Monitor *m, int delta) {  // changing size of master window
  m->mwfactor += delta;
  if (m->mwfactor < 5)
    m->mwfactor = 5;
  if (m->mwfactor > 95)
    m->mwfactor = 95;
  arrange(m);
}
void
handle_maprequest(XMapRequestEvent *e) { // Render the window
  XWindowAttributes wa;
  if (!XGetWindowAttributes(dpy, e->window, &wa))
    return;
  if (wa.override_redirect) {
    XMapWindow(dpy, e->window);
    return;
  }
  Window transient_for = None;
  XGetTransientForHint(dpy, e->window, &transient_for);
  Client *c = calloc(1, sizeof(Client));
  if (!c) return;
  c->win = e->window;
  c->isFloating = (transient_for != None) || (mon->layout == LAYOUT_FLOATING);
  c->w = DEFAULT_WIDTH;
  c->h = DEFAULT_HEIGHT;
  c->isFullscreen = 0;
  c->tags = mon->view;
  c->next = mon->clients;
  mon->clients = c;
  mon->sel = c;
  XSelectInput(dpy, c->win,
               StructureNotifyMask | PropertyChangeMask | EnterWindowMask);
  c->supportsWMDelete = ewmh_window_supports_wm_delete(&ewmh, c->win) ? 1 : 0;
  if (c->tags & mon->view)
    XMapWindow(dpy, c->win);
  arrange(mon);  
}
void
handle_destroy(XDestroyWindowEvent *e) { // Kill or Delete window
  Client **cc = &mon->clients;
  while (*cc) {
    if ((*cc)->win == e->window) {
      Client *tmp = *cc;
      *cc = tmp->next;
      if (mon->sel == tmp)
        mon->sel = mon->clients;
      free(tmp);
      arrange(mon);
      break;
    }
    cc = &(*cc)->next;
  }
}
void
handle_configure(XConfigureRequestEvent *e) { // Move & Resize
  Client *c = winToclient(e->window);
  if(c) {
  if (c->isFloating) {
    int nw = (e->width < MIN_SIZE) ? MIN_SIZE : e->width;
    int nh = (e->height < MIN_SIZE) ? MIN_SIZE : e->height;
    XMoveResizeWindow(dpy, c->win, e->x, e->y, nw, nh);
    c->x = e->x;
    c->y = e->y;
    c->w = nw;
    c->h = nh;
  }
  if (!c->isFloating) arrange(mon);
  return;
  }
  XWindowChanges wc = {
      .x = e->x,
      .y = e->y,
      .width = (e->width < MIN_SIZE) ? MIN_SIZE : e->width,
      .height = (e->height < MIN_SIZE) ? MIN_SIZE : e->height,
      .sibling = e->above,
      .stack_mode = e->detail,
  };
  unsigned int mask = e->value_mask;  
  if ((mask & CWSibling) && wc.sibling == None) mask &= ~CWSibling;
  if ((mask & CWStackMode) && wc.stack_mode == 0) mask &= ~CWStackMode;
  XConfigureWindow(dpy, e->window, mask, &wc);
}
void
toggle_layout(Monitor *m) { // tile -> monocle -> float
  if (m->layout == LAYOUT_TILE)
    m->layout = LAYOUT_MONOCLE;
  else if (m->layout == LAYOUT_MONOCLE) {
    m->layout = LAYOUT_FLOATING;
    for (Client *c = m->clients; c; c = c->next)
      if (c->tags & m->view)
        c->isFloating = 1;
  } else {
    m->layout = LAYOUT_TILE;
    for (Client *c = m->clients; c; c = c->next)
      if (c->tags & m->view)
        c->isFloating = 0;
  }
  if (m->layout != LAYOUT_FLOATING)
    arrange(m);
  else
    for (Client *c = m->clients; c; c = c->next)
      if (c->tags & m->view)
        XMapWindow(dpy, c->win);
} // for tags
void
update_visibility(Monitor *m) {
  for (Client *c = m->clients; c; c = c->next) {
    if (c->tags & m->view)
      XMapWindow(dpy, c->win);
    else
      XUnmapWindow(dpy, c->win);
  }
}
void
focus_relative(Monitor *m, int dir) {
  if (!m->sel)
    return;
  Client *prev = NULL, *first = NULL, *last = NULL;
  for (Client *c = m->clients; c; c = c->next) {
    if (!(c->tags & m->view))
      continue;
    if (!first)
      first = c;
    last = c;
    if (c == m->sel) {
      if (dir > 0 && c->next)
        Focus(c->next, m);
      else if (dir < 0 && prev)
        Focus(prev, m);
      else
        Focus(dir > 0 ? first : last, m);
      return;
    }
    prev = c;
  }    
}  
void
move_to_tag(Client *c, unsigned int tag, Monitor *m) { // mark win to tag
  if (!c)
    return;
  c->tags = 1 << tag;
  if (c->tags & m->view) {
    XMapWindow(dpy, c->win);
    XRaiseWindow(dpy, c->win);
  } else
    XUnmapWindow(dpy, c->win);
  update_visibility(m);
  arrange(m);
  Focus(c, m);
}
void
handle_net_wm_state(XClientMessageEvent *e) {
  if (ewmh_handle_clientmessage(&ewmh, e, e->window)) {
      Client *c = winToclient(e->window);
      if (c) {
        c->isFullscreen ^= 1;
        if (c->isFullscreen)
          resize(c, mon->x, mon->y, mon->w, mon->h);
        else
          arrange(mon);
      }
    }
}
void
handle_net_active_window(XClientMessageEvent *e) {
  Client *c = winToclient(e->window);
    if (c)
      Focus(c, mon);
}
void
cmd_open_tag(unsigned int tag) {
  mon->view = 1 << tag;
      update_visibility(mon);
      arrange(mon);
}
void
cmd_move_to_tag(unsigned int tag) {
 if (mon->sel) {
        Client *c = winToclient(mon->sel->win);
        move_to_tag(c, tag, mon);
        update_visibility(mon);
        arrange(mon);
      }
}
void
cmd_toggle_floating() {
 if (mon->sel) {
        mon->sel->isFloating ^= 1;
        arrange(mon);
      }
}
void
cmd_kill_window() {
  if (mon->sel) {
        if (mon->sel->supportsWMDelete)
          ewmh_send_wm_delete(&ewmh, mon->sel->win);
        else
          XKillClient(dpy, mon->sel->win);
  }        
}
void
cmd_focus_next() { focus_relative(mon, +1); }
void
cmd_focus_prev() { focus_relative(mon, -1); }
void
cmd_master_count(int delta) {
  mon->nmaster += delta;
      if (mon->nmaster < 1)
        mon->nmaster = 1;
      else if (mon->nmaster > 5)
	mon->nmaster = 5;
      arrange(mon);
}  
void
handle_scwm_command(XClientMessageEvent *e) { // scwmctl
  unsigned int cmd = e->data.l[0];
    unsigned int arg = e->data.l[1];
    switch (cmd) {
    case 1: cmd_open_tag(arg); break;
    case 2: cmd_move_to_tag(arg); break;
    case 3: toggle_layout(mon); break;
    case 4: cmd_toggle_floating(); break;
    case 5: cmd_kill_window();
    case 6: cmd_focus_next(); break;
    case 7: cmd_focus_prev(); break;
    case 8: master_factor(mon, +5); break;
    case 9: master_factor(mon, -5); break;
    case 10: cmd_master_count(+1); break;
    case 11: cmd_master_count(-1); break;
    }
  }  
void
handle_clientmessage(XClientMessageEvent *e) {
  if (e->message_type == ewmh.NET_WM_STATE) handle_net_wm_state(e);
  else if (e->message_type == ewmh.NET_ACTIVE_WINDOW)
    handle_net_active_window(e);
  else if (e->message_type == WM_COMMAND_ATOM)
    handle_scwm_command(e);
}  

Client
*clientat(int x, int y) {
  Window win, child;
  int rx, ry, wx, wy;
  unsigned int mask;
  if (XQueryPointer(dpy, mon->root, &win, &child, &rx, &ry, &wx, &wy, &mask)) {
    if (child != None)
      return winToclient(child);
  }
  return NULL;
}
void
handle_buttonpress(XButtonEvent *e) { // move or resize window via mouse
  Client *c = clientat(e->x_root, e->y_root);
  if (c)
    mon->sel = c;
  if (!(grabbed = mon->sel))
    return;
  XRaiseWindow(dpy, grabbed->win);
  start_x = e->x_root;
  start_y = e->y_root;
  start_w = grabbed->w;
  start_h = grabbed->h;
  start_cx = grabbed->x;
  start_cy = grabbed->y;
  if (e->button == MOVE_BUTTON && (e->state & MODKEY))
    moving = 1;
  if (e->button == RESIZE_BUTTON && (e->state & MODKEY))
    resizing = 1;
}
void
handle_motion(XMotionEvent *e) { // motion of mouse
  if (!grabbed) return;
  if (moving)
    resize(grabbed, start_cx + (e->x_root - start_x),
           start_cy + (e->y_root - start_y), grabbed->w, grabbed->h);
  else if (resizing && grabbed->isFloating) {
    int dw = start_w + (e->x_root - start_x);
    int dh = start_h + (e->y_root - start_y);
    resize(grabbed, grabbed->x, grabbed->y,
           dw < MIN_SIZE ? MIN_SIZE : dw, dh < MIN_SIZE ? MIN_SIZE : dh);
  }
} 
void
handle_buttonrelease(XButtonEvent *e) { // When you done with window
  moving = resizing = 0;
  grabbed = NULL;
  arrange(mon);
}
int
xerror(Display *d, XErrorEvent *ee) { // debuging
  char buf[128];
  XGetErrorText(d, ee->error_code, buf, sizeof buf);
  fprintf(stderr, "X error: %s (req=%d) resource=0x%lx\n", buf,
          ee->request_code, ee->resourceid);
  return 0;
}
void
event_loop(Display *dpy) {
 XEvent e;
  while (1) {
    XNextEvent(dpy, &e);
    switch (e.type) {
    case MapRequest:
      handle_maprequest(&e.xmaprequest);
      break;
    case DestroyNotify:
      handle_destroy(&e.xdestroywindow);
      break;
    case ConfigureRequest:
      handle_configure(&e.xconfigurerequest);
      break;
    case ClientMessage:
      if (e.xclient.message_type == WM_COMMAND_ATOM)
        handle_clientmessage(&e.xclient);
      break;
    case ButtonPress:
      handle_buttonpress(&e.xbutton);
      break;
    case MotionNotify:
      handle_motion(&e.xmotion);
      break;
    case ButtonRelease:
      handle_buttonrelease(&e.xbutton);
      break;
    }
  }
}  
int
main() {
  dpy = XOpenDisplay(NULL);
  XSetErrorHandler(xerror);
  if (!dpy) {
    fprintf(stderr, "Cannot open display!\n");
    return 1;
  }
  mon = malloc(sizeof(Monitor));
  mon->root = DefaultRootWindow(dpy);
  mon->clients = NULL;
  mon->sel = NULL;
  mon->view = 1 << 0;
  mon->layout = LAYOUT_TILE;
  mon->nmaster = DEFAULT_NMASTER;
  mon->mwfactor = DEFAULT_MWFACTOR;
  mon->x = 0;
  mon->y = 0;
  mon->w = DisplayWidth(dpy, 0);
  mon->h = DisplayHeight(dpy, 0);
  ewmh_init(&ewmh, dpy, mon->root);
  WM_COMMAND_ATOM = XInternAtom(dpy, "SCWM_COMMAND", False);                     
  Window w = mon->sel ? mon->sel->win : None;
  XChangeProperty(dpy, mon->root, ewmh.NET_ACTIVE_WINDOW, XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&w, 1);
  
  XSelectInput(dpy, mon->root,
               SubstructureRedirectMask | SubstructureNotifyMask);

  XGrabButton(dpy, MOVE_BUTTON, MODKEY, mon->root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(dpy, RESIZE_BUTTON, MODKEY, mon->root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);
  event_loop(dpy);
  XCloseDisplay(dpy);
  return 0;
}
