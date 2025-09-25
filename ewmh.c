#include "ewmh.h" // See LICENSE file for copyright and license details.
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>

void
ewmh_init(EWMH *e, Display *dpy, Window root) {
  e->dpy = dpy;
  e->root = root;
  e->NET_SUPPORTED = XInternAtom(dpy, "_NET_SUPPORTED", False);
  e->NET_WM_STATE = XInternAtom(dpy, "_NET_WM_STATE", False);
  e->NET_WM_STATE_FULLSCREEN =
      XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
  e->NET_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  e->NET_WM_NAME = XInternAtom(dpy, "_NET_WM_NAME", False);
  e->NET_NUMBER_OF_DESKTOPS =
      XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
  e->NET_CURRENT_DESKTOP = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
  e->NET_SUPPORTING_WM_CHECK =
      XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  e->WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
  e->WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

  Window wmcheck = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
  Atom UTF8_STRING = XInternAtom(dpy, "UTF8_STRING", False);
  const char *wmname = "scwm";
  XChangeProperty(dpy, wmcheck, e->NET_WM_NAME, UTF8_STRING, 8, PropModeReplace,
                  (unsigned char *)wmname, strlen(wmname));
  XChangeProperty(dpy, root, e->NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&wmcheck, 1);
  XChangeProperty(dpy, wmcheck, e->NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&wmcheck, 1);
  Atom supported[] = {e->NET_WM_STATE,           e->NET_WM_STATE_FULLSCREEN,
                      e->NET_ACTIVE_WINDOW,      e->NET_WM_NAME,
                      e->NET_NUMBER_OF_DESKTOPS, e->NET_CURRENT_DESKTOP};
  XChangeProperty(dpy, root, e->NET_SUPPORTED, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)supported, sizeof(supported) / sizeof(Atom));

  ewmh_set_number_of_desktops(e, 1);
  long cur = 0;
  XChangeProperty(dpy, root, e->NET_CURRENT_DESKTOP, XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&cur, 1);
  XFlush(dpy);
}

void
ewmh_set_number_of_desktops(EWMH *e, int n) {
  XChangeProperty(e->dpy, e->root, e->NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&n, 1);
  XFlush(e->dpy);
}

int
ewmh_window_supports_wm_delete(EWMH *e, Window w) {
  Atom *protocols = NULL;
  int n = 0;
  if (XGetWMProtocols(e->dpy, w, &protocols, &n) != 0 && protocols) {
    for (int i = 0; i < n; ++i) {
      if (protocols[i] == e->WM_DELETE_WINDOW) {
        XFree(protocols);
        return 1;
      }
    }
    XFree(protocols);
  }
  return 0;
}

void
ewmh_send_wm_delete(EWMH *e, Window w) {
  XEvent ev;
  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = w;
  ev.xclient.message_type = e->WM_PROTOCOLS;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = e->WM_DELETE_WINDOW;
  ev.xclient.data.l[1] = CurrentTime;
  XSendEvent(e->dpy, w, False, NoEventMask, &ev);
  XFlush(e->dpy);
}

int
ewmh_handle_clientmessage(EWMH *e, XClientMessageEvent *ev, Window focused) {
  if (ev->message_type == e->NET_WM_STATE) {
    Atom action = ev->data.l[0];
    Atom target = (Atom)ev->data.l[1];
    if (target == e->NET_WM_STATE_FULLSCREEN) {
      if (action == 1 || action == 2)   
      return 1;
      else if (action == 0)
	return 1;        
    }      
    else if (ev->message_type == e->NET_ACTIVE_WINDOW) {
      XSetInputFocus(e->dpy, ev->window, RevertToPointerRoot, CurrentTime);
      return 1;      
  } else if (ev->message_type == e->WM_PROTOCOLS) 
      return 0;
    
    return 0;
  }
  return 0;  
}
