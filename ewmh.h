#ifndef EWMH_H // **** See LICENSE.txt file for copyright and license details. ****
#define EWMH_H
#include <X11/Xlib.h>

typedef struct {
  Display *dpy;
  Window root;

  Atom NET_SUPPORTED;
  Atom NET_WM_STATE;
  Atom NET_WM_STATE_FULLSCREEN;
  Atom NET_ACTIVE_WINDOW;
  Atom NET_WM_NAME;
  Atom NET_NUMBER_OF_DESKTOPS;
  Atom NET_CURRENT_DESKTOP;
  Atom NET_SUPPORTING_WM_CHECK;
  Atom WM_PROTOCOLS;
  Atom WM_DELETE_WINDOW;
} EWMH;

void ewmh_init(EWMH *e, Display *dpy, Window root);
void ewmh_set_number_of_desktops(EWMH *e, int n);
void ewmh_send_wm_delete(EWMH *e, Window w);
int ewmh_window_supports_wm_delete(EWMH *e, Window w);
int ewmh_handle_clientmessage(EWMH *e, XClientMessageEvent *ev, Window focused);

#endif
