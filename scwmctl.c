#include <X11/Xatom.h> // See LICENSE file for copyright and license details.
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc < 2)
    return 1;
  Display *dpy = XOpenDisplay(NULL);
  Window root = DefaultRootWindow(dpy);
  Atom atom = XInternAtom(dpy, "SCWM_COMMAND", False);

  XEvent ev;
  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = root;
  ev.xclient.message_type = atom;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = atoi(argv[1]);
  ev.xclient.data.l[1] = (argc > 2) ? atoi(argv[2]) : 0;
  XSendEvent(dpy, root, False,
             SubstructureRedirectMask | SubstructureNotifyMask, &ev);
  XFlush(dpy);
  XCloseDisplay(dpy);
  return 0;
}
