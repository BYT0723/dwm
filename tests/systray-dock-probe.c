/* Systray dock probe: send _NET_SYSTEM_TRAY_REQUEST_DOCK for the hostile and
 * graceful cases tests/systray-guards-check.sh needs. Kept in C because a tray
 * client is the only way to reproduce these paths (xdotool/prop cannot send a
 * ClientMessage to the tray owner).
 *
 * usage: systray-dock-probe <display> <case>
 *   hold      dock a plain 24x24 icon and stay alive (test inspects it)
 *   managed   map the window first (dwm manages it), then dock, stay alive
 *   tray      dock the tray selection owner itself
 *   root      dock the root window
 *   zero      dock a 0x0 window
 *   normal    dock a plain icon and exit
 */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Display *dpy;

static void dock(Window tray, Window w) {
  Atom opcode = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  XEvent ev;

  memset(&ev, 0, sizeof ev);
  ev.xclient.type = ClientMessage;
  ev.xclient.window = tray;
  ev.xclient.message_type = opcode;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime;
  ev.xclient.data.l[1] = 0; /* SYSTEM_TRAY_REQUEST_DOCK */
  ev.xclient.data.l[2] = (long)w;
  XSendEvent(dpy, tray, False, NoEventMask, &ev);
  XSync(dpy, False);
}

static Window mkwin(int w, int h, int map) {
  Window win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, w, h, 0,
                             CopyFromParent, InputOutput, CopyFromParent, 0,
                             NULL);
  XClassHint ch = {"SysTray-X", "SysTray-X"};

  XSetClassHint(dpy, win, &ch);
  if (map)
    XMapWindow(dpy, win);
  XSync(dpy, False);
  return win;
}

int main(int argc, char **argv) {
  Window tray, win;

  if (argc < 3)
    return 2;
  if (!(dpy = XOpenDisplay(argv[1])))
    return 3;
  tray = XGetSelectionOwner(dpy, XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False));
  if (!tray) {
    fprintf(stderr, "no tray owner\n");
    XCloseDisplay(dpy);
    return 4;
  }

  if (!strcmp(argv[2], "hold")) {
    win = mkwin(24, 24, 0);
    dock(tray, win);
    sleep(8);
  } else if (!strcmp(argv[2], "managed")) {
    win = mkwin(24, 24, 1);
    sleep(1); /* let dwm manage it as a normal client */
    dock(tray, win);
    sleep(6);
  } else if (!strcmp(argv[2], "tray")) {
    dock(tray, tray);
    sleep(1);
  } else if (!strcmp(argv[2], "root")) {
    dock(tray, DefaultRootWindow(dpy));
    sleep(1);
  } else if (!strcmp(argv[2], "zero")) {
    win = mkwin(0, 0, 0);
    dock(tray, win);
    sleep(1);
  } else if (!strcmp(argv[2], "normal")) {
    win = mkwin(24, 24, 0);
    dock(tray, win);
    sleep(1);
  } else {
    fprintf(stderr, "unknown case\n");
    XCloseDisplay(dpy);
    return 2;
  }

  XSync(dpy, False);
  XCloseDisplay(dpy);
  return 0;
}
