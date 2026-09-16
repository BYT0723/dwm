/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>
#include <poll.h>
#include <errno.h>
#include <time.h>

#include <Imlib2.h>

#include "barparse.h"
#include "drw.h"
#include "util.h"

/* macros */
#define BUTTONMASK (ButtonPressMask | ButtonReleaseMask)
#define CLEANMASK(mask)                                                        \
  (mask & ~(numlockmask | LockMask) &                                          \
   (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask |      \
    Mod5Mask))
#define INTERSECT(x, y, w, h, m)                                               \
  (MAX(0, MIN((x) + (w), (m)->wx + (m)->ww) - MAX((x), (m)->wx)) *             \
   MAX(0, MIN((y) + (h), (m)->wy + (m)->wh) - MAX((y), (m)->wy)))
#define ISVISIBLE(C) ((C->tags & C->mon->tagset[C->mon->seltags]))
#define HIDDEN(C) ((C)->hidden)
#define ISHIDDENSTATE(S) ((S) == IconicState || (S) == WithdrawnState)
#define LENGTH(X) (sizeof X / sizeof X[0])
#define MOUSEMASK (BUTTONMASK | PointerMotionMask)
#define WIDTH(X) ((X)->w + 2 * (X)->bw)
#define HEIGHT(X) ((X)->h + 2 * (X)->bw)
#define TAGMASK ((1 << LENGTH(tags)) - 1)
#define TEXTW(X) (drw_fontset_getwidth(drw, (X)) + lrpad)
#define INMON(m, x, y) \
    ((m) && \
     (x) >= (m)->mx && (x) < (m)->mx + (m)->mw && \
     (y) >= (m)->my && (y) < (m)->my + (m)->mh)

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY 0
#define XEMBED_WINDOW_ACTIVATE 1
#define XEMBED_WINDOW_DEACTIVATE 2
#define XEMBED_FOCUS_IN 4
#define XEMBED_MODALITY_ON 10

#define XEMBED_MAPPED (1 << 0)

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum {
  SchemeNorm,
  SchemeSel,
  SchemeHid,
  SchemeTagNorm,
  SchemeTagSel,
  SchemeStatus,
  SchemeSystray,
  SchemeLayout,
  SchemeEmpty,
  SchemeTooltip,
}; /* color schemes */
enum {
  NetSupported,
  NetWMName,
  NetWMIcon,
  NetWMState,
  NetWMCheck,
  NetSystemTray,
  NetSystemTrayOP,
  NetSystemTrayOrientation,
  NetSystemTrayOrientationHorz,
  NetSystemTrayVisual,
  NetWMFullscreen,
  NetWMMaximizedVert,
  NetWMMaximizedHorz,
  NetActiveWindow,
  NetWMWindowType,
  NetWMWindowTypeDock,
  NetWMWindowTypeDialog,
  NetWMWindowTypeTooltip,
  NetClientList,
  NetWMWindowOpacity,
  NetNumberOfDesktops,
  NetCurrentDesktop,
  NetDesktopNames,
  NetWMDesktop,
  NetLast
};
/* EWMH atoms */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
enum {
  WMProtocols,
  WMDelete,
  WMState,
  WMChangeState,
  WMTakeFocus,
  WMLast
}; /* default atoms */
enum {
  ClkTagBar,
  ClkLtSymbol,
  ClkStatusText,
  ClkWinTitle,
  ClkClientWin,
  ClkTitleBar,
  ClkRootWin,
  ClkLast
}; /* clicks */

typedef union {
  int i;
  unsigned int ui;
  float f;
  const void *v;
} Arg;

typedef struct {
  unsigned int click;
  unsigned int mask;
  unsigned int button;
  void (*func)(const Arg *arg);
  const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
  char name[256];
  char class[256];
  float mina, maxa;
  float cfact;
  int x, y, w, h;
  int oldx, oldy, oldw, oldh;
  int sfx, sfy, sfw, sfh; /* last floating geometry, restored when leaving tiled state */
  int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
  int bw, oldbw, basebw; /* basebw = rule-specified border, restored by arrangemon */
  unsigned int tags;
  int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
  int hidden; /* 1 = IconicState (hide) or WithdrawnState (tray hide), mirrors WM_STATE */
  unsigned int icw, ich, icon_alpha;
  Picture icon;
  /* the same icon at the tab's own size, when the bar reserves room under it
     for the selection dot; None when the two sizes agree. tabicon_size is the
     target size that picture was built for (0 = none), so a mode change can
     tell it went stale */
  unsigned int tabicw, tabich;
  Picture tabicon;
  int tabicon_size;
  Pixmap hspm;   /* hidden snapshot pixmap, None when absent */
  Picture hspic; /* hidden snapshot picture, None when absent */
  unsigned int hspw, hsph; /* hidden snapshot size, fits the hidden box */
  Client *next;
  Client *snext;
  Monitor *mon;
  Window win;   /* application window, child of frame */
  Window frame; /* reparenting frame carrying border + titlebar, None until managed */
  int ignoreunmap; /* expected UnmapNotifies to swallow (reparent/hide) */
  int rule_notitle; /* Rule override: 1 = never show a titlebar for this client */
  int notitle;  /* resolved: rule override OR motif/gtk self-decoration hints */
};

typedef struct {
  unsigned int mod;
  KeySym keysym;
  void (*func)(const Arg *);
  const Arg arg;
} Key;

typedef struct {
  const char *symbol;
  void (*arrange)(Monitor *);
} Layout;

/* bar layout: items are grouped into three zones (config.h) and each zone
   is filled in array order. BarStatus carries the block ids to draw as
   pills; other modules ignore ids. A BarLayout right after BarTags shares
   the tags pill, which is why the two are one pill by default. */
typedef enum { BarNone, BarTags, BarLayout, BarTabs, BarStatus } BarModule;
typedef struct {
  BarModule mod;
  const int *ids;
} BarItem;

/* one drawn bar element, recorded as the bar is painted so clicks and hover
   resolve against exactly what is on screen instead of re-deriving it */
typedef struct {
  int x, w;
  unsigned int click;
  Arg arg;
} BarSlot;
#define BAR_SLOTS 32

/* one drawn tab: where it sits, how wide, and which client it belongs to */
typedef struct {
  int x, w;
  Client *c;
} TabCell;
#define TAB_CELLS 64

typedef struct Pertag Pertag;
struct Monitor {
  char ltsymbol[16];
  float mfact;
  int nmaster;
  int num;
  int by;             /* bar geometry */
  BarSlot slots[BAR_SLOTS]; /* drawn bar elements, for clicks and hover */
  int nslots;
  int mx, my, mw, mh; /* screen size */
  int wx, wy, ww, wh; /* window area  */
  int gappih;         /* horizontal gap between windows */
  int gappiv;         /* vertical gap between windows */
  int gappoh;         /* horizontal outer gaps */
  int gappov;         /* vertical outer gaps */
  unsigned int seltags;
  unsigned int sellt;
  unsigned int tagset[2];
  int showbar;
  int topbar;
  int hidsel;
  int last_mouse_pos[2];
  Client *clients;
  Client *sel;
  Client *stack;
  Monitor *next;
  Window barwin;
  int previewshow;    /* 1-based tag index being previewed, 0 = none */
  Window tagwin;      /* scaled tag preview window, below the bar */
  Pixmap *tagmap;     /* scaled snapshot per tag */
  const Layout *lt[2];
  Pertag *pertag;
};

typedef struct {
  const char *class;
  const char *instance;
  const char *title;
  unsigned int tags;
  int isfloating;
  int monitor;
  int bw; /* < 0 = default borderpx, >= 0 overrides c->bw/basebw */
  int notitle; /* 1 = never show a titlebar for this client, 0 = auto-detect */
} Rule;

typedef struct Systray Systray;
struct Systray {
  Window win;
  Client *icons;
};

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h,
                          int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachbottom(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void cyclecfact(const Arg *arg);
static void cyclemfact(const Arg *arg);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawtabborder(int x, int w, Clr *s);
static int drawstatuspills(Monitor *m, int x, const int *ids);
static int tablayout(Monitor *m, int avail, TabCell *cells, int max, int *ncells);
static int drawbarpill(Monitor *m, const BarItem *items, size_t nitems, size_t k,
                       int x, int occ, int urg);
static void tabicon_size(Client *c, int *w, int *h);
static int tabicon_get(Client *c, int scm, Picture *pic, unsigned int *w,
                       unsigned int *h);
static void tabseldot_paint(int scm, int cx, int cy, int r);
static void drawdisc(int x, int y, int d);
static uint32_t prealpha(uint32_t p, uint32_t custom_alpha);
static void tabdraw(Monitor *m, int x, const TabCell *cells, int ncells);
static int titleh(Client *c);
static void updatenodecor(Client *c);
static int tabiconsize(void);
static void createframe(Client *c);
static void placeclient(Client *c);
static void drawtitle(Client *c);
static void drawtitles(void);
static int titlebtnsat(Client *c, int x);
static Client *frameclient(Window w);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static Client *focusclient(Monitor *m);
static int focusmode(Monitor *m);
static void focusmon(const Arg *arg);
static void focusstack(int inc, int vis);
static void focusstackhid(const Arg *arg);
static void focusstackvis(const Arg *arg);
static void freehspic(Client *c);
static void freeicon(Client *c);
static void fullscreen(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static Picture geticonprop(Window w, unsigned int *icw, unsigned int *ich,
                           unsigned int alpha, int target);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static long getstateprop(Window w, const char *name, long *out, long maxvals);
static unsigned int getsystraywidth();
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void hide(const Arg *arg);
static void hideclient(Client *c);
static void hidewin(Client *c);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void leavenotify(XEvent *e);
static void killclient(const Arg *arg);
static void loadxrdb(void);
static void layoutmenu(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa, int mapped);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void resizerequest(XEvent *e);
static void restack(Monitor *m);
static void restoreclientorder(void);
static void restorefocus(void);
static void restorestacking(void);
static void run(void);
static void runautostart(void);
static void scan(void);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setcurrentmon(Monitor *m);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setstateprop(Window w, Atom a, unsigned long *vals, int nvals);
static void setup(void);
static void seturgent(Client *c, int urg);
static void show(const Arg *arg);
static void showall(const Arg *arg);
static void showhide(Client *c);
static void showwin(Client *c);
static void sigchld(int unused);
static void sighup(int unused);
static void sigterm(int unused);
static void spawn(const Arg *arg);
static int statuswidth(const int *ids);
static void statusparse(const char *text);
static int status_block_width(int i);
static void statuspills_build(const int *ids);
static int barzonewidth(Monitor *m, const BarItem *items, size_t nitems, int occ,
                        int n, int avail);
static void addslot(Monitor *m, int x, int w, unsigned int click, Arg arg);
static const BarSlot *barslotat(Monitor *m, int x);
static int tagindex(unsigned int mask);
static void drawpillcap(Clr *work, int capx, int pillw);
static int status2d_runwidth(char *s);
static void systraydock(Window w);
static int systrayredock(Window w);
static Monitor *systraytomon(Monitor *m);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglefloatingclient(Client *c);
static void focusmaster(const Arg *arg);
static void maximize(const Arg *arg);
static void hoverfire(void);
static void hoverhide(void);
static void hovershow(Client *c, int tx);
static void previewtag(const Arg *arg);
static void showtagpreview(unsigned int i);
static void takesnapshot(Client *c);
static void takepreview(void);
static int tagtextw(unsigned int i);
typedef const char *(*template_resolve)(const char *f, size_t *plen, void *ctx);
static void template_expand(const char *fmt, template_resolve resolve, void *ctx, char *buf, size_t len);
static const char *tab_placeholder(const char *f, size_t *plen, void *ctx);
static const char *tag_placeholder(const char *f, size_t *plen, void *ctx);
static void toggletabmode(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void togglewin(const Arg *arg);
static int trayrank(const char *class);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int clientdesktop(Client *c);
static void updatecurrentdesktop(void);
static void updatedesktopnames(void);
static void updatenumberofdesktops(void);
static void updatewmdesktop(Client *c);
static int updategeom(void);
static void updateicon(Client *c);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatesystray(int flag);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void xinitvisual();
static void zoom(const Arg *arg);

/* variables */
static const char autostartblocksh[] = "autostart_blocking.sh";
static const char autostartsh[] = "autostart.sh";
static const char broken[] = "broken";
static const char dwmdir[] = "dwm";
static const char localshare[] = ".local/share";
static char stext[1024];
#define MAX_STBLOCKS 32
static char stbuf[1024];                  /* stext with control chars filtered out */
static BarBlock stblocks[MAX_STBLOCKS];   /* visible blocks of stbuf, in draw order */
static int nstblocks;
static char pbuf[2048];                   /* pills built from stbuf, ^( .. ^) wrapped */
/* one drawn status block: where it sits and how wide, so a click maps back
   to the control-character id the writer tagged it with */
typedef struct {
  unsigned int id;
  int x, w;
} StatusCell;
static StatusCell scells[MAX_STBLOCKS];
static int nscells;
/* one drawn pill's body width, so it can be painted with the pill's own ^b
   colour before its text runs */
typedef struct {
  int w;
} StatusPill;
static StatusPill spills[MAX_STBLOCKS];
static int nspills;
static int spillidx; /* draw-time cursor into spills */
static int stpills_w;                     /* total drawn width of the pills */
static int hoverw;                        /* width of the hovered tab slot */
static int statuscmdn;
static char lastbutton[] = "-";
static int screen;
static int sw, sh;      /* X display screen geometry width, height */
static int bh; /* bar geometry */
static int th = 0;        /* titlebar height (font height + 2 * titlebarpad when enabled, 0 when disabled) */
static int lrpad;       /* sum of left and right padding for text */
static int lpad;        /* left padding for text, equal lrpad/2  */
static int vp;          /* vertical padding for bar */
static int sp;          /* side padding for bar */
static int tabr;        /* bar tab corner radius */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent])(XEvent *) = {
    [ButtonPress] = buttonpress,
    [ClientMessage] = clientmessage,
    [ConfigureRequest] = configurerequest,
    [ConfigureNotify] = configurenotify,
    [DestroyNotify] = destroynotify,
    [EnterNotify] = enternotify,
    [Expose] = expose,
    [FocusIn] = focusin,
    [KeyPress] = keypress,
    [LeaveNotify] = leavenotify,
    [MappingNotify] = mappingnotify,
    [MapRequest] = maprequest,
    [MotionNotify] = motionnotify,
    [PropertyNotify] = propertynotify,
    [ResizeRequest] = resizerequest,
    [UnmapNotify] = unmapnotify};
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
static Atom motifwmhints = None;   /* _MOTIF_WM_HINTS: decorations == 0 → self-decorated */
static Atom gtkframeextents = None; /* _GTK_FRAME_EXTENTS present → GTK CSD window */
static int restart = 0;
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Fnt *fonts_set;
static Fnt *fonts_highlight_set;
static Window toolwin = None; /* hover tooltip window */
static Drw *tooldrw = NULL;   /* drawing context for the tooltip */
static Client *hoverc = NULL; /* client the tooltip is shown for */
static int hoverarm = 0;      /* 1 = pointer resting on a tab, timer running */
static int hoverx = 0;        /* tab left edge at arm time */
static int hoveridx = -1;     /* tag index armed for preview, -1 = none */
static uint64_t hoverstart = 0; /* monotonic ms when the timer was armed */
static int prevpx = 0, prevpy = 0, prevw = 0, prevh = 0; /* preview area geometry */
static uint64_t hovernow(void);
static void hoverpreview(Client *c, int x, int y, int w, int h);
static void hoverrefresh(void);
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

static Systray *systray = NULL;

static int useargb = 0;
static Visual *visual;
static int depth;
static Colormap cmap;

/* configuration, allows nested code to access above variables */
#include "config.h"

struct Pertag {
  unsigned int curtag, prevtag;          /* current and previous tag */
  int nmasters[LENGTH(tags) + 1];        /* number of windows in master area */
  float mfacts[LENGTH(tags) + 1];        /* mfacts per tag */
  unsigned int sellts[LENGTH(tags) + 1]; /* selected layouts */
  const Layout *ltidxs[LENGTH(tags) + 1][2];    /* matrix of tags and layouts indexes  */
  int showbars[LENGTH(tags) + 1];      /* display bar for the current tag */
  int focusmaster[LENGTH(tags) + 1];   /* 1 = focusmaster (host centeredfloatingmaster, focused window centered), 2 = maximize (host monocle, all tiled stacked with gaps) */
  const Layout *fmlast[LENGTH(tags) + 1];  /* layout active before the first single-master mode; restored on leaving */
};

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags {
  char limitexceeded[LENGTH(tags) > 31 ? -1 : 1];
};

/* function implementations */
void applyrules(Client *c) {
  const char *class, *instance;
  unsigned int i;
  const Rule *r;
  Monitor *m;
  XClassHint ch = {NULL, NULL};

  /* rule matching */
  c->isfloating = 0;
  c->tags = 0;
  c->rule_notitle = 0;
  c->bw = c->basebw = borderpx;
  XGetClassHint(dpy, c->win, &ch);
  class = ch.res_class ? ch.res_class : broken;
  instance = ch.res_name ? ch.res_name : broken;

  strncpy(c->class, class, sizeof(c->class) - 1);
  c->class[sizeof(c->class) - 1] = '\0';

  for (i = 0; i < LENGTH(rules); i++) {
    r = &rules[i];
    if ((!r->title || strstr(c->name, r->title)) &&
        (!r->class || strstr(class, r->class)) &&
        (!r->instance || strstr(instance, r->instance))) {
      c->isfloating = r->isfloating;
      c->tags |= r->tags;
      c->rule_notitle = r->notitle;
      if (r->bw >= 0)
        c->bw = c->basebw = r->bw;
      for (m = mons; m && m->num != r->monitor; m = m->next);
      if (m)
        c->mon = m;
    }
  }
  if (ch.res_class)
    XFree(ch.res_class);
  if (ch.res_name)
    XFree(ch.res_name);
  c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact) {
  int baseismin;
  Monitor *m = c->mon;

  /* set minimum possible */
  *w = MAX(1, *w);
  *h = MAX(1, *h);
  if (interact) {
    if (*x > sw)
      *x = sw - WIDTH(c);
    if (*y > sh)
      *y = sh - HEIGHT(c);
    if (*x + *w + 2 * c->bw < 0)
      *x = 0;
    if (*y + *h + 2 * c->bw < 0)
      *y = 0;
  } else {
    if (*x >= m->wx + m->ww)
      *x = m->wx + m->ww - WIDTH(c);
    if (*y >= m->wy + m->wh)
      *y = m->wy + m->wh - HEIGHT(c);
    if (*x + *w + 2 * c->bw <= m->wx)
      *x = m->wx;
    if (*y + *h + 2 * c->bw <= m->wy)
      *y = m->wy;
  }
  if (*h < bh)
    *h = bh;
  if (*w < bh)
    *w = bh;
  if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
    if (!c->hintsvalid)
      updatesizehints(c);
    /* see last two sentences in ICCCM 4.1.2.3 */
    baseismin = c->basew == c->minw && c->baseh == c->minh;
    if (!baseismin) { /* temporarily remove base dimensions */
      *w -= c->basew;
      *h -= c->baseh;
    }
    /* adjust for aspect limits */
    if (c->mina > 0 && c->maxa > 0) {
      if (c->maxa < (float)*w / *h)
        *w = *h * c->maxa + 0.5;
      else if (c->mina < (float)*h / *w)
        *h = *w * c->mina + 0.5;
    }
    if (baseismin) { /* increment calculation requires this */
      *w -= c->basew;
      *h -= c->baseh;
    }
    /* adjust for increment value */
    if (c->incw)
      *w -= *w % c->incw;
    if (c->inch)
      *h -= *h % c->inch;
    /* restore base dimensions */
    *w = MAX(*w + c->basew, c->minw);
    *h = MAX(*h + c->baseh, c->minh);
    if (c->maxw)
      *w = MIN(*w, c->maxw);
    if (c->maxh)
      *h = MIN(*h, c->maxh);
  }
  return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void arrange(Monitor *m) {
  if (m)
    showhide(m->stack);
  else
    for (m = mons; m; m = m->next)
      showhide(m->stack);
  if (m) {
    arrangemon(m);
    restack(m);
  } else
    for (m = mons; m; m = m->next)
      arrangemon(m);
}

void arrangemon(Monitor *m) {
  Client *c;
  /* only fullscreen() (monocle + bar hidden) is borderless; plain [M]
     and the maximize host (focusmaster == 2, tiled with gaps) keep borders */
  int borderless =
      m->lt[m->sellt]->arrange == monocle &&
      m->pertag->focusmaster[m->pertag->curtag] != 2 && !m->showbar;
  for (c = m->clients; c; c = c->next) {
    int target = c->basebw;
    if (c->isfullscreen)
      continue;
    if (borderless && !c->isfloating && ISVISIBLE(c) && !HIDDEN(c))
      target = 0;
    if (c->bw != target) {
      c->bw = target;
      if (c->frame != None)
        XSetWindowBorderWidth(dpy, c->frame, c->bw);
    }
  }
  strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
  if (m->lt[m->sellt]->arrange)
    m->lt[m->sellt]->arrange(m);
}

void attach(Client *c) {
  c->next = c->mon->clients;
  c->mon->clients = c;
}

void attachbottom(Client *c) {
  Client **tc;
  c->next = NULL;
  for (tc = &c->mon->clients; *tc; tc = &(*tc)->next);
  *tc = c;
}

void attachstack(Client *c) {
  c->snext = c->mon->stack;
  c->mon->stack = c;
}

void buttonpress(XEvent *e) {
  unsigned int click;
  int i;
  const BarSlot *s;
  Arg arg = {0};
  Client *c;
  Monitor *m;
  XButtonPressedEvent *ev = &e->xbutton;

  click = ClkRootWin;
  /* focus monitor if necessary */
  if ((m = wintomon(ev->window)) && m != selmon) {
    unfocus(selmon->sel, 1);
    selmon = m;
    focus(NULL);
  }

  if (ev->window == selmon->barwin) {
    hoverhide();
    if ((s = barslotat(selmon, ev->x))) {
      click = s->click;
      arg = s->arg;
      if (click == ClkStatusText) {
        /* hand the block's control character to the click command */
        *lastbutton = '0' + ev->button;
        statuscmdn = (int)s->arg.ui;
      }
    }
  } else if ((c = wintoclient(ev->window))) {
    hoverhide();
    focus(c);
    restack(selmon);
    XAllowEvents(dpy, ReplayPointer, CurrentTime);
    click = ClkClientWin;
  } else if ((c = frameclient(ev->window))) {
    int btn;
    hoverhide();
    focus(c);
    restack(selmon);
    XAllowEvents(dpy, ReplayPointer, CurrentTime);
    if (titleh(c) > 0) {
      /* left-click on a titlebar button acts directly (min/max/close),
         clicks elsewhere fall through to the ClkTitleBar bindings (drag) */
      if (ev->button == Button1 && (btn = titlebtnsat(c, ev->x)) >= 0) {
        if (btn == 0)
          hideclient(c);
        else if (btn == 1)
          togglefloatingclient(c);
        else
          killclient(NULL);
        return;
      }
      click = ClkTitleBar;
    } else {
      click = ClkClientWin;
    }
  } else if (ev->window == root) {
    hoverhide();
  }
  for (i = 0; i < LENGTH(buttons); i++)
    if (click == buttons[i].click &&
      buttons[i].func &&
      buttons[i].button == ev->button &&
      CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
      buttons[i].func((click == ClkTagBar || click == ClkWinTitle) &&
      buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void checkotherwm(void) {
  xerrorxlib = XSetErrorHandler(xerrorstart);
  /* this causes an error if some other window manager is running */
  XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
  XSync(dpy, False);
  XSetErrorHandler(xerror);
  XSync(dpy, False);
}

void cleanup(void) {
  Arg a = {.ui = ~0};
  Layout foo = {"", NULL};
  Monitor *m;
  size_t i;

  view(&a);
  selmon->lt[selmon->sellt] = &foo;
  for (m = mons; m; m = m->next)
    while (m->stack)
      unmanage(m->stack, 0);
  XUngrabKey(dpy, AnyKey, AnyModifier, root);
  while (mons)
    cleanupmon(mons);

  if (showsystray) {
    while (systray->icons)
      removesystrayicon(systray->icons);
    XUnmapWindow(dpy, systray->win);
    XDestroyWindow(dpy, systray->win);
    free(systray);
  }

  for (i = 0; i < CurLast; i++)
    drw_cur_free(drw, cursor[i]);
  for (i = 0; i < LENGTH(colors) + 1; i++) {
    XftColorFree(dpy, visual, cmap, &scheme[i][ColFg]);
    XftColorFree(dpy, visual, cmap, &scheme[i][ColBg]);
    XftColorFree(dpy, visual, cmap, &scheme[i][ColBorder]);
    free(scheme[i]);
  }
  free(scheme);
  if (toolwin != None) {
    /* tooldrw shares fonts_set with drw (hovershow swaps it in and back);
       don't let drw_free release it twice */
    tooldrw->fonts = NULL;
    drw_free(tooldrw);
    XDestroyWindow(dpy, toolwin);
  }
  XDestroyWindow(dpy, wmcheckwin);
  drw_free(drw);
  drw_fontset_free(fonts_highlight_set);
  XSync(dpy, False);
  XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
  XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void cleanupmon(Monitor *mon) {
  Monitor *m;
  size_t i;

  if (mon == mons)
    mons = mons->next;
  else {
    for (m = mons; m && m->next != mon; m = m->next);
    if (!m)
      return;
    m->next = mon->next;
  }
  for (i = 0; i < LENGTH(tags); i++)
    if (mon->tagmap[i])
      XFreePixmap(dpy, mon->tagmap[i]);
  free(mon->tagmap);
  if (mon->tagwin != None) {
    XUnmapWindow(dpy, mon->tagwin);
    XDestroyWindow(dpy, mon->tagwin);
  }
  XUnmapWindow(dpy, mon->barwin);
  XDestroyWindow(dpy, mon->barwin);
  free(mon->pertag);
  free(mon);
}

void clientmessage(XEvent *e) {
  XClientMessageEvent *cme = &e->xclient;
  Client *c = wintoclient(cme->window);

  if (showsystray && cme->window == systray->win &&
      cme->message_type == netatom[NetSystemTrayOP]) {
    /* add systray icons */
    if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK)
      systraydock(cme->data.l[2]);
    return;
  }

  if (!c)
    return;
  if (cme->message_type == netatom[NetWMState]) {
    if (cme->data.l[1] == netatom[NetWMFullscreen] ||
        cme->data.l[2] == netatom[NetWMFullscreen])
      setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
                        || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ &&
                            !c->isfullscreen)));
    else if (cme->data.l[1] == netatom[NetWMMaximizedVert] ||
             cme->data.l[2] == netatom[NetWMMaximizedVert] ||
             cme->data.l[1] == netatom[NetWMMaximizedHorz] ||
             cme->data.l[2] == netatom[NetWMMaximizedHorz])
      /* CSD "maximize" uses the same transition as the titlebar button:
         ADD/REMOVE/TOGGLE all mean "press maximize once". */
      togglefloatingclient(c);
  } else if (cme->message_type == wmatom[WMChangeState]) {
    if (cme->data.l[0] == IconicState)
      hideclient(c);
  } else if (cme->message_type == netatom[NetActiveWindow]) {
    if (jump_on_activate) {
      if (c != selmon->sel) {
        unsigned int tag;

        for (tag = 0; tag < LENGTH(tags) && !(c->tags & 1 << tag); tag++)
          ;
        setcurrentmon(c->mon);
        if (tag < LENGTH(tags) && !(c->tags & selmon->tagset[selmon->seltags])) {
          Arg a = {.ui = 1 << tag};

          view(&a);
        }
        if (HIDDEN(c))
          showwin(c); /* an activated hidden client must be displayed */
        focus(c);
        if (c)
          restack(selmon);
      }
    } else if (c != selmon->sel && !c->isurgent) {
      seturgent(c, 1);
    }
  }
}

void configure(Client *c) {
  XConfigureEvent ce;
  int dh = titleh(c);

  /* report the client's visible geometry inside its frame */
  ce.type = ConfigureNotify;
  ce.display = dpy;
  ce.event = c->win;
  ce.window = c->win;
  ce.x = c->x;
  ce.y = c->y + dh;
  ce.width = c->w;
  ce.height = MAX(c->h - dh, 1);
  ce.border_width = 0;
  ce.above = None;
  ce.override_redirect = False;
  XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void configurenotify(XEvent *e) {
  Monitor *m;
  Client *c;
  XConfigureEvent *ev = &e->xconfigure;
  int dirty;

  /* TODO: updategeom handling sucks, needs to be simplified */
  if (ev->window == root) {
    dirty = (sw != ev->width || sh != ev->height);
    sw = ev->width;
    sh = ev->height;
    if (updategeom() || dirty) {
      drw_resize(drw, sw, bh);
      updatebars();
      for (m = mons; m; m = m->next) {
        for (c = m->clients; c; c = c->next)
          if (c->isfullscreen)
            resizeclient(c, m->mx, m->my, m->mw, m->mh);
        resizebarwin(m);
      }
      focus(NULL);
      arrange(NULL);
    }
  }
}

void configurerequest(XEvent *e) {
  Client *c;
  Monitor *m;
  XConfigureRequestEvent *ev = &e->xconfigurerequest;
  XWindowChanges wc;

  if ((c = wintoclient(ev->window))) {
    if (ev->value_mask & CWBorderWidth) {
      /* the frame owns the border; the client stays borderless */
      if (c->frame != None) {
        c->bw = ev->border_width;
        XSetWindowBorderWidth(dpy, c->frame, c->bw);
      }
    } else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
      m = c->mon;
      if (ev->value_mask & CWX) {
        c->oldx = c->x;
        c->x = m->mx + ev->x;
      }
      if (ev->value_mask & CWY) {
        c->oldy = c->y;
        c->y = m->my + ev->y - titleh(c);
      }
      if (ev->value_mask & CWWidth) {
        c->oldw = c->w;
        c->w = ev->width;
      }
      if (ev->value_mask & CWHeight) {
        c->oldh = c->h;
        c->h = ev->height + titleh(c);
      }
      if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
        c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
      if ((c->y + c->h) > m->my + m->mh && c->isfloating)
        c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
      if ((ev->value_mask & (CWX | CWY)) &&
          !(ev->value_mask & (CWWidth | CWHeight)))
        configure(c);
      if (ISVISIBLE(c))
        resize(c, c->x, c->y, c->w, c->h, 0);
    } else
      configure(c);
  } else if (!frameclient(ev->window)) {
    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
  }
  XSync(dpy, False);
}

Monitor *createmon(void) {
  Monitor *m;
  unsigned int i;

  m = ecalloc(1, sizeof(Monitor));
  m->tagset[0] = m->tagset[1] = 1;
  m->mfact = mfact;
  m->nmaster = nmaster;
  m->showbar = showbar;
  m->topbar = topbar;
  m->gappih = gappih;
  m->gappiv = gappiv;
  m->gappoh = gappoh;
  m->gappov = gappov;
  m->lt[0] = &layouts[0];
  m->lt[1] = &layouts[1 % LENGTH(layouts)];
  strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
  m->tagmap = ecalloc(LENGTH(tags), sizeof(Pixmap));
  m->pertag = ecalloc(1, sizeof(Pertag));
  m->pertag->curtag = m->pertag->prevtag = 1;

  for (i = 0; i <= LENGTH(tags); i++) {
    m->pertag->nmasters[i] = m->nmaster;
    m->pertag->mfacts[i] = m->mfact;

    m->pertag->ltidxs[i][0] = m->lt[0];
    m->pertag->ltidxs[i][1] = m->lt[1];
    m->pertag->sellts[i] = m->sellt;

    m->pertag->showbars[i] = m->showbar;
    m->pertag->focusmaster[i] = 0;
    m->pertag->fmlast[i] = NULL;
  }

  return m;
}

void destroynotify(XEvent *e) {
  Client *c;
  XDestroyWindowEvent *ev = &e->xdestroywindow;

  if (frameclient(ev->window))
    return; /* our own frame teardown, ignore */
  if ((c = wintoclient(ev->window))) {
    if (c == hoverc)
      hoverhide();
    unmanage(c, 1);
  } else if (showsystray && (c = wintosystrayicon(ev->window))) {
    removesystrayicon(c);
    updatesystray(1);
  }
  if (selmon->sel && !selmon->sel->neverfocus)
    setfocus(selmon->sel);
}

void detach(Client *c) {
  Client **tc;

  if (!c || !c->mon)
    return;
  for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next)
    ;
  if (*tc)
    *tc = c->next;
}

void detachstack(Client *c) {
  Client **tc, *t;

  if (!c || !c->mon)
    return;
  for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext)
    ;
  if (!*tc)
    return;
  *tc = c->snext;

  if (c == c->mon->sel) {
    for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext)
      ;
    c->mon->sel = t;
  }
}

Monitor *dirtomon(int dir) {
  Monitor *m = NULL;

  if (dir > 0) {
    if (!(m = selmon->next))
      m = mons;
  } else if (selmon == mons)
    for (m = mons; m->next; m = m->next)
      ;
  else
    for (m = mons; m->next != selmon; m = m->next)
      ;
  return m;
}

/* border-colored outline around a rounded-cap tab spanning [x, x + w):
   caps get the arc-following border, the flat middle gets top/bottom
   lines. The outline reads the ColBorder channel of scheme s; pass NULL
   to keep the caller's current scheme (tabs use their own scheme). */
static void drawtabborder(int x, int w, Clr *s) {
  Clr *prev = drw->scheme;

  if (tabborderpx <= 0 || tabborderpx >= bh || w <= 0)
    return;
  if (s && s != prev)
    drw_setscheme(drw, s);
  if (tabr > 0 && w > 2 * tabr) {
    drw_rounded_border(drw, x, 0, bh, tabr, RoundedLeft, tabborderpx);
    drw_rounded_border(drw, x + w - tabr, 0, bh, tabr, RoundedRight, tabborderpx);
    drw_rect_border(drw, x + tabr, 0, w - 2 * tabr, tabborderpx);
    drw_rect_border(drw, x + tabr, bh - tabborderpx, w - 2 * tabr, tabborderpx);
  } else {
    drw_rect_border(drw, x, 0, w, tabborderpx);
    drw_rect_border(drw, x, bh - tabborderpx, w, tabborderpx);
    drw_rect_border(drw, x, 0, tabborderpx, bh);
    drw_rect_border(drw, x + w - tabborderpx, 0, tabborderpx, bh);
  }
  drw_setscheme(drw, prev);
}

/* append a drawn bar element; this table is what clicks and hover consult */
static void addslot(Monitor *m, int x, int w, unsigned int click, Arg arg) {
  if (!w || m->nslots >= BAR_SLOTS)
    return;
  m->slots[m->nslots].x = x;
  m->slots[m->nslots].w = w;
  m->slots[m->nslots].click = click;
  m->slots[m->nslots].arg = arg;
  m->nslots++;
}

/* drawn element under x, NULL on empty bar space */
static const BarSlot *barslotat(Monitor *m, int x) {
  int i;

  for (i = 0; i < m->nslots; i++)
    if (x >= m->slots[i].x && x < m->slots[i].x + m->slots[i].w)
      return &m->slots[i];
  return NULL;
}

/* tags and the layout symbol share one pill whenever they are adjacent, in
   either order; returns how many items starting at k form that pill */
static size_t pillrun(const BarItem *items, size_t nitems, size_t k) {
  if (k + 1 < nitems &&
      ((items[k].mod == BarTags && items[k + 1].mod == BarLayout) ||
       (items[k].mod == BarLayout && items[k + 1].mod == BarTags)))
    return 2;
  return 1;
}

/* width of the visible tags */
static int tagswidth(Monitor *m, int occ) {
  int i, w = 0;

  for (i = 0; i < LENGTH(tags); i++)
    if (occ & 1 << i || m->tagset[m->seltags] & 1 << i)
      w += tagtextw(i);
  return w;
}

/* the visible tags from x on. first draws the pill's left cap and keeps the
   first run clear of it; last shortens the final run so the right cap fits
   inside its padding, the way drawlayout() does. Returns the new x. */
static int drawtags(Monitor *m, int x, int occ, int urg, int first, int last) {
  int gx = x, i, w, lastvis = -1;

  if (last)
    for (i = 0; i < LENGTH(tags); i++)
      if (occ & 1 << i || m->tagset[m->seltags] & 1 << i)
        lastvis = i;

  for (i = 0; i < LENGTH(tags); i++) {
    char text[64];
    int cap, skip, tx, tw;

    /* Do not draw vacant tags */
    if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
      continue;
    template_expand(tagtext, tag_placeholder, &i, text, sizeof(text));
    tw = TEXTW(text);
    w = tw - (i == lastvis ? tabr : 0);
    drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeTagSel : SchemeTagNorm]);
    cap = first && x == gx && tabr > 0;
    skip = cap;
    if (cap)
      drw_rounded(drw, x, 0, bh, tabr, RoundedLeft);
    tx = x;
    x = drw_text(drw, x, 0, w, bh, skip ? tabr : lpad, text, urg & 1 << i, skip);
    addslot(m, tx, tw, ClkTagBar, (Arg){.ui = 1 << i});
  }
  return x;
}

/* the layout symbol from x on; first draws the left cap, last leaves room for
   the right one. Returns the new x. */
static int drawlayout(Monitor *m, int x, int first, int last) {
  int w = TEXTW(m->ltsymbol);
  int skip = first && tabr > 0;

  drw_setscheme(drw, scheme[SchemeLayout]);
  if (skip)
    drw_rounded(drw, x, 0, bh, tabr, RoundedLeft);
  addslot(m, x, w, ClkLtSymbol, (Arg){0});
  return drw_text(drw, x, 0, w - (last ? tabr : 0), bh, skip ? tabr : lpad,
                  m->ltsymbol, 0, skip);
}

/* draw the pill formed by the items [k, k + pillrun(k)): the tags pill and the
   layout symbol join, everything else is its own pill. Records the slots and
   closes the pill with a right cap plus its outline. Returns the new x. */
static int drawbarpill(Monitor *m, const BarItem *items, size_t nitems, size_t k,
                       int x, int occ, int urg) {
  int gx = x, first = 1;
  size_t run = pillrun(items, nitems, k), j;

  for (j = 0; j < run; j++) {
    int before = x;

    if (items[k + j].mod == BarTags)
      x = drawtags(m, x, occ, urg, first, j + 1 == run);
    else
      x = drawlayout(m, x, first, j + 1 == run);
    if (x > before)
      first = 0;
  }

  if (x > gx) {
    x += drw_rounded(drw, x, 0, bh, tabr, RoundedRight);
    drawtabborder(gx, x - gx, scheme[SchemeStatus]);
  }
  return x;
}

/* width a zone takes, counting the gaps between its pills; avail is the
   room elastic tabs may stretch into (0 outside the center zone) */
static int barzonewidth(Monitor *m, const BarItem *items, size_t nitems,
                        int occ, int n, int avail) {
  size_t k, run, j;
  int w = 0;

  for (k = 0; k < nitems; k += run) {
    run = pillrun(items, nitems, k);
    if (k)
      w += (int)tabgap;
    for (j = 0; j < run; j++) {
      switch (items[k + j].mod) {
      case BarTags:
        w += tagswidth(m, occ);
        break;
      case BarLayout:
        w += TEXTW(m->ltsymbol);
        break;
      case BarTabs:
        w += tablayout(m, avail, NULL, 0, NULL);
        break;
      case BarStatus:
        if (m == selmon)
          w += statuswidth(items[k + j].ids);
        break;
      default: /* BarNone: an intentionally empty slot */
        break;
      }
    }
  }
  return w;
}

/* the zone lists a monitor uses: portrait (wh > ww) has its own set, so the
   layout can differ per orientation without a marker in the status text */
typedef struct {
  const BarItem *left, *center, *right;
  size_t nleft, ncenter, nright;
} BarZones;

static BarZones barzones(Monitor *m) {
  BarZones z;

  if (m->wh > m->ww) {
    z.left = bar_left_portrait;
    z.nleft = LENGTH(bar_left_portrait);
    z.center = bar_center_portrait;
    z.ncenter = LENGTH(bar_center_portrait);
    z.right = bar_right_portrait;
    z.nright = LENGTH(bar_right_portrait);
  } else {
    z.left = bar_left;
    z.nleft = LENGTH(bar_left);
    z.center = bar_center;
    z.ncenter = LENGTH(bar_center);
    z.right = bar_right;
    z.nright = LENGTH(bar_right);
  }
  return z;
}

/* draw one zone left to right and record its slots; tabs take width w */
static int drawzone(Monitor *m, const BarItem *items, size_t nitems, int x,
                    int w, int occ, int urg, int n) {
  size_t k;

  for (k = 0; k < nitems; k++) {
    if (k)
      x += (int)tabgap;
    switch (items[k].mod) {
    case BarTags:
    case BarLayout:
      x = drawbarpill(m, items, nitems, k, x, occ, urg);
      k += pillrun(items, nitems, k) - 1;
      break;
    case BarTabs: {
      /* the zone was sized to this row, so laying it out again gives the
         same geometry the measurement used */
      TabCell cells[TAB_CELLS];
      int nc = 0, roww = tablayout(m, w, cells, TAB_CELLS, &nc);

      if (nc > 0)
        tabdraw(m, x, cells, nc);
      x += roww;
      break;
    }
    case BarStatus:
      x = drawstatuspills(m, x, items[k].ids);
      break;
    default: /* BarNone: an intentionally empty slot */
      break;
    }
  }
  return x;
}

void drawbar(Monitor *m) {
  int leftw, centerw, centerx, rightw, barw, stw = 0, n = 0;
  BarZones zones;
  unsigned int occ = 0, urg = 0;
  Client *c;

  if (!m->showbar)
    return;

  if (showsystray && m == systraytomon(m)) {
    stw = getsystraywidth();
  }

  drw_setscheme(drw, scheme[SchemeEmpty]);
  drw_rect(drw, 0, 0, m->ww, bh, 1, 1);
  resizebarwin(m);

  for (c = m->clients; c; c = c->next) {
    if (ISVISIBLE(c))
      n++;
    occ |= c->tags;
    if (c->isurgent)
      urg |= c->tags;
  }

  /* three zones inside the bar window: left from the start, right from the
     end (systray reserved), center in the middle. The center zone is sized to
     its own content and then centred on the monitor's middle, not on the bar
     window's: the window stops at the systray, so window centring would sit
     half a systray left of the monitor's middle. */
  zones = barzones(m);
  barw = m->ww - 2 * sp - stw;
  leftw = barzonewidth(m, zones.left, zones.nleft, occ, n, 0);
  rightw = barzonewidth(m, zones.right, zones.nright, occ, n, 0);
  if (leftw > barw)
    leftw = barw;
  if (rightw > barw - leftw)
    rightw = barw - leftw;
  centerw = barzonewidth(m, zones.center, zones.ncenter, occ, n,
                         barw - leftw - rightw);
  if (centerw > barw - leftw - rightw)
    centerw = barw - leftw - rightw;
  if (centerw < 0)
    centerw = 0;
  /* centred on the monitor while there is slack, but never over a side zone:
     a middle that is as wide as the gap can only fill that gap */
  centerx = bar_centerx(barw, stw, leftw, rightw, centerw);

  m->nslots = 0;
  drawzone(m, zones.left, zones.nleft, 0, 0, occ, urg, n);
  drawzone(m, zones.center, zones.ncenter, centerx, centerw, occ, urg, n);
  drawzone(m, zones.right, zones.nright, barw - rightw, 0, occ, urg, n);

  drw_map(drw, m->barwin, 0, 0, m->ww - stw, bh);
}

void drawbars(void) {
  Monitor *m;

  for (m = mons; m; m = m->next)
    drawbar(m);

  if (showsystray && !systraypinning)
    updatesystray(0);
}

/* vertical space the titlebar takes inside c's frame, 0 when disabled/hidden.
   c->x/y/w/h always describe the FRAME; the client lives at (0, titleh). */
int
titleh(Client *c)
{
  if (!showtitlebar || !c || c->isfullscreen || c->notitle)
    return 0;
  return th;
}

/* 1 if the client asked for no window-manager decorations via Motif hints */
static int
motifnodecor(Window w)
{
  Atom actual;
  int fmt;
  unsigned long n, extra;
  unsigned char *p = NULL;
  long *m;
  int nodecor = 0;

  if (motifwmhints == None)
    return 0;
  if (XGetWindowProperty(dpy, w, motifwmhints, 0, 5, False, AnyPropertyType,
                         &actual, &fmt, &n, &extra, &p) != Success ||
      !p || fmt != 32 || n < 3)
    goto out;
  m = (long *)p;
  nodecor = ((m[0] & 2L) && m[2] == 0); /* MWM_HINTS_DECORATIONS && decorations == 0 */
out:
  if (p)
    XFree(p);
  return nodecor;
}

/* 1 if the client advertises GTK client-side decorations */
static int
hasgtkcsd(Window w)
{
  Atom actual;
  int fmt;
  unsigned long n, extra;
  unsigned char *p = NULL;
  int found = 0;

  if (gtkframeextents == None)
    return 0;
  if (XGetWindowProperty(dpy, w, gtkframeextents, 0, 4, False, AnyPropertyType,
                         &actual, &fmt, &n, &extra, &p) != Success || !p)
    goto out;
  found = (n > 0);
out:
  if (p)
    XFree(p);
  return found;
}

/* resolve c->notitle from the rule override and live self-decoration hints */
void
updatenodecor(Client *c)
{
  if (c->rule_notitle) {
    c->notitle = 1;
    return;
  }
  c->notitle = 0;
  if (motifnodecor(c->win) || hasgtkcsd(c->win))
    c->notitle = 1;
}

/* create c->frame, reparent c->win into it and place it below the title area.
   The frame owns the border (c->bw); the client border is forced to 0. */
void
createframe(Client *c)
{
  XSetWindowAttributes wa = {
    .override_redirect = False,
    .background_pixel = 0,
    .border_pixel = scheme[SchemeNorm][ColBorder].pixel,
    .colormap = cmap,
    .event_mask = ButtonPressMask | ExposureMask | EnterWindowMask |
                  LeaveWindowMask | SubstructureRedirectMask |
                  SubstructureNotifyMask,
  };
  XClassHint ch = {"dwm", "dwm"};
  XWindowChanges wc;

  c->frame = XCreateWindow(dpy, root, c->x, c->y, c->w, c->h, c->bw,
    depth, InputOutput, visual,
    CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &wa);
  XDefineCursor(dpy, c->frame, cursor[CurNormal]->cursor);
  XSetClassHint(dpy, c->frame, &ch);
  c->ignoreunmap++;
  /* keep the client alive if the frame dies with us (quit/hot-restart) */
  XAddToSaveSet(dpy, c->win);
  XReparentWindow(dpy, c->win, c->frame, 0, titleh(c));
  wc.border_width = 0;
  XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
  placeclient(c);
}

/* (re)position c->win inside c->frame below the title area */
void
placeclient(Client *c)
{
  int dh = titleh(c);

  XMoveResizeWindow(dpy, c->win, 0, dh, c->w, MAX(c->h - dh, 1));
}

/* titlebtns is configured in config.h; index 0 = minimize, 1 = maximize, 2 = close */
#define NTITLEBTNS (LENGTH(titlebtns))

/* index of the titlebar button at frame-relative x, -1 for none */
int
titlebtnsat(Client *c, int x)
{
  int btnw;

  if (!c || titleh(c) <= 0)
    return -1;
  btnw = NTITLEBTNS * th;
  if (c->w < btnw + lrpad || x < c->w - btnw)
    return -1;
  return MIN((x - (c->w - btnw)) / th, (int)NTITLEBTNS - 1);
}

/* draw the title + buttons onto c->frame's top strip.
   The icon (if shown) stays at the left; title layout follows
   titlebaralign (0 = left, 1 = true center of the full strip width,
   2 = right against the button area). */
void
drawtitle(Client *c)
{
  int scm, i, btnw, titlew, bx, lp, tx, txtw, hasicon, iconw;
  char text[256];

  if (!c || c->frame == None || titleh(c) <= 0)
    return;
  if (!ISVISIBLE(c) || HIDDEN(c))
    return;
  scm = (c == c->mon->sel) ? SchemeSel : SchemeNorm;
  updateicon(c);
  drw_setscheme(drw, scheme[scm]);
  btnw = (c->w >= (int)(NTITLEBTNS * th + lrpad)) ? NTITLEBTNS * th : 0;
  titlew = c->w - btnw;
  template_expand(titlebartext, tab_placeholder, c, text, sizeof(text));
  txtw = TEXTW(text) - lrpad;
  hasicon = showtitleicon && c->icon && titlew >= (int)(c->icw + ICONSPACING + lrpad);
  iconw = hasicon ? (int)(c->icw + ICONSPACING) : 0;
  /* the icon stays at the left; alignment positions the text only */
  if (titlebaralign == 2) /* right: text ends at the button area */
    tx = MAX((int)lpad + iconw, titlew - txtw);
  else if (titlebaralign == 1) /* center: true center of the full titlebar width */
    tx = MAX((int)lpad + iconw, (c->w - txtw) / 2);
  else /* left (0 and anything unexpected) */
    tx = lpad + iconw;
  drw_text(drw, 0, 0, titlew, th, tx, text, 0, 0);
  if (hasicon)
    drw_pic(drw, lpad, (th - c->ich) / 2, c->icw, c->ich, c->icon);
  for (i = 0; i < (int)NTITLEBTNS && btnw; i++) {
    bx = titlew + i * th;
    lp = MAX((th - (int)drw_fontset_getwidth(drw, titlebtns[i])) / 2, 0);
    drw_text(drw, bx, 0, th, th, lp, titlebtns[i], 0, 0);
  }
  drw_map(drw, c->frame, 0, 0, c->w, th);
}

void
drawtitles(void)
{
  Monitor *m;
  Client *c;

  if (th <= 0)
    return;
  for (m = mons; m; m = m->next)
    for (c = m->clients; c; c = c->next)
      if (ISVISIBLE(c) && !HIDDEN(c))
        drawtitle(c);
}

Client *
frameclient(Window w)
{
  Monitor *m;
  Client *c;

  if (w == None)
    return NULL;
  for (m = mons; m; m = m->next)
    for (c = m->clients; c; c = c->next)
      if (c->frame != None && c->frame == w)
        return c;
  return NULL;
}

/* 1 if s is an "#RRGGBB" color string */
static int ishexcolor(const char *s) {
  int i;

  if (!s || s[0] != '#' || strnlen(s, 8) != 7)
    return 0;
  for (i = 1; i < 7; i++)
    if (!isxdigit((unsigned char)s[i]))
      return 0;
  return 1;
}

/* Paint a pill's body and its left cap in the colours that are current once
   the pill's own leading codes (the writer's ^b pane background) have been
   applied, so cap, body and text match. The cap corner is cleared first so
   the bar background shows through outside the arc. */
static void drawpillcap(Clr *work, int capx, int pillw) {
  if (capx < 0)
    return;
  drw_setscheme(drw, work);
  if (pillw > 0)
    drw_rect(drw, capx, 0, (unsigned int)pillw, bh, 1, 1);
  drw_setscheme(drw, scheme[SchemeEmpty]);
  drw_rect(drw, capx, 0, tabr, bh, 1, 0);
  drw_setscheme(drw, work);
  drw_rounded(drw, capx, 0, bh, tabr, RoundedLeft);
}

/* draw the configured status pills at x and record a slot per drawn block so
   a click resolves to that block's INDEX; returns the x after the pills.
   Only the selected monitor carries status. */
static int drawstatuspills(Monitor *m, int x, const int *ids) {
  int i, w, roww, origin, tabstart;
  short iscode = 0;
  /* 1 = the next text run draws right after a '(' cap, so its left
     padding must be skipped to keep the rounded corner visible */
  int skip_pad = 0;
  char *text;
  Clr *work = scheme[LENGTH(colors)];
  Clr pillbase[3]; /* pill colours as of its first drawn run, for ^d */
  int capx = -1;   /* pending left cap of the open pill, -1 when none */
  int pillw = 0;   /* its body width, from the layout pass */
  int pillprologue = 0; /* 1 while the pill's leading codes are applied */

  if (m != selmon)
    return x;

  /* one shared parse for drawing and click resolution: stbuf holds the
     visible text (control characters dropped), stblocks the block ids, and
     pbuf the configured pills with their cap markers injected */
  statusparse(stext);
  statuspills_build(ids);
  text = pbuf;

  /* width of the pills; kept apart from `w`, which the interpreter reuses */
  roww = stpills_w;
  origin = x;
  tabstart = x;

  /* draw on the working scheme, reset to the configured status colours so
     ^c/^b from an earlier pill or the previous frame cannot leak */
  drw_setscheme(drw, work);
  work[ColFg] = scheme[SchemeStatus][ColFg];
  work[ColBg] = scheme[SchemeStatus][ColBg];
  work[ColBorder] = scheme[SchemeStatus][ColBorder];
  memcpy(pillbase, work, sizeof pillbase);

  /* process status text */
  i = -1;
  while (text[++i]) {
    if (text[i] == '^' && !iscode) {
      iscode = 1;

      text[i] = '\0';
      if (strlen(text) > 0) {
        if (capx >= 0) { /* first run of a pill: body and cap, then the text */
          drawpillcap(work, capx, pillw);
          pillprologue = 0;
          capx = -1;
        }
        w = TEXTW(text);
        drw_text(drw, x, 0, w, bh, lpad, text, 0, skip_pad);
        skip_pad = 0;
        x += w;
      }

      /* process code; stop at the closing '^' or at the end of the
       * string (an unterminated code must not read past the buffer) */
      while (text[++i] && text[i] != '^') {
        if (text[i] == 'c' || text[i] == 'b') {
          char buf[8];
          int n = 0, isbg = (text[i] == 'b');
          while (n < 7 && (text + i + 1)[n] && (text + i + 1)[n] != '^')
            n++;
          memcpy(buf, (char *)text + i + 1, n);
          buf[n] = '\0';
          if (ishexcolor(buf)) {
            drw_clr_create(drw, &drw->scheme[isbg ? ColBg : ColFg], buf,
                           alphas[SchemeStatus][isbg ? 1 : 0]);
            /* the pill's leading ^b is its pane background: remember it as
               the colour ^d resets to */
            if (isbg && pillprologue)
              pillbase[ColBg] = drw->scheme[ColBg];
          }
          i += n;
        } else if (text[i] == 'd') {
          drw->scheme[ColFg] = pillbase[ColFg];
          drw->scheme[ColBg] = pillbase[ColBg];
        } else if (text[i] == 'r') {
          int rx, ry, rw, rh;

          rx = atoi(text + ++i);
          while (text[i] && text[i] != ',')
            i++;
          ry = text[i] ? atoi(text + ++i) : 0;
          while (text[i] && text[i] != ',')
            i++;
          rw = text[i] ? atoi(text + ++i) : 0;
          while (text[i] && text[i] != ',')
            i++;
          rh = text[i] ? atoi(text + ++i) : 0;

          drw_rect(drw, rx + x, ry, rw, rh, 1, 0);
        } else if (text[i] == 'f')
          x += atoi(text + ++i);
        else if (text[i] == '(' && tabradius > 0) {
          /* remember the cap and draw it with the pill's first run instead:
             the pill's own ^b must apply first so cap, body and text agree,
             and the body width is known from the layout pass */
          tabstart = x;
          capx = x;
          pillw = (spillidx < nspills) ? spills[spillidx].w : 0;
          spillidx++;
          /* reset the pill base to the configured colours; the pill's own
             leading ^b then refines its background */
          pillbase[ColFg] = scheme[SchemeStatus][ColFg];
          pillbase[ColBg] = scheme[SchemeStatus][ColBg];
          pillbase[ColBorder] = scheme[SchemeStatus][ColBorder];
          pillprologue = 1;
          skip_pad = 1;
        } else if (text[i] == ')' && tabradius > 0) {
          if (capx >= 0) { /* a pill with no drawn text at all */
            drawpillcap(work, capx, pillw);
            pillprologue = 0;
            capx = -1;
          }
          drw_setscheme(drw, scheme[SchemeEmpty]);
          drw_rect(drw, x-tabr, 0, tabr + tabgap, bh, 1, 0);
          drw_setscheme(drw, work);
          drw_rounded(drw, x-tabr, 0, bh, tabr, RoundedRight);
          drawtabborder(tabstart, x - tabstart, scheme[SchemeStatus]);
          x += tabgap;
        }
      }

      if (!text[i])
        break; /* unterminated code: drop the rest of the text */

      text = text + i + 1;
      i = -1;
      iscode = 0;
    }
  }

  if (!iscode) {
    if (capx >= 0) { /* a pill with no drawn text at all */
      drawpillcap(work, capx, pillw);
      pillprologue = 0;
      capx = -1;
    }
    /* an empty trailing run must not be drawn: drw_text() would fill
       TEXTW("") = lrpad pixels in the scheme's background, leaving a stray
       block-coloured rectangle after the last pill */
    if (*text) {
      w = TEXTW(text);
      drw_text(drw, x, 0, w, bh, lpad, text, 0, 0);
    }
  }

  drw_setscheme(drw, scheme[SchemeNorm]);

  /* record the drawn blocks; a click resolves to the block's id, which is
     the INDEX the writer's click command expects */
  for (i = 0; i < nscells; i++)
    addslot(m, origin + scells[i].x, scells[i].w, ClkStatusText,
            (Arg){.ui = scells[i].id});

  return origin + roww;
}

/* expand a template string ({name}, {icon}, ...) into buf */
static void template_expand(const char *fmt, template_resolve resolve, void *ctx, char *buf, size_t len) {
  size_t n = 0;

  while (*fmt && n < len - 1) {
    const char *val;
    size_t plen;

    if (*fmt == '{' && (val = resolve(fmt, &plen, ctx))) {
      size_t vlen = MIN(strlen(val), len - n - 1);
      strncpy(buf + n, val, vlen);
      n += vlen;
      fmt += plen;
    } else {
      buf[n++] = *fmt++;
    }
  }
  buf[n] = '\0';
}

/* resolve {title}, {class} against a client */
static const char *tab_placeholder(const char *f, size_t *plen, void *ctx) {
  Client *c = ctx;

  if (!strncmp(f, "{title}", 7)) {
    *plen = 7;
    return c->name;
  }
  if (!strncmp(f, "{class}", 7)) {
    *plen = 7;
    return c->class;
  }
  return NULL;
}

/* resolve {name}, {icon}, {index} against a 0-based tag index */
static const char *tag_placeholder(const char *f, size_t *plen, void *ctx) {
  unsigned int i = *(unsigned int *)ctx;

  if (!strncmp(f, "{name}", 6)) {
    *plen = 6;
    return tag_names[i];
  }
  if (!strncmp(f, "{icon}", 6)) {
    *plen = 6;
    return tags[i];
  }
  if (!strncmp(f, "{index}", 7)) {
    static char buf[16];
    *plen = 7;
    snprintf(buf, sizeof buf, "%u", i + 1);
    return buf;
  }
  return NULL;
}

/* rendered width of tag i, shared by drawtags and tagswidth */
static int tagtextw(unsigned int i) {
  char text[64];
  template_expand(tagtext, tag_placeholder, &i, text, sizeof(text));
  return TEXTW(text);
}

/* index of the single tag in a 1 << i mask, -1 when it is not one tag */
static int tagindex(unsigned int mask) {
  int i;

  for (i = 0; i < (int)LENGTH(tags); i++)
    if (mask & 1u << i)
      return i;
  return -1;
}

/* lay the tab row out inside avail. When cells is non-NULL the per-cell
   geometry is recorded too, so the zone measurement (cells == NULL) and the
   painter always agree: both run this one function. Returns the row width.
   TabModeIconTitle keeps the configured nominal width and levels the row to
   avail when it does not fit; TabModeIcons sizes each cell to its icon and,
   when the icons do not fit, closes the gaps before dropping trailing ones. */
static int tablayout(Monitor *m, int avail, TabCell *cells, int max,
                     int *ncells) {
  int widths[TAB_CELLS];
  int i, n = 0, want, total, x, g;
  BarCellsMode mode;
  Client *c;

  if (ncells)
    *ncells = 0;

  if (tabmode == TabModeIcons) {
    int end[TAB_CELLS];

    for (g = (int)tabgap;; g--) {
      x = (int)tabr;
      i = 0;
      for (c = m->clients; c && i < TAB_CELLS; c = c->next) {
        int iw, ih;

        if (!ISVISIBLE(c))
          continue;
        tabicon_size(c, &iw, &ih);
        if (!iw)
          iw = 1; /* keep the client clickable even without any icon */
        x += (int)iw;
        end[i] = x; /* right edge of this cell, pill-relative */
        if (cells && i < max) {
          cells[i].x = x - (int)iw;
          cells[i].w = (int)iw;
          cells[i].c = c;
        }
        x += g;
        i++;
      }
      total = i ? end[i - 1] + (int)tabr : 0;
      if (total <= avail || g <= 0)
        break;
    }
    while (i > 0 && end[i - 1] + (int)tabr > avail) /* drop trailing cells */
      i--;
    total = i ? end[i - 1] + (int)tabr : 0;
    if (ncells)
      *ncells = i;
    return total;
  }

  for (c = m->clients; c; c = c->next)
    if (ISVISIBLE(c))
      n++;
  if (n > TAB_CELLS)
    n = TAB_CELLS;
  if (n <= 0)
    return 0;

  /* the nominal width is a character count so it tracks the font */
  want = tabwidth * drw_fontset_getwidth(drw, " ") + lrpad;
  mode = tabsize == TabFill    ? BarCellsFill
         : tabsize == TabFixed ? BarCellsFixed
                               : BarCellsFit;
  total = bar_cells(want, n, avail, (int)tabgap, mode, widths, TAB_CELLS);

  x = 0;
  i = 0;
  for (c = m->clients; c && i < n; c = c->next) {
    if (!ISVISIBLE(c))
      continue;
    if (x + widths[i] > avail) /* TabFixed overflow: drop what does not fit */
      break;
    if (cells && i < max) {
      cells[i].x = x;
      cells[i].w = widths[i];
      cells[i].c = c;
    }
    x += widths[i] + (int)tabgap;
    i++;
  }
  if (i < n)
    total = x; /* truncated: the row is only what fits */
  if (ncells)
    *ncells = i;
  return total;
}

/* ---- tab icon ----------------------------------------------------------
 * Clients without _NET_WM_ICON have no picture to show. The PNG named by
 * tabiconpath is loaded once as a coverage mask and tinted with the scheme's
 * foreground, then modulated by that scheme's alpha exactly like a real icon
 * (both end up going through prealpha()). A missing file simply means
 * "no icon", never an error.
 *
 * TabModeIcons shrinks the tab icon to leave room under it for the selection
 * dot, so a client carries a second picture at that size (c->tabicon). */

/* icon size the tab row uses; TabModeIcons reserves the dot's room, sharing
   the same arithmetic bar_tabdotroom() the painter uses */
static int tabiconsize(void) {
  return bar_tabiconsize(ICONSIZE,
                         tabmode == TabModeIcons ? (int)tabseldot : 0);
}

/* tabiconpath with "$HOME/" or "~/" expanded */
static const char *tabiconfile(void) {
  static char path[512];
  const char *home = getenv("HOME");
  const char *rel = tabiconpath;

  if (!home)
    home = "";
  if (!strncmp(rel, "$HOME/", 6))
    rel += 6;
  else if (rel[0] == '~' && rel[1] == '/')
    rel += 2;
  else
    return rel;
  snprintf(path, sizeof path, "%s/%s", home, rel);
  return path;
}

/* coverage mask at the file's own resolution, loaded once; NULL if unusable */
static unsigned char *tabfal_mask;
static int tabfal_sw, tabfal_sh;

static int tabfal_load(void) {
  static int tried;
  Imlib_Image im;
  DATA32 *src;
  int i, iw, ih, n;

  if (tried)
    return tabfal_mask != NULL;
  tried = 1;
  im = imlib_load_image(tabiconfile());
  if (!im)
    return 0;
  imlib_context_set_image(im);
  iw = imlib_image_get_width();
  ih = imlib_image_get_height();
  src = (iw > 0 && ih > 0) ? imlib_image_get_data_for_reading_only() : NULL;
  if (!src) {
    imlib_free_image();
    return 0;
  }
  tabfal_sw = iw;
  tabfal_sh = ih;
  tabfal_mask = ecalloc((size_t)iw * ih, 1);
  for (n = iw * ih, i = 0; i < n; i++)
    tabfal_mask[i] = (unsigned char)(src[i] >> 24u);
  imlib_free_image();
  return 1;
}

/* bilinear coverage sample, in source pixels */
static unsigned int tabfal_cov(double fx, double fy) {
  int x0 = (int)fx, y0 = (int)fy, x1, y1;
  double dx = fx - x0, dy = fy - y0, top, bot;

  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x0 > tabfal_sw - 1)
    x0 = tabfal_sw - 1;
  if (y0 > tabfal_sh - 1)
    y0 = tabfal_sh - 1;
  x1 = MIN(x0 + 1, tabfal_sw - 1);
  y1 = MIN(y0 + 1, tabfal_sh - 1);
  top = tabfal_mask[y0 * tabfal_sw + x0] +
        (tabfal_mask[y0 * tabfal_sw + x1] - tabfal_mask[y0 * tabfal_sw + x0]) * dx;
  bot = tabfal_mask[y1 * tabfal_sw + x0] +
        (tabfal_mask[y1 * tabfal_sw + x1] - tabfal_mask[y1 * tabfal_sw + x0]) * dx;
  return (unsigned int)(top + (bot - top) * dy + 0.5);
}

/* size the fallback takes at the given target size: longest side to `size`,
   like geticonprop does for real icons */
static int tabfal_size(int size, int *w, int *h) {
  if (!tabfal_load())
    return 0;
  if (tabfal_sw <= tabfal_sh) {
    *h = size;
    *w = MAX(1, tabfal_sw * size / tabfal_sh);
  } else {
    *w = size;
    *h = MAX(1, tabfal_sh * size / tabfal_sw);
  }
  return 1;
}

/* fallback picture for a scheme at the given target size, built on first use */
static Picture tabfal_pic(int scm, int size, int *w, int *h) {
  static Picture pic[3];
  static int pic_size[3], state[3];
  unsigned int fg;
  DATA32 *buf;
  int dw, dh, x, y;

  if (scm != SchemeNorm && scm != SchemeSel && scm != SchemeHid)
    return None;
  if (!tabfal_size(size, &dw, &dh))
    return None;
  if (state[scm] == 1 && pic_size[scm] == size) {
    if (w)
      *w = dw;
    if (h)
      *h = dh;
    return pic[scm];
  }
  if (pic[scm]) {
    XRenderFreePicture(drw->dpy, pic[scm]);
    pic[scm] = None;
  }
  state[scm] = -1;

  fg = scheme[scm][ColFg].pixel;
  buf = ecalloc((size_t)dw * dh, sizeof(DATA32));
  for (y = 0; y < dh; y++)
    for (x = 0; x < dw; x++) {
      /* the mask's coverage becomes the pixel alpha, the scheme's foreground
         its colour, and prealpha() modulates both like a real icon */
      unsigned int cov = tabfal_cov((double)x * tabfal_sw / dw,
                                    (double)y * tabfal_sh / dh);
      uint32_t p = ((uint32_t)cov << 24u) | (fg & 0x00FFFFFFu);

      buf[(size_t)y * dw + x] = prealpha(p, fg >> 24u);
    }
  pic[scm] = drw_picture_create_resized(drw, (char *)buf, dw, dh, dw, dh);
  free(buf);
  if (!pic[scm])
    return None;
  state[scm] = 1;
  pic_size[scm] = size;
  if (w)
    *w = dw;
  if (h)
    *h = dh;
  return pic[scm];
}

/* size of the icon c's tab will show, without building anything */
static void tabicon_size(Client *c, int *w, int *h) {
  if (c->tabicon) {
    *w = (int)c->tabicw;
    *h = (int)c->tabich;
    return;
  }
  if (c->icon) {
    *w = (int)c->icw;
    *h = (int)c->ich;
    return;
  }
  if (!tabfal_size(tabiconsize(), w, h))
    *w = *h = 0;
}

/* the icon c's tab shows, at the tab's own size: the client's picture or the
   fallback. Fills the picture and its size; 0 when there is nothing to show */
static int tabicon_get(Client *c, int scm, Picture *pic, unsigned int *w,
                       unsigned int *h) {
  int iw, ih;

  if (c->tabicon) {
    *pic = c->tabicon;
    iw = (int)c->tabicw;
    ih = (int)c->tabich;
  } else if (c->icon) {
    *pic = c->icon;
    iw = (int)c->icw;
    ih = (int)c->ich;
  } else if ((*pic = tabfal_pic(scm, tabiconsize(), &iw, &ih)) == None) {
    return 0;
  }
  *w = (unsigned int)iw;
  *h = (unsigned int)ih;
  return 1;
}

/* a filled disc of diameter d at (x, y) in the scheme's background colour:
   two pill caps of radius d/2 back to back */
static void drawdisc(int x, int y, int d) {
  int r = d / 2;

  if (r <= 0)
    return;
  drw_rounded(drw, x, y, (unsigned int)(2 * r), r, RoundedLeft);
  drw_rounded(drw, x + r, y, (unsigned int)(2 * r), r, RoundedRight);
}

/* the TabModeIcons selection mark: a filled circle of radius r in the scheme's
   foreground, centred under the selected icon. drw_rounded paints in ColBg, so
   it is handed a scratch scheme whose background is that foreground. */
static void tabseldot_paint(int scm, int cx, int cy, int r) {
  Clr *work = scheme[LENGTH(colors)];

  if (r <= 0)
    return;
  /* drw_rounded() paints in ColBg, so that is all the scratch scheme needs */
  drw_setscheme(drw, work);
  work[ColBg] = scheme[scm][ColFg];
  drawdisc(cx - r, cy - r, 2 * r);
  drw_setscheme(drw, scheme[scm]);
}

/* paint one tab pill of width w at x for client c, and record its slot */
static void tabpaint(Monitor *m, int x, int w, Client *c) {
  int scm, highlight, tw, cx;
  int boxw = drw->fonts->h / 6 + 2;
  char text[256];

  if (m->sel == c)
    scm = SchemeSel;
  else if (HIDDEN(c))
    scm = SchemeHid;
  else
    scm = SchemeNorm;
  drw_setscheme(drw, scheme[scm]);
  highlight = scm == SchemeSel && fonts_highlight_set;
  if (highlight)
    drw_setfontset(drw, fonts_highlight_set);

  template_expand(tabtext, tab_placeholder, c, text, sizeof(text));
  tw = TEXTW(text) - lrpad;

  /* content area: the rounded-cap branch insets it by the cap radius;
     icon+text are centred inside it, minpad is the left stop */
  {
    int cxx = x + tabr;
    int contentw = MAX(w - tabr * 2, 0);
    int minpad = tabr > 0 ? 0 : (int)lpad;

    if (tabr > 0)
      drw_rounded(drw, x, 0, bh, tabr, RoundedLeft);
    Picture ic = None;
    unsigned int iw = 0, ih = 0;

    if (tabicon_get(c, scm, &ic, &iw, &ih) &&
        contentw - minpad >= (int)(iw + ICONSPACING)) {
      cx = MAX(minpad, (contentw - tw - (int)(iw + ICONSPACING)) / 2);
      drw_text(drw, cxx, 0, contentw, bh, cx + iw + ICONSPACING, text, 0, 0);
      drw_pic(drw, cxx + cx, (bh - (int)ih) / 2, iw, ih, ic);
    } else {
      cx = MAX(minpad, (contentw - tw) / 2);
      drw_text(drw, cxx, 0, contentw, bh, cx, text, 0, 0);
    }
    if (tabr > 0)
      drw_rounded(drw, x + w - tabr, 0, bh, tabr, RoundedRight);
  }
  if (drw->scheme == scheme[SchemeSel])
    drawtabborder(x, w, NULL);

  // floating marker
  if (c->isfloating)
    drw_rect(drw, x + w - tabr - boxw, (bh - boxw) / 2, boxw, boxw, c->isfixed, 0);
  if (highlight)
    drw_setfontset(drw, fonts_set);

  addslot(m, x, w, ClkWinTitle, (Arg){.v = c});
}

/* paint a row laid out by tablayout() at x0 */
static void tabdraw(Monitor *m, int x0, const TabCell *cells, int ncells) {
  int i;

  if (ncells <= 0)
    return;

  if (tabmode != TabModeIcons) { /* one pill per client */
    for (i = 0; i < ncells; i++)
      tabpaint(m, x0 + cells[i].x, cells[i].w, cells[i].c);
    return;
  }

  /* one pill shared by every icon, painted in the selected scheme: the icons'
     own tint and the dot under the selected one carry the per-client state */
  {
    int pillw = cells[ncells - 1].x + cells[ncells - 1].w + (int)tabr;

    drw_setscheme(drw, scheme[SchemeSel]);
    drw_rect(drw, x0, 0, (unsigned int)pillw, bh, 1, 1);
    if (tabr > 0) {
      drw_setscheme(drw, scheme[SchemeEmpty]);
      drw_rect(drw, x0, 0, tabr, bh, 1, 0);
      drw_rect(drw, x0 + pillw - tabr, 0, tabr, bh, 1, 0);
      drw_setscheme(drw, scheme[SchemeSel]);
      drw_rounded(drw, x0, 0, bh, tabr, RoundedLeft);
      drw_rounded(drw, x0 + pillw - tabr, 0, bh, tabr, RoundedRight);
      drawtabborder(x0, pillw, NULL);
    }
  }

  for (i = 0; i < ncells; i++) {
    Client *c = cells[i].c;
    Picture ic = None;
    unsigned int iw = 0, ih = 0;
    int scm = (m->sel == c) ? SchemeSel : (HIDDEN(c) ? SchemeHid : SchemeNorm);
    int cx = x0 + cells[i].x;
    int r = (int)tabseldot;
    int gap = r > 0 ? 1 : 0;
    int total, top;

    tabicon_get(c, scm, &ic, &iw, &ih);
    /* the dot's room is reserved for every cell so the icons stay aligned;
       bar_tabdotroom() is what tabiconsize() shrank the icon by, so the icon
       and its dot always fit the row together */
    total = (int)ih + bar_tabdotroom(r);
    top = (bh - total) / 2;
    if (top < 0)
      top = 0;

    if (scm == SchemeSel && r > 0)
      tabseldot_paint(scm, cx + cells[i].w / 2, top + (int)ih + gap + r, r);
    if (ic)
      drw_pic(drw, cx + ((int)cells[i].w - (int)iw) / 2, top, iw, ih, ic);
    /* the gap after a cell belongs to it, so clicks tile the pill */
    addslot(m, cx, cells[i].w + (int)tabgap, ClkWinTitle, (Arg){.v = c});
  }
}

/* re-arm or dismiss the hover state from the pointer position: a tag with a
   preview, or a client tab; called from enter- and motion-notify */
static void hoverupdate(Monitor *m, int x) {
  const BarSlot *s = barslotat(m, x);
  Client *tc;
  int ti;

  if (s && s->click == ClkTagBar && (ti = tagindex(s->arg.ui)) >= 0 &&
      m->tagmap[ti] && !(m->tagset[m->seltags] & 1 << ti)) {
    if (m->previewshow != ti + 1) {
      hoverhide();
      hoverarm = 1;
      hoveridx = ti;
      hoverstart = hovernow();
    }
    return;
  }
  if (s && s->click == ClkWinTitle && (tc = (Client *)s->arg.v)) {
    if (tc != hoverc) {
      hoverhide();
      hoverarm = 1;
      hoverc = tc;
      hoverx = s->x;
      hoverw = s->w;
      hoverstart = hovernow();
    }
  } else {
    hoverhide();
  }
}

static uint64_t hovernow(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static void hoverfire(void) {
  if (!hoverarm)
    return;
  hoverarm = 0;
  if (hoveridx >= 0) {
    selmon->previewshow = hoveridx + 1;
    showtagpreview(hoveridx);
  } else if (hoverc)
    hovershow(hoverc, hoverx);
}

static void hoverhide(void) {
  hoverarm = 0;
  hoverc = NULL;
  hoveridx = -1;
  if (toolwin != None)
    XUnmapWindow(dpy, toolwin);
  if (selmon && selmon->tagwin != None && selmon->previewshow) {
    selmon->previewshow = 0;
    XUnmapWindow(dpy, selmon->tagwin);
  }
}

/* largest byte index <= n that ends a complete UTF-8 character */
static int utf8boundary(const char *s, int n) {
  int i = n;
  while (i > 0) {
    unsigned char c = (unsigned char)s[i - 1];
    int len;

    if (c < 0x80)
      return i;
    if ((c & 0xE0) == 0xC0)
      len = 2;
    else if ((c & 0xF0) == 0xE0)
      len = 3;
    else if ((c & 0xF8) == 0xF0)
      len = 4;
    else { /* continuation byte: keep walking back */
      i--;
      continue;
    }
    if (i - 1 + len <= n) /* lead byte with all its bytes within [0, n) */
      return i - 1 + len;
    i--; /* truncated lead byte: drop the whole character */
  }
  return 0;
}

/* shorten src to fit maxw, appending an ellipsis; never splits UTF-8 */
static void hoverellipsize(const char *src, char *buf, size_t bufsz, int maxw) {
  char t[512];
  int n = MIN((int)strlen(src), (int)sizeof(t) - 4); /* room for ellipsis */

  if (drw_fontset_getwidth(drw, src) <= maxw) {
    strncpy(buf, src, bufsz - 1);
    buf[bufsz - 1] = '\0';
    return;
  }
  for (; n > 1; n = utf8boundary(src, n - 1)) {
    int w;

    strncpy(t, src, n);
    t[n] = '\0';
    strcat(t, "…");
    w = drw_fontset_getwidth(drw, t);
    if (w <= maxw) {
      strncpy(buf, t, bufsz - 1);
      buf[bufsz - 1] = '\0';
      return;
    }
  }
  strncpy(buf, src, bufsz - 1);
  buf[bufsz - 1] = '\0';
}

/* scale factor fitting a sw x sh region into the preview area: at most
   previewh tall, at most the monitor width minus hoverpad wide */
static double previewscale(Monitor *m, int sw, int sh) {
  double scale = MIN((double)previewh / MAX(sh, 1), 1.0);
  return MIN(scale, (double)(m->mw - (int)hoverpad * 2) / MAX(sw, 1));
}

/* preview area size for a sw x sh source, using the live-tooltip scale */
static void previewsize(Monitor *m, int sw, int sh, int *pw, int *ph) {
  double scale = previewscale(m, sw, sh);

  *pw = MAX(1, (int)(sw * scale));
  *ph = MAX(1, (int)(sh * scale));
}

/* show the hover tooltip for client c, centered above/below the tab at tx */
static void hovershow(Client *c, int tx) {
  int iw, lh, y, gx, gy, pw, ph;
  unsigned int tw, th;
  char titlebuf[512];
  const char *title;
  Monitor *m = c->mon;
  XSetWindowAttributes wa = {.override_redirect = True,
                             .background_pixel = 0,
                             .border_pixel = 0,
                             .colormap = cmap,
                             .event_mask = NoEventMask};

  if (!m->showbar || !hoverinfo)
    return;
  lh = drw->fonts->h;
  title = c->name;

  /* preview area: the same scale for visible and hidden clients, so the
     tooltip is the same size either way; a hidden client shows the snapshot
     cached at hide time. Scale by height, then by width so the preview never
     exceeds the monitor; wide-but-short windows would otherwise overflow */
  previewsize(m, c->w, c->h, &pw, &ph);

  /* width follows the preview; a long title is ellipsized to fit */
  tw = pw + hoverpad * 2;
  th = hoverpad * 2 + ph + hovergap + lh;

  gx = m->wx + sp + tx + hoverw / 2 - (int)tw / 2;
  if (gx < m->mx) /* left overflow: anchor to the tab's right edge instead */
    gx = MIN(m->wx + sp + tx + hoverw, m->mx + m->mw - (int)tw);
  gx = MAX(m->mx, MIN(gx, m->mx + m->mw - (int)tw));
  gy = m->topbar ? m->by + vp + bh + 2 : m->by + vp - (int)th - 2;
  gy = MAX(0, MIN(gy, sh - (int)th));

  if (!toolwin) {
    toolwin = XCreateWindow(dpy, root, gx, gy, tw, th, 0, depth, InputOutput,
                            visual, CWOverrideRedirect | CWBackPixel |
                                CWBorderPixel | CWColormap | CWEventMask,
                            &wa);
    XStoreName(dpy, toolwin, "dwm-tooltip");
    XChangeProperty(dpy, toolwin, netatom[NetWMWindowType], XA_ATOM, 32,
                    PropModeReplace,
                    (unsigned char *)&netatom[NetWMWindowTypeTooltip], 1);
    tooldrw = drw_create(dpy, screen, toolwin, tw, th, visual, depth, cmap);
  }

  drw_resize(tooldrw, tw, th);
  XMoveResizeWindow(dpy, toolwin, gx, gy, tw, th);
  drw_setscheme(tooldrw, scheme[SchemeTooltip]);
  drw_rect(tooldrw, 0, 0, tw, th, 1, 1); /* solid background */

  iw = tw - hoverpad * 2;
  hoverellipsize(title, titlebuf, sizeof(titlebuf), iw);
  y = hoverpad;
  if (fonts_highlight_set)
    drw_setfontset(tooldrw, fonts_highlight_set);
  drw_text(tooldrw, hoverpad, y, iw, lh, 0, titlebuf, 0, 0);
  drw_setfontset(tooldrw, fonts_set);

  prevpx = hoverpad;
  prevpy = hoverpad + lh + hovergap;
  prevw = pw;
  prevh = ph;
  hoverpreview(c, prevpx, prevpy, prevw, prevh);

  /* map before copying: draws on an unmapped window are discarded on the
     first map, which would show an empty tooltip; the server processes
     map + copy in order, so no flash (same as showtagpreview) */
  XMapRaised(dpy, toolwin);
  drw_map(tooldrw, toolwin, 0, 0, tw, th);
  XFlush(dpy);
}

/* build the XRender scale transform; destination coords map to source
   coords (verified empirically: dst pixel p samples src pixel T(p)), so
   the scale is source/destination, not destination/source */
static void scaletransform(XTransform *tr, int sw, int dw, int sh, int dh) {
  tr->matrix[0][0] = XDoubleToFixed((double)sw / dw);
  tr->matrix[0][1] = 0;
  tr->matrix[0][2] = 0;
  tr->matrix[1][0] = 0;
  tr->matrix[1][1] = XDoubleToFixed((double)sh / dh);
  tr->matrix[1][2] = 0;
  tr->matrix[2][0] = 0;
  tr->matrix[2][1] = 0;
  tr->matrix[2][2] = 1 << 16;
}

/* set while a snapshot capture runs; the handler only records, XSync
   collects */
static int snapfail;

static int snaperror(Display *d, XErrorEvent *e) {
  (void)d; (void)e;
  snapfail = 1;
  return 0;
}

/* capture c's visible contents into the hidden-snapshot cache, scaled to
   the live-tooltip size so it exactly fills its box; the frame is still
   mapped at this point (hidewin calls this before unmapping). A failed
   capture is discarded and the previous snapshot is kept. */
static void takesnapshot(Client *c) {
  XWindowAttributes wa, fwa;
  XRenderPictFormat *fmt;
  int pw, ph;
  Pixmap pm;
  Picture src, dst;
  XTransform tr;

  if (!c || !c->mon)
    return;
  /* only a currently viewable frame holds readable contents */
  if (!XGetWindowAttributes(dpy, c->frame, &fwa) ||
      fwa.map_state != IsViewable)
    return;
  if (!XGetWindowAttributes(dpy, c->win, &wa) || wa.class != InputOutput ||
      !(fmt = XRenderFindVisualFormat(dpy, wa.visual)))
    return;

  /* same size as the live tooltip (which is sized from the frame), so the
     snapshot exactly fills its box; the client itself is the source, scaled
     to fit, just like the live path */
  previewsize(c->mon, c->w, c->h, &pw, &ph);

  snapfail = 0;
  XSetErrorHandler(snaperror);
  pm = XCreatePixmap(dpy, root, pw, ph, wa.depth);
  dst = XRenderCreatePicture(dpy, pm, fmt, 0, NULL);
  src = XRenderCreatePicture(dpy, c->win, fmt, 0, NULL);
  XRenderSetPictureFilter(dpy, src, FilterGood, NULL, 0);
  scaletransform(&tr, wa.width, pw, wa.height, ph);
  XRenderSetPictureTransform(dpy, src, &tr);
  XRenderComposite(dpy, PictOpSrc, src, None, dst, 0, 0, wa.width, wa.height,
                   0, 0, pw, ph);
  XRenderFreePicture(dpy, src);
  XSync(dpy, False);
  if (snapfail) { /* keep the old snapshot; drop the broken attempt */
    XRenderFreePicture(dpy, dst);
    XFreePixmap(dpy, pm);
  }
  XSetErrorHandler(xerror);
  if (snapfail)
    return;
  freehspic(c);
  c->hspm = pm;
  c->hspic = dst;
  c->hspw = pw;
  c->hsph = ph;
}

/* composite a live scaled snapshot of c into the preview area (x, y, w, h);
   hidden clients show the snapshot cached at hide time, then fall back to
   an icon or a placeholder */
static void hoverpreview(Client *c, int x, int y, int w, int h) {
  if (!c || !tooldrw)
    return;

  if (HIDDEN(c)) {
    if (c->hspic && c->hspw && c->hsph && (unsigned int)w >= c->hspw &&
        (unsigned int)h >= c->hsph) {
      /* last visible frame, cached at hide time, centered */
      drw_pic(tooldrw, x + (w - (int)c->hspw) / 2, y + (h - (int)c->hsph) / 2,
              c->hspw, c->hsph, c->hspic);
      return;
    }
    if (c->icon && w >= c->icw + 8 && h >= c->ich + 8) {
      drw_pic(tooldrw, x + (w - c->icw) / 2, y + (h - c->ich) / 2, c->icw,
              c->ich, c->icon);
      return;
    }
    drw_text(tooldrw, x, y + (h - drw->fonts->h) / 2, w, drw->fonts->h, 0,
             "no preview", 0, 0);
    return;
  }

  /* inset the snapshot by previewborder so the highlight border drawn
     around the preview area sits on the background, not over the image */
  int ix = x + (int)previewborderpx, iy = y + (int)previewborderpx,
      iw2 = MAX(1, w - (int)previewborderpx * 2),
      ih2 = MAX(1, h - (int)previewborderpx * 2), i;
  XWindowAttributes wa;
  XRenderPictFormat *fmt;
  Picture pic;
  XTransform tr;

  /* an InputOnly window has no contents to sample at all */
  if (!XGetWindowAttributes(dpy, c->win, &wa) || wa.class != InputOutput ||
      !(fmt = XRenderFindVisualFormat(dpy, wa.visual))) {
    drw_text(tooldrw, x, y + (h - drw->fonts->h) / 2, w, drw->fonts->h, 0,
             "no preview", 0, 0);
    return;
  }

  /* Sample the client window directly: XCompositeNameWindowPixmap needs the
     compositor to have redirected that exact window (picom may skip one,
     e.g. Firefox) and misses GTK child-window contents, while a window
     picture covers the whole tree. A preview is cosmetic, so swallow any
     error here instead of letting it take down the WM. */
  XSetErrorHandler(xerrordummy);
  pic = XRenderCreatePicture(dpy, c->win, fmt, 0, NULL);
  XRenderSetPictureFilter(dpy, pic, FilterGood, NULL, 0);
  scaletransform(&tr, wa.width, iw2, wa.height, ih2);
  XRenderSetPictureTransform(dpy, pic, &tr);
  /* the source rect must be the full source image (wa.width x wa.height);
     the transform maps it onto the destination rect (iw2 x ih2).
     PictOpOver blends over the black background: PictOpSrc would copy
     the client's alpha channel straight through, so a translucent client
     (e.g. a semi-transparent terminal) would punch a hole through the
     tooltip instead of showing against it */
  XRenderComposite(dpy, PictOpOver, pic, None, tooldrw->picture, 0, 0,
                   wa.width, wa.height, ix, iy, iw2, ih2);
  XRenderFreePicture(dpy, pic);
  XSync(dpy, False);
  XSetErrorHandler(xerror);
  /* highlight border in colors[x][2]; drw_rect has no border channel
     (filled=0 draws with the fg color), so draw it directly */
  XSetForeground(dpy, tooldrw->gc, scheme[SchemeSel][ColBorder].pixel);
  for (i = 0; i < (int)previewborderpx && w - i * 2 > 0 && h - i * 2 > 0; i++)
    XDrawRectangle(dpy, tooldrw->drawable, tooldrw->gc, x + i, y + i,
                   w - 1 - i * 2, h - 1 - i * 2);
}

/* refresh the live preview while the tooltip is shown */
static void hoverrefresh(void) {
  Client *c = hoverc;

  if (!c || toolwin == None)
    return;
  hoverpreview(c, prevpx, prevpy, prevw, prevh);
  drw_map(tooldrw, toolwin, 0, 0, tooldrw->w, tooldrw->h);
}

/* show the scaled snapshot of tag i in the tagwin below the bar, laid
   out like the hover tooltip: snapshot inset by previewborder, padding
   hoverpad, highlight border on the padding edge */
void showtagpreview(unsigned int i) {
  unsigned int tw, th, bw, dep, ww, wh;
  Window r;
  int x, y, n, iw2, ih2;
  XWindowAttributes wa;

  if (selmon->tagwin == None)
    return;
  if (!selmon->previewshow || !selmon->tagmap[i]) {
    XUnmapWindow(dpy, selmon->tagwin);
    return;
  }
  if (!XGetGeometry(dpy, selmon->tagmap[i], &r, &x, &y, &tw, &th, &bw, &dep))
    return;
  ww = tw + (unsigned int)hoverpad * 2;
  wh = th + (unsigned int)hoverpad * 2;
  /* plain background so a resize fills exposed pixels with the tooltip
     color, never a snapshot tile */
  XSetWindowBackground(dpy, selmon->tagwin,
                       scheme[SchemeTooltip][ColBg].pixel);
  if (XGetWindowAttributes(dpy, selmon->tagwin, &wa) &&
      ((unsigned)wa.width != ww || (unsigned)wa.height != wh))
    XMoveResizeWindow(dpy, selmon->tagwin, selmon->wx + sp,
                      selmon->by + vp + bh, ww, wh);
  /* map before drawing: draws on an unmapped window are discarded on
     the first map, which would show an empty preview; the server
     processes map + copy in order, so no flash */
  XMapRaised(dpy, selmon->tagwin);
  /* inset the snapshot by previewborder, like hoverpreview, so the
     highlight border below wraps it on all four sides */
  iw2 = MAX(1, (int)tw - (int)previewborderpx * 2);
  ih2 = MAX(1, (int)th - (int)previewborderpx * 2);
  XCopyArea(dpy, selmon->tagmap[i], selmon->tagwin, drw->gc, 0, 0, iw2, ih2,
            (int)hoverpad + (int)previewborderpx,
            (int)hoverpad + (int)previewborderpx);
  /* XDrawRectangle's w/h span to x+w inclusive; use the snapshot area
     (tw x th) so the border exactly wraps the inset image */
  XSetForeground(dpy, drw->gc, scheme[SchemeSel][ColBorder].pixel);
  for (n = 0; n < (int)previewborderpx && tw - n * 2 > 0 && th - n * 2 > 0; n++)
    XDrawRectangle(dpy, selmon->tagwin, drw->gc, hoverpad + n, hoverpad + n,
                   tw - 1 - n * 2, th - 1 - n * 2);
  XSync(dpy, False);
}

/* toggle the preview of tag arg->ui (0-based index) */
void previewtag(const Arg *arg) {
  if (selmon->tagwin == None)
    return;
  if (selmon->previewshow == arg->ui + 1) {
    selmon->previewshow = 0;
  } else if (selmon->tagmap[arg->ui]) { /* no snapshot, nothing to show */
    selmon->previewshow = arg->ui + 1;
  }
  showtagpreview(arg->ui);
}

/* render the scaled snapshot of the captured screen region img into
   m->tagmap[i] (a dw x dh pixmap); the caller owns img and frees it */
static void previewtagshot(Monitor *m, unsigned int i, XImage *img, int dw,
                           int dh) {
  Pixmap full;
  GC gc;
  XRenderPictFormat *fmt;
  Picture src = None, dst = None;
  XTransform tr;

  full = XCreatePixmap(dpy, m->tagwin, img->width, img->height, img->depth);
  if (!full) {
    return;
  }
  /* drw->gc belongs to the alpha-depth drawable; PutImage needs a GC
     matching the source pixmap depth (root may be 24-bit while the
     bar visual is 32-bit), so use a scratch GC */
  gc = XCreateGC(dpy, full, 0, NULL);
  XPutImage(dpy, full, gc, img, 0, 0, 0, 0, img->width, img->height);
  XFreeGC(dpy, gc);
  m->tagmap[i] = XCreatePixmap(dpy, m->tagwin, dw, dh, depth);
  if (!m->tagmap[i]) {
    XFreePixmap(dpy, full);
    return;
  }
  /* the source pixmap depth follows the root framebuffer (which may
     differ from the alpha visual depth), so match the format to it */
  fmt = XRenderFindStandardFormat(
      dpy, img->depth == 32 ? PictStandardARGB32 : PictStandardRGB24);
  if (fmt)
    src = XRenderCreatePicture(dpy, full, fmt, 0, NULL);
  if (src)
    dst = XRenderCreatePicture(dpy, m->tagmap[i],
                               XRenderFindVisualFormat(dpy, visual), 0, NULL);
  if (src && dst) {
    XRenderSetPictureFilter(dpy, src, FilterGood, NULL, 0);
    scaletransform(&tr, img->width, dw, img->height, dh);
    XRenderSetPictureTransform(dpy, src, &tr);
    /* the source rect is the full captured image; the transform maps it
       onto the scaled destination. PictOpSrc copies the pixels verbatim
       so alpha bytes from the framebuffer can't punch holes */
    XRenderComposite(dpy, PictOpSrc, src, None, dst, 0, 0, img->width,
                     img->height, 0, 0, dw, dh);
  } else
    fprintf(stderr, "dwm: XRender failed for tag preview\n");
  if (src)
    XRenderFreePicture(dpy, src);
  if (dst)
    XRenderFreePicture(dpy, dst);
  XFreePixmap(dpy, full);
}

/* capture scaled snapshots of the current view for all occupied tags,
   so hovering a tag later re-views its last layout */
void takepreview(void) {
  Client *c;
  unsigned int occ = 0, i;
  int dw, dh;
  XImage *img;

  hoverhide(); /* keep the tooltip and tag preview out of the shot */
  XSync(dpy, False);

  for (c = selmon->clients; c; c = c->next)
    occ |= c->tags;

  /* snapshot the whole monitor including the bar once; every occupied
     tag scales the same source image, so capture it a single time */
  img = XGetImage(dpy, root, selmon->mx, selmon->my, selmon->mw, selmon->mh,
                  AllPlanes, ZPixmap);
  if (!img) {
    fprintf(stderr, "dwm: XGetImage failed for tag preview\n");
    return;
  }
  previewsize(selmon, selmon->mw, selmon->mh, &dw, &dh);

  for (i = 0; i < LENGTH(tags); i++) {
    /* only tags that are occupied and part of the current view */
    if (!(occ & 1 << i) || !(selmon->tagset[selmon->seltags] & 1 << i))
      continue;

    if (selmon->tagmap[i]) { /* tagmap exists, clean it */
      XFreePixmap(dpy, selmon->tagmap[i]);
      selmon->tagmap[i] = 0;
    }

    previewtagshot(selmon, i, img, dw, dh);
  }
  XDestroyImage(img);
}

void enternotify(XEvent *e) {
  Client *c;
  Monitor *m;
  XCrossingEvent *ev = &e->xcrossing;

  if (ev->mode != NotifyNormal && ev->window != root)
    return;
  c = wintoclient(ev->window);
  m = c ? c->mon : wintomon(ev->window);
  /* entering a sub-window of a client on the current monitor must not
   * steal focus, but a pointer crossing into another monitor must follow */
  if (ev->detail == NotifyInferior && ev->window != root && m == selmon)
    return;
  if (m != selmon) {
    unfocus(selmon->sel, 1);
    selmon = m;
    focus(c);
  }
  if (ev->window == selmon->barwin && hoverinfo)
    hoverupdate(selmon, ev->x);
}

void leavenotify(XEvent *e) {
  XCrossingEvent *ev = &e->xcrossing;

  if (ev->window == selmon->barwin)
    hoverhide();
}

void expose(XEvent *e) {
  Monitor *m;
  Client *c;
  XExposeEvent *ev = &e->xexpose;

  if (ev->count == 0 && (c = frameclient(ev->window))) {
    drawtitle(c);
    return;
  }
  if (ev->count == 0 && (m = wintomon(ev->window))) {
    drawbar(m);

    if (showsystray && m == systraytomon(m))
      updatesystray(0);
  }
}

void focus(Client *c) {
  if (!c || !ISVISIBLE(c))
    for (c = selmon->stack; c && (!ISVISIBLE(c) || HIDDEN(c)); c = c->snext);
  if (selmon->sel && selmon->sel != c) {
    unfocus(selmon->sel, 0);

    if (selmon->hidsel) {
      hidewin(selmon->sel);
      if (c)
        arrange(c->mon);
      selmon->hidsel = 0;
    }
  }
  if (c) {
    if (c->mon != selmon)
      selmon = c->mon;
    if (c->isurgent)
      seturgent(c, 0);
    detachstack(c);
    attachstack(c);
    grabbuttons(c, 1);
    if (c->frame != None)
      XSetWindowBorder(dpy, c->frame, scheme[SchemeSel][ColBorder].pixel);
    setfocus(c);
  } else {
    XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
  }
  selmon->sel = c;
  if (c && focusclient(c->mon))
    arrange(c->mon); /* keep the focused window in the single master slot */
  drawbars();
  drawtitles();
}

/* there are some broken focus acquiring clients needing extra handling */
void focusin(XEvent *e) {
  XFocusChangeEvent *ev = &e->xfocus;

  hoverhide(); /* focus left the bar: dismiss any open tooltip */
  if (selmon->sel && ev->window != selmon->sel->win)
    setfocus(selmon->sel);
}

void setcurrentmon(Monitor *m) {
  Window root_return, child_return;
  int root_x, root_y, win_x, win_y;
  unsigned int mask;

  if (m == selmon)
    return;
  unfocus(selmon->sel, 0);

  if (XQueryPointer(dpy, root, &root_return, &child_return, &root_x,
                      &root_y, &win_x, &win_y, &mask)) {
    /* remember old monitor pointer position */
    if (INMON(selmon, root_x, root_y)) {
      selmon->last_mouse_pos[0] = root_x;
      selmon->last_mouse_pos[1] = root_y;
    } else {
      selmon->last_mouse_pos[0] = selmon->mx + selmon->mw/2;
      selmon->last_mouse_pos[1] = selmon->my + selmon->mh/2;
    }
    /* warp when pointer is outside target monitor */
    if (!INMON(m, root_x, root_y)) {
      int tx = m->last_mouse_pos[0];
      int ty = m->last_mouse_pos[1];
      if (!INMON(m, tx, ty)) {
        tx = m->mx + m->mw/2;
        ty = m->my + m->mh/2;
      }
      XWarpPointer(dpy, None, root, 0, 0, 0, 0, tx, ty);
    }
  }
  selmon = m;
  focus(NULL);
  updatecurrentdesktop();
}

void focusmon(const Arg *arg) {
  setcurrentmon(dirtomon(arg->i));
}

void focusstackvis(const Arg *arg) { focusstack(arg->i, 0); }

void focusstackhid(const Arg *arg) { focusstack(arg->i, 1); }

void focusstack(int inc, int hid) {
  Client *c = NULL, *i;

  if ((!selmon->sel && !hid) ||
      (selmon->sel && selmon->sel->isfullscreen && lockfullscreen))
    return;
  if (!selmon->clients)
    return;

  if (inc > 0) {
    if (selmon->sel)
      for (c = selmon->sel->next; c && (!ISVISIBLE(c) || (!hid && HIDDEN(c)));
           c = c->next)
        ;
    if (!c)
      for (c = selmon->clients; c && (!ISVISIBLE(c) || (!hid && HIDDEN(c)));
           c = c->next)
        ;
  } else {
    if (selmon->sel) {
      for (i = selmon->clients; i != selmon->sel; i = i->next)
        if (ISVISIBLE(i) && !(!hid && HIDDEN(i)))
          c = i;
    } else
      c = selmon->clients;
    if (!c)
      for (; i; i = i->next)
        if (ISVISIBLE(i) && !(!hid && HIDDEN(i)))
          c = i;
  }
  if (c) {
    i = selmon->sel;
    focus(c);
    if (i) updateicon(i);
    updateicon(c);
    restack(selmon);

    if (HIDDEN(c)) {
      showwin(c);
      if (!autoshowhid)
        c->mon->hidsel = 1;
    }
  }
}

Atom getatomprop(Client *c, Atom prop) {
  int format;
  unsigned long nitems,dl;
  unsigned char *p = NULL;
  Atom da, atom = None;

  /* FIXME getatomprop should return the number of items and a pointer to
   * the stored data instead of this workaround */
  Atom req = XA_ATOM;
  if (prop == xatom[XembedInfo])
    req = xatom[XembedInfo];

  if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
                         &da, &format, &nitems, &dl, &p) == Success && p) {
    if (nitems > 0 && format == 32)
      atom = *(long *)p;
    if (da == xatom[XembedInfo] && dl == 2)
      atom = ((Atom *)p)[1];
    XFree(p);
  }
  return atom;
}

static uint32_t prealpha(uint32_t p, uint32_t custom_alpha) {
  // min(pixel alpha, custom alpha)
  uint8_t a = MIN(p >> 24u, custom_alpha);
  uint32_t rb = (a * (p & 0xFF00FFu)) >> 8u;
  uint32_t g = (a * (p & 0x00FF00u)) >> 8u;
  return (rb & 0xFF00FFu) | (g & 0x00FF00u) | (a << 24u);
}

Picture geticonprop(Window win, unsigned int *picw, unsigned int *pich,
                    unsigned int alpha, int target) {
  int format;
  unsigned long n, extra, *p = NULL;
  Atom real;

  *picw = *pich = 0;
  if (XGetWindowProperty(dpy, win, netatom[NetWMIcon], 0L, LONG_MAX, False, AnyPropertyType,
               &real, &format, &n, &extra, (unsigned char **)&p) != Success)
    return None;
  if (n == 0 || format != 32) { XFree(p); return None; }

  unsigned long *bstp = NULL;
  uint32_t w, h, sz;
  {
    unsigned long *i; const unsigned long *end = p + n;
    uint32_t bstd = UINT32_MAX, d, m;
    for (i = p; i < end - 1; i += sz) {
      if ((w = *i++) >= 16384 || (h = *i++) >= 16384) { XFree(p); return None; }
      if ((sz = w * h) > end - i) break;
      if ((m = w > h ? w : h) >= (uint32_t)target && (d = m - target) < bstd) {
        bstd = d;
        bstp = i;
      }
    }
    if (!bstp) {
      for (i = p; i < end - 1; i += sz) {
        if ((w = *i++) >= 16384 || (h = *i++) >= 16384) { XFree(p); return None; }
        if ((sz = w * h) > end - i) break;
        if ((d = target - (w > h ? w : h)) < bstd) {
          bstd = d;
          bstp = i;
        }
      }
    }
    if (!bstp) { XFree(p); return None; }
  }

  if ((w = *(bstp - 2)) == 0 || (h = *(bstp - 1)) == 0) { XFree(p); return None; }

  uint32_t icw, ich;
  if (w <= h) {
    ich = target; icw = w * target / h;
    if (icw == 0) icw = 1;
  }
  else {
    icw = target; ich = h * target / w;
    if (ich == 0) ich = 1;
  }
  *picw = icw; *pich = ich;

  uint32_t i, *bstp32 = (uint32_t *)bstp;
  for (sz = w * h, i = 0; i < sz; ++i) bstp32[i] = prealpha(bstp[i], alpha);

  Picture ret = drw_picture_create_resized(drw, (char *)bstp, w, h, icw, ich);
  XFree(p);

  return ret;
}

int getrootptr(int *x, int *y) {
  int di;
  unsigned int dui;
  Window dummy;

  return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long getstate(Window w) {
  int format;
  long result = -1;
  unsigned char *p = NULL;
  unsigned long n, extra;
  Atom real;

  if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False,
                         wmatom[WMState], &real, &format, &n, &extra, &p) != Success)
    return -1;
  if (n != 0 && format == 32)
    result = *(long *)p;
  XFree(p);
  return result;
}

/* read a saved hot-restart state property (list of longs) and remove it */
long getstateprop(Window w, const char *name, long *out, long maxvals) {
  Atom a = XInternAtom(dpy, name, False);
  Atom actual;
  int fmt;
  unsigned long n, extra;
  unsigned char *data = NULL;
  long i, nread = 0;

  if (XGetWindowProperty(dpy, w, a, 0, maxvals, False, XA_CARDINAL,
                         &actual, &fmt, &n, &extra, &data) != Success || !data || fmt != 32)
    goto cleanup;
  nread = MIN(n, maxvals);
  for (i = 0; i < nread; i++)
    out[i] = (long)((long *)data)[i];
cleanup:
  if (data)
    XFree(data);
  XDeleteProperty(dpy, w, a);
  return nread;
}

unsigned int getsystraywidth() {
  unsigned int w = 0;
  Client *i;
  if (showsystray)
    for (i = systray->icons; i; w += i->w + systrayspacing, i = i->next)
      ;
  return w ? w + systrayspacing : 0;
}

static int trayrank(const char *class) {
  unsigned int i;
  for (i = 0; systrayorder[i]; i++)
    if (strcasecmp(class, systrayorder[i]) == 0)
      return i;
  /* unlisted: find "..." slot, otherwise end */
  for (i = 0; systrayorder[i]; i++)
    if (strcmp(systrayorder[i], "...") == 0)
      return i;
  return i;
}

int gettextprop(Window w, Atom atom, char *text, unsigned int size) {
  char **list = NULL;
  int n;
  XTextProperty name;

  if (!text || size == 0)
    return 0;
  text[0] = '\0';
  if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
    return 0;
  if (name.encoding == XA_STRING)
    strncpy(text, (char *)name.value, size - 1);
  else {
    if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 &&
        *list) {
      strncpy(text, *list, size - 1);
      XFreeStringList(list);
    }
  }
  text[size - 1] = '\0';
  XFree(name.value);
  return 1;
}

void grabbuttons(Client *c, int focused) {
  updatenumlockmask();
  {
    unsigned int i, j;
    unsigned int modifiers[] = {0, LockMask, numlockmask,
                                numlockmask | LockMask};
    XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
    /* NOTE: never grab on c->frame. The frame is our own window, so title
       clicks already reach us via its ButtonPressMask; a passive grab there
       would also swallow every click inside the client (ancestor grabs
       intercept descendant presses) and break all in-client mouse input. */
    if (!focused)
      XGrabButton(dpy, AnyButton, AnyModifier, c->win, False, BUTTONMASK,
                  GrabModeSync, GrabModeSync, None, None);
    for (i = 0; i < LENGTH(buttons); i++)
      if (buttons[i].click == ClkClientWin)
        for (j = 0; j < LENGTH(modifiers); j++)
          XGrabButton(dpy, buttons[i].button, buttons[i].mask | modifiers[j],
                      c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync,
                      None, None);
  }
}

void grabkeys(void) {
  updatenumlockmask();
  {
    unsigned int i, j;
    unsigned int modifiers[] = {0, LockMask, numlockmask,
                                numlockmask | LockMask};
    KeyCode code;

    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    for (i = 0; i < LENGTH(keys); i++)
      if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
        for (j = 0; j < LENGTH(modifiers); j++)
          XGrabKey(dpy, code, keys[i].mod | modifiers[j], root, True,
                   GrabModeAsync, GrabModeAsync);
  }
}

void hideclient(Client *c) {
  if (!c || HIDDEN(c))
    return;

  hidewin(c);
  if (c == c->mon->sel)
    focus(NULL);
  arrange(c->mon);
}

void hide(const Arg *arg) {
  (void)arg;
  hideclient(selmon->sel);
}

void hidewin(Client *c) {
  if (!c || HIDDEN(c))
    return;

  takesnapshot(c);
  /* hiding unmaps the frame only; the client stays mapped inside it.
     Frame unmaps are ignored by unmapnotify, so no event juggling needed. */
  XUnmapWindow(dpy, c->frame);
  c->hidden = 1;
  setclientstate(c, IconicState);
  // refresh tab icon after IconicState; alpha follows client state
  updateicon(c);
}

void incnmaster(const Arg *arg) {
  selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] =
      MAX(selmon->nmaster + arg->i, 0);
  arrange(selmon);
}

#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo *unique, size_t n,
                        XineramaScreenInfo *info) {
  while (n--)
    if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org &&
        unique[n].width == info->width && unique[n].height == info->height)
      return 0;
  return 1;
}
#endif /* XINERAMA */

void keypress(XEvent *e) {
  unsigned int i;
  KeySym keysym;
  XKeyEvent *ev;

  ev = &e->xkey;
  keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
  for (i = 0; i < LENGTH(keys); i++)
    if (keysym == keys[i].keysym &&
        CLEANMASK(keys[i].mod) == CLEANMASK(ev->state) && keys[i].func)
      keys[i].func(&(keys[i].arg));
}

void killclient(const Arg *arg) {
  if (!selmon->sel)
    return;

  if (selmon->hidsel) selmon->hidsel = 0;

  if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask,
                 wmatom[WMDelete], CurrentTime, 0, 0, 0)) {
    XGrabServer(dpy);
    XSetErrorHandler(xerrordummy);
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, selmon->sel->win);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XUngrabServer(dpy);
  }
}

/* copy an "#RRGGBB" resource value into dest; invalid values are ignored */
static void xrdb_color(XrmDatabase xrdb, const char *name, char *dest) {
  char *type;
  XrmValue value;

  if (XrmGetResource(xrdb, name, NULL, &type, &value) != True ||
      !ishexcolor(value.addr))
    return;
  strncpy(dest, value.addr, 7);
  dest[7] = '\0';
}

void loadxrdb() {
  XrmDatabase xrdb;
  char *resm = XResourceManagerString(dpy);

  if (resm == NULL)
    return;
  xrdb = XrmGetStringDatabase(resm);
  if (xrdb == NULL)
    return;
  xrdb_color(xrdb, "dwm.col_black", col_black);
  xrdb_color(xrdb, "dwm.col_red", col_red);
  xrdb_color(xrdb, "dwm.col_green", col_green);
  xrdb_color(xrdb, "dwm.col_yellow", col_yellow);
  xrdb_color(xrdb, "dwm.col_blue", col_blue);
  xrdb_color(xrdb, "dwm.col_magenta", col_magenta);
  xrdb_color(xrdb, "dwm.col_cyan", col_cyan);
  xrdb_color(xrdb, "dwm.col_white", col_white);
  XrmDestroyDatabase(xrdb);
}


void layoutmenu(const Arg *arg) {
  FILE *p;
  char c[16], *s;
  int i;

  if (!(p = popen(layoutmenu_cmd, "r")))
    return;
  s = fgets(c, sizeof(c), p);
  pclose(p);

  if (!s || *s == '\0' || c[0] == '\0')
    return;

  i = atoi(c);
  setlayout(&((Arg){.v = &layouts[i]}));
}

void manage(Window w, XWindowAttributes *wa, int mapped) {
  Client *c, *t = NULL;
  Window trans = None;

  /* InputOnly windows have no drawable contents and must never become
     clients: there is nothing to frame, tab or sample from them */
  if (!wa || wa->class == InputOnly)
    return;

  c = ecalloc(1, sizeof(Client));
  c->win = w;
  /* geometry */
  c->x = c->oldx = wa->x;
  c->y = c->oldy = wa->y;
  c->w = c->oldw = wa->width;
  c->h = c->oldh = wa->height;
  c->oldbw = wa->border_width;
  c->cfact = 1.0;
  c->icon_alpha = 0;
  /* hidden state is only meaningful when adopting a pre-existing window (scan):
     a MapRequest is explicit intent to be visible, and unmanage() writes
     WithdrawnState itself, so trusting it there would leave the client hidden */
  c->hidden = !mapped && ISHIDDENSTATE(getstate(w));

  updatetitle(c);
  if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
    c->mon = t->mon;
    c->tags = t->tags;
    applyrules(c);   /* match class/border rules (e.g. borderrule bw) */
    c->mon = t->mon; /* ...but stay with the parent window */
    c->tags = t->tags;
  } else {
    c->mon = selmon;
    {
      long monnum;
      if (getstateprop(w, "_DWM_MONITOR", &monnum, 1)) {
        Monitor *m;
        for (m = mons; m && m->num != (int)monnum; m = m->next);
        if (m)
          c->mon = m;
      }
    }
    applyrules(c);
    {
      long tag;
      if (getstateprop(w, "_DWM_TAG", &tag, 1))
        c->tags = (unsigned int)tag;
    }
  }

  c->x = MAX(MIN(c->x ,c->mon->mx + c->mon->mw - WIDTH(c)), c->mon->mx);
  c->y = MAX(MIN(c->y, c->mon->my + c->mon->mh - HEIGHT(c)), c->mon->my);

  /* resolve titlebar opt-out (rule override + motif/gtk hints) first:
     the frame grows by titleh() below only when a title is wanted */
  updatenodecor(c);

  /* wa->width/height is the size the client asked for; the frame wraps it
     and the titlebar (when enabled) comes on top inside the frame */
  c->h = c->oldh = c->h + titleh(c);
  createframe(c);
  XSetWindowBorder(dpy, c->frame, scheme[SchemeNorm][ColBorder].pixel);
  updatewindowtype(c);
  updatesizehints(c);
  updatewmhints(c);
  XSelectInput(dpy, w, EnterWindowMask | FocusChangeMask |
               PropertyChangeMask | StructureNotifyMask);
  grabbuttons(c, 0);
  if (!c->isfloating)
    c->isfloating = c->oldstate = trans != None || c->isfixed;
  {
    long fl;
    if (getstateprop(w, "_DWM_FLOATING", &fl, 1))
      c->isfloating = c->oldstate = (int)fl;
  }
  /* _DWM_FULLSCREEN layout: [isfullscreen, oldx, oldy, oldw, oldh,
     oldstate, oldbw] — pre-fullscreen geometry lost on hot-restart,
     needed by setfullscreen() to restore the client on exit. */
  {
    long fstate[7];
    long n = getstateprop(w, "_DWM_FULLSCREEN", fstate, 7);
    if (n > 0 && fstate[0]) {
      c->isfullscreen = 1;
      if (n >= 5) {
        c->oldx = (int)fstate[1];
        c->oldy = (int)fstate[2];
        c->oldw = (int)fstate[3];
        c->oldh = (int)fstate[4];
      }
      if (n >= 6)
        c->oldstate = (int)fstate[5];
      if (n >= 7)
        c->oldbw = (int)fstate[6];
    }
  }
  if (c->isfloating) {
    c->x = (c->x == c->mon->mx || c->x + c->w == c->mon->mx + c->mon->mw)
            ? c->mon->mx+(c->mon->mw - c->w)/2 : c->x;
    c->y = (c->y == c->mon->my + bh || c->y == c->mon->my ||
            c->y + c->h == c->mon->my + c->mon->mh) ? c->mon->my+(c->mon->mh - c->h)/2 : c->y;
  }
  configure(c); /* propagates border_width, if size doesn't change */
  attachtop ? attach(c) : attachbottom(c);
  attachstack(c);
  updatewmdesktop(c);
  updateclientlist();
  if (!HIDDEN(c))
    setclientstate(c, NormalState);
  if (c->mon == selmon)
    unfocus(selmon->sel, 0);
  c->mon->sel = c;
  arrange(c->mon);
  if (!HIDDEN(c)) {
    XMapWindow(dpy, c->frame);
    XMapWindow(dpy, c->win);
    drawtitle(c);
  }
  focus(NULL);
}

void mappingnotify(XEvent *e) {
  XMappingEvent *ev = &e->xmapping;

  XRefreshKeyboardMapping(ev);
  if (ev->request == MappingKeyboard)
    grabkeys();
}

void maprequest(XEvent *e) {
  static XWindowAttributes wa;
  XMapRequestEvent *ev = &e->xmaprequest;

  Client *i, *c;
  if (showsystray && (i = wintosystrayicon(ev->window))) {
    sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
              XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
    updatesystray(1);
  }

  if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
    return;
  if (wa.class == InputOnly) {
    /* InputOnly windows carry no contents: there is nothing to frame, tab
       or preview, so just let them be mapped */
    XMapWindow(dpy, ev->window);
    return;
  }
  if ((c = wintoclient(ev->window))) {
    /* remap request for a managed client (e.g. tray show after Withdrawn) */
    if (!HIDDEN(c) && getstate(c->win) == NormalState)
      return;
    XMapWindow(dpy, c->frame);
    XMapWindow(dpy, ev->window);
    drawtitle(c);
    c->hidden = 0;
    setclientstate(c, NormalState);
    updateicon(c);
    arrange(c->mon);
    drawbars();
    return;
  }
  manage(ev->window, &wa, 1);
}

void monocle(Monitor *m) {
  unsigned int n = 0;
  int oh = 0, ov = 0, ih = 0, iv = 0;
  Client *c;
  /* inset by the outer gaps only when acting as the maximize (mode 2) host
     layout; plain [M] and fullscreen also run this layout but at mode 0 and
     should stay edge-to-edge */
  int gaps = (m->pertag->focusmaster[m->pertag->curtag] == 2);

  for (c = m->clients; c; c = c->next)
    if (ISVISIBLE(c))
      n++;
  if (n > 0) /* override layout symbol */
    snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
  if (gaps)
    getgaps(m, &oh, &ov, &ih, &iv, &n);
  for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
    resize(c, m->wx + ov, m->wy + oh, m->ww - 2 * ov - 2 * c->bw,
           m->wh - 2 * oh - 2 * c->bw, 0);
}

void motionnotify(XEvent *e) {
  static Monitor *mon = NULL;
  Monitor *m;
  XMotionEvent *ev = &e->xmotion;

  if (ev->window == selmon->barwin) {
    /* moving between tags/tabs while hovering */
    if (hoverinfo)
      hoverupdate(selmon, ev->x);
    return;
  }
  for (m = mons; m; m = m->next)
    if (ev->window == m->tagwin) {
      hoverhide(); /* touching the preview dismisses it */
      return;
    }
  if (ev->window != root)
    return;
  if (selmon->previewshow)
    hoverhide(); /* pointer left the bar */
  if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
    unfocus(selmon->sel, 1);
    selmon = m;
    focus(NULL);
  }
  mon = m;
}

void movemouse(const Arg *arg) {
  int x, y, ocx, ocy, nx, ny;
  Client *c;
  Monitor *m;
  XEvent ev;
  Time lasttime = 0;

  if (!(c = selmon->sel))
    return;
  if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
    return;
  restack(selmon);
  ocx = c->x;
  ocy = c->y;
  if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                   None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
    return;
  if (!getrootptr(&x, &y))
    return;
  do {
    XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
    switch (ev.type) {
    case ConfigureRequest:
    case Expose:
    case MapRequest:
      handler[ev.type](&ev);
      break;
    case MotionNotify:
      if ((ev.xmotion.time - lasttime) <= (1000 / refreshrate))
        continue;
      lasttime = ev.xmotion.time;

      nx = ocx + (ev.xmotion.x - x);
      ny = ocy + (ev.xmotion.y - y);
      if (abs(selmon->wx - nx) < snap)
        nx = selmon->wx;
      else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
        nx = selmon->wx + selmon->ww - WIDTH(c);
      if (abs(selmon->wy - ny) < snap)
        ny = selmon->wy;
      else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
        ny = selmon->wy + selmon->wh - HEIGHT(c);
      if (!c->isfloating && selmon->lt[selmon->sellt]->arrange &&
          (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
        togglefloatingclient(c);
      if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
        resize(c, nx, ny, c->w, c->h, 1);
      break;
    }
  } while (ev.type != ButtonRelease);
  XUngrabPointer(dpy, CurrentTime);
  if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
    sendmon(c, m);
    selmon = m;
    focus(NULL);
  }
}

Client *nexttiled(Client *c) {
  for (; c && (c->isfloating || !ISVISIBLE(c) || HIDDEN(c)); c = c->next)
    ;
  return c;
}

void pop(Client *c) {
  if (!c || !c->mon)
    return;
  detach(c);
  attach(c);
  focus(c);
  arrange(c->mon);
}

void propertynotify(XEvent *e) {
  Client *c;
  Window trans;
  XPropertyEvent *ev = &e->xproperty;

  if (showsystray && (c = wintosystrayicon(ev->window))) {
    int dirty = 0;
    if (ev->atom == XA_WM_NORMAL_HINTS) {
      updatesizehints(c);
      updatesystrayicongeom(c, c->w, c->h);
      dirty = 1;
    } else if (ev->atom == xatom[XembedInfo]) {
      updatesystrayiconstate(c, ev);
      dirty = 1;
    }
    if (dirty)
      updatesystray(1);
  }

  if ((ev->window == root) && (ev->atom == XA_WM_NAME))
    updatestatus();
  else if (ev->state == PropertyDelete && ev->atom != wmatom[WMState] &&
           ev->atom != motifwmhints && ev->atom != gtkframeextents)
    return; /* ignore (but decor-hint removal must re-resolve the title) */
  else if ((c = wintoclient(ev->window))) {
    if (ev->atom == wmatom[WMState]) {
      /* WM_STATE can be rewritten by the client or external tools
         (e.g. tray clients hiding with Withdrawn);
         re-sync the cached hidden flag so HIDDEN() stays accurate */
      int hid = ISHIDDENSTATE(getstate(c->win));
      if (hid != c->hidden) {
        c->hidden = hid;
        updateicon(c);
        arrange(c->mon);
        drawbars();
      }
    }
    switch (ev->atom) {
    default:
      break;
    case XA_WM_TRANSIENT_FOR:
      if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
          (c->isfloating = (wintoclient(trans)) != NULL))
        arrange(c->mon);
      break;
    case XA_WM_NORMAL_HINTS:
      c->hintsvalid = 0;
      break;
    case XA_WM_HINTS:
      updatewmhints(c);
      drawbars();
      break;
    }
    if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
      updatetitle(c);
      if (c == c->mon->sel && !c->isfullscreen && strstr(tabtext, "{title}"))
        drawbar(c->mon);
    } else if (ev->atom == netatom[NetWMIcon]) {
      c->icon_alpha = 0;
      updateicon(c);
      if (c == c->mon->sel && !c->isfullscreen)
        drawbar(c->mon);
      drawtitle(c);
    } else if (ev->atom == motifwmhints || ev->atom == gtkframeextents) {
      /* self-decoration hints (un)set at runtime: grow/shrink the frame */
      int old = c->notitle;
      updatenodecor(c);
      if (old != c->notitle && !c->isfullscreen) {
        c->h += c->notitle ? -th : th;
        c->oldh = c->h;
        arrange(c->mon);
      }
    }
    if (ev->atom == netatom[NetWMWindowType])
      updatewindowtype(c);
  }
}

void quit(const Arg *arg) {
  if(arg->i) restart = 1;
  running = 0;
}

Monitor *recttomon(int x, int y, int w, int h) {
  Monitor *m, *r = selmon;
  int a, area = 0;

  for (m = mons; m; m = m->next)
    if ((a = INTERSECT(x, y, w, h, m)) > area) {
      area = a;
      r = m;
    }
  return r;
}

void removesystrayicon(Client *i) {
  Client **ii;

  if (!showsystray || !i)
    return;
  for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next)
    ;
  if (ii)
    *ii = i->next;
  free(i);
}

void resize(Client *c, int x, int y, int w, int h, int interact) {
  if (applysizehints(c, &x, &y, &w, &h, interact))
    resizeclient(c, x, y, w, h);
}

void resizebarwin(Monitor *m) {
  unsigned int w = m->ww - 2 * sp;
  if (showsystray && m == systraytomon(m))
    w -= getsystraywidth();
  XMoveResizeWindow(dpy, m->barwin, m->wx + sp, m->by + vp, w, bh);
}

void resizeclient(Client *c, int x, int y, int w, int h) {
  XWindowChanges wc;

  c->oldx = c->x;
  c->x = wc.x = x;
  c->oldy = c->y;
  c->y = wc.y = y;
  c->oldw = c->w;
  c->w = wc.width = w;
  c->oldh = c->h;
  c->h = wc.height = h;
  wc.border_width = c->bw;
  /* x, y, w, h describe the frame; the client fills it below the title */
  XConfigureWindow(dpy, c->frame, CWX | CWY | CWWidth | CWHeight | CWBorderWidth,
                   &wc);
  placeclient(c);
  configure(c);
  drawtitle(c);
  XSync(dpy, False);
}

void resizemouse(const Arg *arg) {
  int ocx, ocy, nw, nh;
  Client *c;
  Monitor *m;
  XEvent ev;
  Time lasttime = 0;

  if (!(c = selmon->sel))
    return;
  if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
    return;
  restack(selmon);
  ocx = c->x;
  ocy = c->y;
  if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                   None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
    return;
  XWarpPointer(dpy, None, c->frame, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
  do {
    XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
    switch (ev.type) {
    case ConfigureRequest:
    case Expose:
    case MapRequest:
      handler[ev.type](&ev);
      break;
    case MotionNotify:
      if ((ev.xmotion.time - lasttime) <= (1000 / refreshrate))
        continue;
      lasttime = ev.xmotion.time;

      nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
      nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
      if (c->mon->wx + nw >= selmon->wx &&
          c->mon->wx + nw <= selmon->wx + selmon->ww &&
          c->mon->wy + nh >= selmon->wy &&
          c->mon->wy + nh <= selmon->wy + selmon->wh) {
        if (!c->isfloating && selmon->lt[selmon->sellt]->arrange &&
            (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
          togglefloatingclient(c);
      }
      if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
        resize(c, c->x, c->y, nw, nh, 1);
      break;
    }
  } while (ev.type != ButtonRelease);
  XWarpPointer(dpy, None, c->frame, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
  XUngrabPointer(dpy, CurrentTime);
  while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
    ;
  if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
    sendmon(c, m);
    selmon = m;
    focus(NULL);
  }
}

void resizerequest(XEvent *e) {
  XResizeRequestEvent *ev = &e->xresizerequest;
  Client *i;

  if ((i = wintosystrayicon(ev->window))) {
    updatesystrayicongeom(i, ev->width, ev->height);
    updatesystray(1);
  }
}

void restack(Monitor *m) {
  Client *c;
  XEvent ev;
  XWindowChanges wc;

  drawbar(m);
  drawtitles();
  if (!m->sel)
    return;
  if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
    XRaiseWindow(dpy, m->sel->frame);
  if (m->lt[m->sellt]->arrange) {
    wc.stack_mode = Below;
    wc.sibling = m->barwin;
    for (c = m->stack; c; c = c->snext)
      if (!c->isfloating && ISVISIBLE(c)) {
        XConfigureWindow(dpy, c->frame, CWSibling | CWStackMode, &wc);
        wc.sibling = c->frame;
      }
  }
  XSync(dpy, False);
  while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
    ;
}

void run(void) {
  XEvent ev;
  /* main event loop */
  XSync(dpy, False);
  while (running) {
    /* poll is the single blocking point: it wakes on X events, on the
       hover delay, or on the preview refresh interval; timeout -1
       blocks indefinitely when no hover is pending */
    struct pollfd pfd = {.fd = ConnectionNumber(dpy), .events = POLLIN};
    int timeout = -1;
    if (hoverarm)
      timeout = MAX((int)(hoverstart + hoverdelay - hovernow()), 0);
    else if (hoverc && !HIDDEN(hoverc) && previewrefresh)
      timeout = previewrefresh;
    int r = poll(&pfd, 1, timeout);
    if (r == 0) {
      if (hoverarm)
        hoverfire();
      else
        hoverrefresh();
      continue;
    }
    if (r < 0 && errno == EINTR)
      continue;
    while (running && XPending(dpy) && !XNextEvent(dpy, &ev)) {
      /* flood guard: skip redundant draw events when queue backs up */
      if (XPending(dpy) > 50 &&
          (ev.type == PropertyNotify || ev.type == Expose || ev.type == NoExpose)) {
        continue;
      }
      if (handler[ev.type])
        handler[ev.type](&ev); /* call handler */
    }
  }
}

void runautostart(void) {
  char *pathpfx;
  char *path;
  char *xdgdatahome;
  char *home;
  struct stat sb;

  if ((home = getenv("HOME")) == NULL)
    /* this is almost impossible */
    return;

  /* if $XDG_DATA_HOME is set and not empty, use $XDG_DATA_HOME/dwm,
   * otherwise use ~/.local/share/dwm as autostart script directory
   */
  xdgdatahome = getenv("XDG_DATA_HOME");
  if (xdgdatahome != NULL && *xdgdatahome != '\0') {
    /* space for path segments, separators and nul */
    pathpfx = ecalloc(1, strlen(xdgdatahome) + strlen(dwmdir) + 2);

    if (sprintf(pathpfx, "%s/%s", xdgdatahome, dwmdir) <= 0) {
      free(pathpfx);
      return;
    }
  } else {
    /* space for path segments, separators and nul */
    pathpfx =
        ecalloc(1, strlen(home) + strlen(localshare) + strlen(dwmdir) + 3);

    if (sprintf(pathpfx, "%s/%s/%s", home, localshare, dwmdir) < 0) {
      free(pathpfx);
      return;
    }
  }

  /* check if the autostart script directory exists */
  if (!(stat(pathpfx, &sb) == 0 && S_ISDIR(sb.st_mode))) {
    /* the XDG conformant path does not exist or is no directory
     * so we try ~/.dwm instead
     */
    char *pathpfx_new = realloc(pathpfx, strlen(home) + strlen(dwmdir) + 3);
    if (pathpfx_new == NULL) {
      free(pathpfx);
      return;
    }
    pathpfx = pathpfx_new;

    if (sprintf(pathpfx, "%s/.%s", home, dwmdir) <= 0) {
      free(pathpfx);
      return;
    }
  }

  /* try the blocking script first */
  path = ecalloc(1, strlen(pathpfx) + strlen(autostartblocksh) + 4);
  if (sprintf(path, "%s/%s", pathpfx, autostartblocksh) <= 0) {
    free(path);
    free(pathpfx);
    return;
  }

  if (access(path, X_OK) == 0)
    system(path);

  /* now the non-blocking script */
  if (sprintf(path, "%s/%s", pathpfx, autostartsh) <= 0) {
    free(path);
    free(pathpfx);
    return;
  }

  if (access(path, X_OK) == 0)
    system(strcat(path, " &"));

  free(pathpfx);
  free(path);
}

void scan(void) {
  unsigned int i, num;
  Window d1, d2, *wins = NULL;
  XWindowAttributes wa;

  if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
    for (i = 0; i < num; i++) {
      if (!XGetWindowAttributes(dpy, wins[i], &wa) || wa.override_redirect ||
          XGetTransientForHint(dpy, wins[i], &d1))
        continue;
      if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState) {
        if (!systrayredock(wins[i]))
          manage(wins[i], &wa, 0);
      }
    }
    for (i = 0; i < num; i++) { /* now the transients */
      if (!XGetWindowAttributes(dpy, wins[i], &wa))
        continue;
      if (XGetTransientForHint(dpy, wins[i], &d1) &&
          (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
        manage(wins[i], &wa, 0);
    }
    if (showsystray && systray)
      updatesystray(1);
    if (wins)
      XFree(wins);
  }
}

void sendmon(Client *c, Monitor *m) {
  if (c->mon == m)
    return;
  unfocus(c, 1);
  detach(c);
  detachstack(c);
  c->mon = m;
  c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
  attach(c);
  attachstack(c);
  updatewmdesktop(c);
  updateclientlist();
  if (c->isfullscreen)
    resizeclient(c, m->mx, m->my, m->mw, m->mh);
  focus(NULL);
  arrange(NULL);
}

void setclientstate(Client *c, long state) {
  long data[] = {state, None};

  XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
                  PropModeReplace, (unsigned char *)data, 2);
}

int sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2,
              long d3, long d4) {
  int n;
  Atom *protocols, mt;
  int exists = 0;
  XEvent ev;

  if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
    mt = wmatom[WMProtocols];
    if (XGetWMProtocols(dpy, w, &protocols, &n)) {
      while (!exists && n--)
        exists = protocols[n] == proto;
      XFree(protocols);
    }
  } else {
    exists = True;
    mt = proto;
  }

  if (exists) {
    ev.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = mt;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = d0;
    ev.xclient.data.l[1] = d1;
    ev.xclient.data.l[2] = d2;
    ev.xclient.data.l[3] = d3;
    ev.xclient.data.l[4] = d4;
    XSendEvent(dpy, w, False, mask, &ev);
  }
  return exists;
}

void setfocus(Client *c) {
  if (!c->neverfocus)
    XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
  XChangeProperty(dpy, root, netatom[NetActiveWindow], XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&(c->win), 1);
  sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus],
            CurrentTime, 0, 0, 0);
}

static const Layout *last_layout;
void fullscreen(const Arg *arg) {
  if (selmon->showbar) {
    last_layout = selmon->lt[selmon->sellt];
    setlayout(&((Arg){.v = &layouts[2]}));
  } else {
    setlayout(&((Arg){.v = last_layout}));
  }
  togglebar(arg);
}

void setfullscreen(Client *c, int fullscreen) {
  if (fullscreen && !c->isfullscreen) {
    XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&netatom[NetWMFullscreen],
                    1);
    c->isfullscreen = 1;
    c->oldstate = c->isfloating;
    c->oldbw = c->bw;
    c->bw = 0;
    c->isfloating = 1;
    resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
    XRaiseWindow(dpy, c->frame);
  } else if (!fullscreen && c->isfullscreen) {
    XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)0, 0);
    c->isfullscreen = 0;
    c->isfloating = c->oldstate;
    c->bw = c->oldbw;
    c->x = c->oldx;
    c->y = c->oldy;
    c->w = c->oldw;
    c->h = c->oldh;
    resizeclient(c, c->x, c->y, c->w, c->h);
    arrange(c->mon);
  }
}

void setlayout(const Arg *arg) {
  if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
    selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
  if (arg && arg->v)
    selmon->lt[selmon->sellt] =
        selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] =
            (Layout *)arg->v;
  strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol,
          sizeof selmon->ltsymbol);
  if (selmon->sel)
    arrange(selmon);
  else
    drawbar(selmon);
}

/* smallest preset strictly greater than cur, wrapping to the first */
static float nextpreset(const float *presets, unsigned int n, float cur) {
  unsigned int i;
  for (i = 0; i < n; i++)
    if (presets[i] > cur)
      return presets[i];
  return presets[0];
}

/* niri-style preset cycling: step forward through cfact_presets (wrap-around) */
void cyclecfact(const Arg *arg) {
  Client *c;

  c = selmon->sel;
  if (!c || !selmon->lt[selmon->sellt]->arrange)
    return;
  c->cfact = nextpreset(cfact_presets, LENGTH(cfact_presets), c->cfact);
  arrange(selmon);
}

/* niri-style preset cycling: step forward through mfact_presets (wrap-around) */
void cyclemfact(const Arg *arg) {
  if (!selmon->lt[selmon->sellt]->arrange)
    return;
  selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] =
      nextpreset(mfact_presets, LENGTH(mfact_presets), selmon->mfact);
  arrange(selmon);
}

/* persist a state property for the next hot-restart; nvals == 0 deletes it */
void setstateprop(Window w, Atom a, unsigned long *vals, int nvals) {
  if (nvals > 0)
    XChangeProperty(dpy, w, a, XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)vals, nvals);
  else
    XDeleteProperty(dpy, w, a);
}

void setup(void) {
  int i;
  XSetWindowAttributes wa;
  Atom utf8string;

  /* clean up any zombies immediately */
  sigchld(0);

  signal(SIGHUP, sighup);
  signal(SIGTERM, sigterm);

  /* init screen */
  screen = DefaultScreen(dpy);
  sw = DisplayWidth(dpy, screen);
  sh = DisplayHeight(dpy, screen);
  root = RootWindow(dpy, screen);
  xinitvisual();
  drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
  if (!(fonts_set = drw_fontset_create(drw, fonts, LENGTH(fonts))))
    die("no fonts could be loaded.");
  fonts_highlight_set = drw_fontset_create(drw, fonts_highlight, LENGTH(fonts_highlight));
  drw_setfontset(drw, fonts_set); /* drw_fontset_create overwrites drw->fonts */

  // NOTE: halving lpad avoids rounding cracks in status text width
  /*  lrpad = drw->fonts->h; */
  /* lpad = lrpad/2; */
  lpad = drw->fonts->h/2;
  lrpad = lpad * 2;
  tabr = MIN(tabradius, lpad);

  bh = drw->fonts->h + barfontpad * 2;
  th = showtitlebar ? drw->fonts->h + 2 * titlebarpad : 0;
  sp = sidepad;
  vp = (topbar == 1) ? vertpad : -vertpad;
  updategeom();
  {
    Monitor *m;
    for (m = mons; m; m = m->next) {
      char buf[32];
      snprintf(buf, sizeof(buf), "DWM_TAG_%d", m->num);
      char *env = getenv(buf);
      if (env) {
        unsigned int ts = atoi(env);
        if (ts & TAGMASK) {
          int i;
          m->tagset[m->seltags] = ts & TAGMASK;
          /* keep pertag->curtag in sync with the restored tagset so
             focusmode()/tile()/getgaps() see the same tag as ISVISIBLE() */
          if (m->tagset[m->seltags] == TAGMASK)
            m->pertag->curtag = 0;
          else {
            for (i = 0; !(m->tagset[m->seltags] & 1 << i); i++)
              ;
            m->pertag->curtag = i + 1;
          }
          m->pertag->prevtag = m->pertag->curtag;
          m->nmaster = m->pertag->nmasters[m->pertag->curtag];
          m->mfact = m->pertag->mfacts[m->pertag->curtag];
          m->sellt = m->pertag->sellts[m->pertag->curtag];
          m->lt[m->sellt] = m->pertag->ltidxs[m->pertag->curtag][m->sellt];
          m->lt[m->sellt ^ 1] =
              m->pertag->ltidxs[m->pertag->curtag][m->sellt ^ 1];
          m->showbar = m->pertag->showbars[m->pertag->curtag];
        }
        unsetenv(buf);
      }
    }
  }
  /* init atoms */
  utf8string = XInternAtom(dpy, "UTF8_STRING", False);
  wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
  wmatom[WMChangeState] = XInternAtom(dpy, "WM_CHANGE_STATE", False);
  wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
  netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
  netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
  netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  netatom[NetSystemTrayOrientation] =
      XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
  netatom[NetSystemTrayOrientationHorz] =
      XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
  netatom[NetSystemTrayVisual] =
      XInternAtom(dpy, "_NET_SYSTEM_TRAY_VISUAL", False);
  netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
  netatom[NetWMIcon] = XInternAtom(dpy, "_NET_WM_ICON", False);
  netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
  netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  netatom[NetWMFullscreen] =
      XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
  netatom[NetWMMaximizedVert] =
      XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  netatom[NetWMMaximizedHorz] =
      XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  netatom[NetWMWindowTypeDock] =
      XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
  netatom[NetWMWindowTypeDialog] =
      XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  netatom[NetWMWindowTypeTooltip] =
      XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLTIP", False);
  netatom[NetWMWindowOpacity] =
      XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
  netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
  netatom[NetNumberOfDesktops] =
      XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
  netatom[NetCurrentDesktop] =
      XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
  netatom[NetDesktopNames] = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);
  netatom[NetWMDesktop] = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
  xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
  xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
  xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
  motifwmhints = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
  gtkframeextents = XInternAtom(dpy, "_GTK_FRAME_EXTENTS", False);
  /* init cursors */
  cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
  cursor[CurResize] = drw_cur_create(drw, XC_sizing);
  cursor[CurMove] = drw_cur_create(drw, XC_fleur);
  /* init appearance */
  scheme = ecalloc(LENGTH(colors) + 1, sizeof(Clr *));
  /* the extra slot is the status working scheme: status drawing mutates
     drw->scheme in place with its ^c/^b codes, so it runs on a copy and
     keeps colors[SchemeStatus] pristine for ^d to restore from */
  scheme[LENGTH(colors)] =
      drw_scm_create(drw, colors[SchemeStatus], alphas[SchemeStatus], 3);
  for (i = 0; i < LENGTH(colors); i++)
    scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 3);
  /* init system tray */
  if (showsystray)
    updatesystray(0);
  /* init bars */
  updatebars();
  updatestatus();
  /* supporting window for NetWMCheck */
  wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
  XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&wmcheckwin, 1);
  XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
                  PropModeReplace, (unsigned char *)"dwm", 3);
  XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&wmcheckwin, 1);
  /* EWMH support per view */
  XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)netatom, NetLast);
  XDeleteProperty(dpy, root, netatom[NetClientList]);
  updatenumberofdesktops();
  updatedesktopnames();
  updatecurrentdesktop();
  /* select events */
  wa.cursor = cursor[CurNormal]->cursor;
  wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask |
                  ButtonPressMask | PointerMotionMask | EnterWindowMask |
                  LeaveWindowMask | StructureNotifyMask | PropertyChangeMask | KeyPressMask;
  XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);
  XSelectInput(dpy, root, wa.event_mask);
  grabkeys();
  focus(NULL);
}

void seturgent(Client *c, int urg) {
  XWMHints *wmh;

  c->isurgent = urg;
  if (!(wmh = XGetWMHints(dpy, c->win)))
    return;
  wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
  XSetWMHints(dpy, c->win, wmh);
  XFree(wmh);
}

void showhide(Client *c) {
  if (!c)
    return;
  if (ISVISIBLE(c)) {
    /* show clients top down (position only; mapping is owned by
       manage/showwin — an iconically hidden client must stay unmapped) */
    XMoveWindow(dpy, c->frame, c->x, c->y);
    if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) &&
        !c->isfullscreen)
      resize(c, c->x, c->y, c->w, c->h, 0);
    showhide(c->snext);
  } else {
    /* hide clients bottom up: park the frame offscreen but keep it mapped
       so composite snapshots (tag previews) keep working */
    showhide(c->snext);

    static XWindowAttributes ra;
    XGetWindowAttributes(dpy, root, &ra);

    if (c->tags < selmon->tagset[selmon->seltags])
      XMoveWindow(dpy, c->frame, -c->w * 3/2, c->y);
    else if (c->tags > selmon->tagset[selmon->seltags])
      XMoveWindow(dpy, c->frame, ra.width * 3/2, c->y);
  }
  updateicon(c);
}

void sigchld(int unused) {
  if (signal(SIGCHLD, sigchld) == SIG_ERR)
    die("can't install SIGCHLD handler:");
  while (0 < waitpid(-1, NULL, WNOHANG));
}

void sighup(int unused) {
  Arg a = {.i = 1};
  quit(&a);
}

void sigterm(int unused) {
  Arg a = {.i = 0};
  quit(&a);
}

void spawn(const Arg *arg) {
  if (fork() == 0) {
    if (dpy)
      close(ConnectionNumber(dpy));
    if (arg->v == statuscmd) {
      char index[8];
      snprintf(index, sizeof(index), "%d", statuscmdn);
      setenv("INDEX", index, 1);
      setenv("BUTTON", lastbutton, 1);
    }
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
    perror(" failed");
    _exit(EXIT_FAILURE);
  }
}

/* advance past the status2d code at *s and return its horizontal
   advance ('f' digits, ')' tabgap); other codes advance 0 */
static int status2d_advance(char **s) {
  int adv = 0;
  char *p = *s;

  if (*p == 'f') {
    adv = atoi(p + 1);
    while (p[1] && p[1] != '^')
      p++;
    *s = p;
  } else if (*p == ')' && tabradius > 0)
    adv = (int)tabgap;
  return adv;
}

/* parse stext into stbuf/stblocks. On portrait monitors bar_blocks keeps
   only the segment after the last 0x7f marker, which is the same rule the
   renderer used to apply inline; drawing and click resolution share it. */
static void statusparse(const char *text) {
  nstblocks =
      bar_blocks(text, stbuf, sizeof stbuf, stblocks, MAX_STBLOCKS);
}

/* drawn width of block i: its text with status2d codes interpreted */
static int status_block_width(int i) {
  int w, save = stbuf[stblocks[i].end];

  stbuf[stblocks[i].end] = '\0';
  w = status2d_runwidth(stbuf + stblocks[i].off);
  stbuf[stblocks[i].end] = save;
  return w;
}

/* Build the drawn status from the configured pills (see bar_pills): the
   writer only emits content plus colours and separates blocks with control
   characters, while grouping and rounded caps are injected here. Fills
   scells with each drawn block's offset and width so a click maps back to
   its id; the pill gap the old inline ^) used to produce is folded into the
   cell that ends the pill, so click ranges tile the drawn width. */
static void statuspills_build(const int *ids) {
  BarPillCell cells[MAX_STBLOCKS];
  int i, n;

  pbuf[0] = '\0';
  nscells = 0;
  nspills = 0;
  spillidx = 0;
  stpills_w = 0;
  if (!ids)
    return;

  n = bar_pills(stbuf, stblocks, nstblocks, ids, pbuf, sizeof pbuf, cells,
                MAX_STBLOCKS);
  spillidx = 0;
  nspills = 0;
  for (i = 0; i < n; i++) {
    int bw = status_block_width(cells[i].block);
    int gap = cells[i].gap ? (int)tabgap : 0;

    /* a cell that follows a pill's last one opens the next pill */
    if (nspills < MAX_STBLOCKS && (i == 0 || cells[i - 1].gap)) {
      spills[nspills].w = 0;
      nspills++;
    }
    if (nspills > 0)
      spills[nspills - 1].w += bw;
    if (nscells < MAX_STBLOCKS) {
      scells[nscells].id = stblocks[cells[i].block].id;
      scells[nscells].x = stpills_w;
      scells[nscells].w = bw + gap;
      nscells++;
    }
    stpills_w += bw + gap;
  }
}

/* drawn width of one NUL-terminated status2d run (a single block): text
   widths plus the 'f' and ')' advances, colours and rects advance nothing */
static int status2d_runwidth(char *s) {
  char *text = s, ch;
  int isCode = 0, w = 0;

  for (; *s; s++) {
    if ((unsigned char)(*s) == '^') {
      if (!isCode) {
        ch = *s;
        *s = '\0';
        if (strlen(text) > 0)
          w += TEXTW(text);
        *s = ch;
        text = s + 1;
        isCode = 1;
        continue;
      }
      isCode = 0;
      text = s + 1;
      continue;
    }
    if (isCode) {
      /* only the first char after '^' is the code identifier */
      if (s == text && (*s == 'f' || *s == '(' || *s == ')'))
        w += status2d_advance(&s);
      continue;
    }
  }
  if (!isCode && strlen(text) > 0)
    w += TEXTW(text);
  return w;
}

/* total drawn width of the pills selected by ids */
static int statuswidth(const int *ids) {
  statusparse(stext);
  statuspills_build(ids);
  return stpills_w;
}

/* hot-restart: re-dock former systray icons. When dwm restarts via
   XCloseDisplay+execvp, XAddToSaveSet reparents embedded icon windows
   back to root instead of destroying them, so the clients miss the
   DestroyNotify and won't re-request dock on their own. Identify them
   by WM_CLASS="Systray" and re-handshake through systraydock(). */
int systrayredock(Window w) {
  XClassHint ch = {NULL, NULL};
  int issystray;

  if (!showsystray || !systray)
    return 0;
  if (!XGetClassHint(dpy, w, &ch))
    return 0;
  issystray = (ch.res_name && strcmp(ch.res_name, "Systray") == 0);
  if (ch.res_name) XFree(ch.res_name);
  if (ch.res_class) XFree(ch.res_class);
  if (issystray)
    systraydock(w);
  return issystray;
}

Monitor *systraytomon(Monitor *m) {
  Monitor *t;
  int i, n;
  if (!systraypinning) {
    if (!m)
      return selmon;
    return m == selmon ? m : NULL;
  }
  for (n = 1, t = mons; t && t->next; n++, t = t->next)
    ;
  for (i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next)
    ;
  if (systraypinningfailfirst && n < systraypinning)
    return mons;
  return t;
}

void tag(const Arg *arg) {
  if (selmon->sel && arg->ui & TAGMASK) {
    Client *c = selmon->sel;
    unsigned int t = arg->ui & TAGMASK;
    c->tags = t;
    updatewmdesktop(c);
    updateclientlist();
    focus(NULL);
    arrange(selmon);
    if (focusonmove) {
      view(arg);
      if (t && !(t & (t - 1)) && !HIDDEN(c))
        focus(c);
    }
  }
}

void tagmon(const Arg *arg) {
  Client *c = selmon->sel;
  Monitor *m;
  if (!c || !mons->next)
    return;
  m = dirtomon(arg->i);
  sendmon(c, m);
  if (focusonmove && !HIDDEN(c)) {
    setcurrentmon(m);
    focus(c);
  }
}

void togglebar(const Arg *arg) {
  selmon->showbar = selmon->pertag->showbars[selmon->pertag->curtag] =
      !selmon->showbar;
  updatebarpos(selmon);
  resizebarwin(selmon);
  if (showsystray) {
    XWindowChanges wc;
    if (!selmon->showbar)
      wc.y = -bh;
    else if (selmon->showbar) {
      wc.y = vp;
      if (!selmon->topbar)
        wc.y = selmon->mh - bh + vp;
    }
    XConfigureWindow(dpy, systray->win, CWY, &wc);
  }
  arrange(selmon);
}

/* flip the bar's tab rendering between one titled pill per client and a single
   shared pill of icons. The mode decides the row's layout and the tab icon's
   own size (TabModeIcons keeps room for the selection dot), so every client's
   tab-sized icon is rebuilt and every bar is redrawn; the old slot geometry
   and any open tab tooltip go with it. */
static void toggletabmode(const Arg *arg) {
  Monitor *m;
  Client *c;

  tabmode = tabmode == TabModeIconTitle ? TabModeIcons : TabModeIconTitle;
  for (m = mons; m; m = m->next)
    for (c = m->clients; c; c = c->next)
      updateicon(c);
  hoverhide();
  drawbars();
}

/* find the Layout entry whose arrange function matches, so single-master modes
   can switch to / restore from their host layout without hardcoding an index */
static const Layout *findlayout(void (*arrange)(Monitor *)) {
  unsigned int i;
  for (i = 0; i < LENGTH(layouts); i++)
    if (layouts[i].arrange == arrange)
      return &layouts[i];
  return &layouts[0];
}

/* Enter/exit or switch a single-master mode. mode 1 = focusmaster (host
   centeredfloatingmaster), mode 2 = maximize (host monocle). Pressing the key
   for the active mode exits it; pressing the other one switches modes. Leaving
   a mode restores the layout that was active before the first mode was entered
   (fmlast, kept across mode switches). */
static void setfmmode(int mode, void (*arrange)(Monitor *)) {
  unsigned int t = selmon->pertag->curtag;
  int *f = &selmon->pertag->focusmaster[t];
  const Layout *host = findlayout(arrange);

  if (*f == mode) {
    /* leaving the mode: clear it, restore the pre-mode layout if recorded */
    *f = 0;
    if (selmon->pertag->fmlast[t]) {
      setlayout(&((Arg){.v = (void *)selmon->pertag->fmlast[t]}));
      selmon->pertag->fmlast[t] = NULL;
    } else {
      arrange(selmon);
    }
    return;
  }

  /* entering (or switching to) the mode: only the first entry from plain mode
     records the layout to restore later; switching 1<->2 keeps the original */
  if (!*f && selmon->lt[selmon->sellt] != host)
    selmon->pertag->fmlast[t] = selmon->lt[selmon->sellt];
  *f = mode;
  if (selmon->lt[selmon->sellt] != host)
    setlayout(&((Arg){.v = (void *)host}));
  else
    arrange(selmon);
}

void focusmaster(const Arg *arg) {
  setfmmode(1, centeredfloatingmaster); /* focusmaster: host = centeredfloatingmaster */
}

void maximize(const Arg *arg) {
  setfmmode(2, monocle); /* maximize: host = monocle (all tiled stacked) */
}

/* focused client when any single-master mode is active for m's current tag,
   else NULL. Callers use it for both focusmaster (mode 1) and maximize (mode 2):
   focus() rearranges on focus switch so the focused window stays the visible
   master / topmost monocle window. */
static Client *focusclient(Monitor *m) {
  Client *c = m->sel;
  if (!m->pertag->focusmaster[m->pertag->curtag] || !c || c->mon != m)
    return NULL;
  if (c->isfloating || !ISVISIBLE(c) || HIDDEN(c))
    return NULL;
  return c;
}

/* single-master mode for m's current tag: 0 off, 1 focusmaster, 2 maximize */
static int focusmode(Monitor *m) {
  return focusclient(m) ? m->pertag->focusmaster[m->pertag->curtag] : 0;
}

void togglefloatingclient(Client *c) {
  if (!c || c->isfullscreen) /* no support for fullscreen windows */
    return;
  if (c->isfloating && !c->isfixed) {
    /* floating -> tiled: remember the floating geometry ... */
    c->sfx = c->x;
    c->sfy = c->y;
    c->sfw = c->w;
    c->sfh = c->h;
    c->isfloating = 0;
  } else {
    /* ... tiled -> floating: go back to it (fixed windows stay floating) */
    c->isfloating = 1;
    if (c->sfw > 0)
      resize(c, c->sfx, c->sfy, c->sfw, c->sfh, 0);
    else
      resize(c, c->x, c->y, c->w, c->h, 0);
  }

  arrange(c->mon);
}

void togglefloating(const Arg *arg) {
  (void)arg;
  togglefloatingclient(selmon->sel);
}

void toggletag(const Arg *arg) {
  unsigned int newtags;

  if (!selmon->sel)
    return;
  newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
  if (newtags) {
    selmon->sel->tags = newtags;
    updatewmdesktop(selmon->sel);
    updateclientlist();
    focus(NULL);
    arrange(selmon);
  }
}

void toggleview(const Arg *arg) {
  unsigned int newtagset =
      selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);
  int i;

  if (newtagset) {
    takepreview();
    selmon->tagset[selmon->seltags] = newtagset;

    if (newtagset == ~0) {
      selmon->pertag->prevtag = selmon->pertag->curtag;
      selmon->pertag->curtag = 0;
    }

    /* test if the user did not select the same tag */
    if (!(newtagset & 1 << (selmon->pertag->curtag - 1))) {
      selmon->pertag->prevtag = selmon->pertag->curtag;
      for (i = 0; !(newtagset & 1 << i); i++)
        ;
      selmon->pertag->curtag = i + 1;
    }

    /* apply settings for this view */
    selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
    selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
    selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
    selmon->lt[selmon->sellt] =
        selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
    selmon->lt[selmon->sellt ^ 1] =
        selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt ^ 1];

    if (selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
      togglebar(NULL);

    focus(NULL);
    arrange(selmon);
    updatecurrentdesktop();
  }
}

void freehspic(Client *c) {
  if (c->hspic) {
    XRenderFreePicture(dpy, c->hspic);
    c->hspic = None;
  }
  if (c->hspm) {
    XFreePixmap(dpy, c->hspm);
    c->hspm = None;
  }
  c->hspw = c->hsph = 0;
}

void freeicon(Client *c) {
  if (c->icon) {
    XRenderFreePicture(dpy, c->icon);
    c->icon = None;
  }
  if (c->tabicon) {
    XRenderFreePicture(dpy, c->tabicon);
    c->tabicon = None;
  }
  c->tabicon_size = 0;
}

void togglewin(const Arg *arg) {
  Client *c = (Client *)arg->v;

  if (c == selmon->sel) {
    hidewin(c);
    focus(NULL);
    arrange(c->mon);
  } else {
    if (HIDDEN(c))
      showwin(c);
    focus(c);
    restack(selmon);
  }
}

void unfocus(Client *c, int setfocus) {
  if (!c)
    return;
  grabbuttons(c, 0);
  if (c->frame != None)
    XSetWindowBorder(dpy, c->frame, scheme[SchemeNorm][ColBorder].pixel);
  if (setfocus) {
    XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
  }
}

void unmanage(Client *c, int destroyed) {
  Monitor *m = c->mon;
  XWindowChanges wc;

  detach(c);
  detachstack(c);
  freeicon(c);
  freehspic(c);
  if (!destroyed) {
    wc.border_width = c->oldbw;
    XGrabServer(dpy); /* avoid race conditions */
    XSetErrorHandler(xerrordummy);
    /* give the client back to root and restore its border */
    XRemoveFromSaveSet(dpy, c->win);
    XReparentWindow(dpy, c->win, root, c->x, c->y);
    XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
    XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
    setclientstate(c, WithdrawnState);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XUngrabServer(dpy);
  }
  if (c->frame != None)
    XDestroyWindow(dpy, c->frame);
  free(c);
  focus(NULL);
  updateclientlist();
  arrange(m);
}

void unmapnotify(XEvent *e) {
  Client *c;
  XUnmapEvent *ev = &e->xunmap;

  if (frameclient(ev->window))
    return; /* frame unmaps are WM-initiated (hide/showhide), ignore */
  if ((c = wintoclient(ev->window))) {
    if (c->ignoreunmap > 0) {
      c->ignoreunmap--;
      return;
    }
    if (c == hoverc)
      hoverhide();
    if (ev->send_event) {
      /* client-initiated withdraw (XWithdrawWindow, used by minimize-to-tray):
         treat like hide()'s Iconic, otherwise focusstackvis would select
         an unmapped window that never displays */
      setclientstate(c, WithdrawnState);
      c->hidden = 1;
      updateicon(c);
      if (c == selmon->sel)
        focus(NULL);
      arrange(c->mon);
      drawbars();
    } else
      unmanage(c, 0);
  } else if ((c = wintosystrayicon(ev->window))) {
    /* KLUDGE! sometimes icons occasionally unmap their windows, but do
     * _not_ destroy them. We map those windows back */
    XMapRaised(dpy, c->win);
    updatesystray(1);
  }
}

void updatebars(void) {
  Monitor *m;
  XSetWindowAttributes wa = {.override_redirect = True,
                             .background_pixel = 0,
                             .border_pixel = 0,
                             .colormap = cmap,
                             .event_mask = ButtonPressMask | ExposureMask |
                                           EnterWindowMask | LeaveWindowMask |
                                           PointerMotionMask};
  XClassHint ch = {"dwm", "dwm"};
  for (m = mons; m; m = m->next) {
    int dw, dh;

    previewsize(m, m->mw, m->mh, &dw, &dh);
    if (!m->tagwin) {
      XSetWindowAttributes twa = {
          .override_redirect = True,
          .background_pixel = scheme[SchemeTooltip][ColBg].pixel,
          .border_pixel = 0,
          .colormap = cmap,
          .event_mask = PointerMotionMask};
      m->tagwin = XCreateWindow(dpy, root, m->wx + sp, m->by + vp + bh,
                                dw + hoverpad * 2, dh + hoverpad * 2, 0, depth,
                                InputOutput, visual,
                                CWOverrideRedirect | CWBackPixel |
                                    CWBorderPixel | CWColormap | CWEventMask,
                                &twa);
      XUnmapWindow(dpy, m->tagwin);
    }
    XMoveResizeWindow(dpy, m->tagwin, m->wx + sp, m->by + vp + bh,
                      dw + hoverpad * 2, dh + hoverpad * 2);
    if (!m->barwin) {
      m->barwin = XCreateWindow(dpy, root, m->wx + sp, m->by + vp, m->ww, bh,
                        0, depth, InputOutput, visual,
                        CWOverrideRedirect | CWBackPixel | CWBorderPixel |
                            CWColormap | CWEventMask,
                        &wa);
      XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
      if (showsystray && m == systraytomon(m))
        XMapRaised(dpy, systray->win);
      XMapRaised(dpy, m->barwin);
      XSetClassHint(dpy, m->barwin, &ch);
    }
  }
}

void updatebarpos(Monitor *m) {
  m->wy = m->my;
  m->wh = m->mh;
  if (m->showbar) {
    m->wh = m->wh - vertpad - bh;
    m->by = m->topbar ? m->wy : m->wy + m->wh + vertpad;
    m->wy = m->topbar ? m->wy + bh + vp : m->wy;
  } else
    m->by = -bh - vp;
}

/* Primary tag index for EWMH desktop mapping (DESKTOP <-> tag).
 * Multi-tag windows appear once, under their lowest set bit.
 * Sticky (~0, e.g. bilichat-tui) naturally falls to 0 (first group).
 * tags == 0 (untagged) also maps to 0. */
static int
clientdesktop(Client *c) {
  unsigned int i;

  if (!c || !c->tags)
    return 0;
  for (i = 0; i < LENGTH(tags); i++)
    if (c->tags & 1 << i)
      return (int)i;
  return 0;
}

static void
updatenumberofdesktops(void) {
  unsigned long n = LENGTH(tags);

  XChangeProperty(dpy, root, netatom[NetNumberOfDesktops], XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&n, 1);
}

static void
updatedesktopnames(void) {
  Atom utf8 = XInternAtom(dpy, "UTF8_STRING", False);
  /* icon+name per tag, NUL-separated per EWMH */
  char buf[LENGTH(tags) * 64];
  size_t off = 0;
  unsigned int i;

  for (i = 0; i < LENGTH(tags); i++) {
    char text[64];
    size_t len;

    template_expand(tagtext, tag_placeholder, &i, text, sizeof(text));
    len = strlen(text) + 1;
    if (off + len > sizeof(buf))
      break;
    memcpy(buf + off, text, len);
    off += len;
  }
  XChangeProperty(dpy, root, netatom[NetDesktopNames], utf8, 8,
                  PropModeReplace, (unsigned char *)buf, off);
}

static void
updatecurrentdesktop(void) {
  unsigned long d = 0;
  unsigned int i;

  if (selmon)
    for (i = 0; i < LENGTH(tags); i++)
      if (selmon->tagset[selmon->seltags] & 1 << i) {
        d = i;
        break;
      }
  XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&d, 1);
}

static void
updatewmdesktop(Client *c) {
  unsigned long d;

  if (!c)
    return;
  d = (unsigned long)clientdesktop(c);
  XChangeProperty(dpy, c->win, netatom[NetWMDesktop], XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *)&d, 1);
}

void updateclientlist() {
  Client *c;
  Monitor *m;
  /* Global tag-first order: collect in (monitor, m->clients) order (= bar
   * relative order), then stable-sort by primary tag only. Equal tags keep
   * their (monitor, bar) order. m->clients itself is never reordered.
   * Published with a single Replace so readers never see a partial list. */
  Client **order = NULL;
  Window *wins = NULL;
  size_t n = 0, cap = 0;
  size_t i, j;

  for (m = mons; m; m = m->next)
    for (c = m->clients; c; c = c->next) {
      if (n == cap) {
        size_t ncap = cap ? cap * 2 : 32;
        Client **norder = ecalloc(ncap, sizeof(*norder));
        if (order) {
          memcpy(norder, order, n * sizeof(*order));
          free(order);
        }
        order = norder;
        cap = ncap;
      }
      order[n++] = c;
    }
  /* stable insertion sort by primary tag */
  for (i = 1; i < n; i++) {
    Client *tmp = order[i];
    int td = clientdesktop(tmp);
    j = i;
    while (j > 0 && clientdesktop(order[j - 1]) > td) {
      order[j] = order[j - 1];
      j--;
    }
    order[j] = tmp;
  }
  if (!n) {
    XDeleteProperty(dpy, root, netatom[NetClientList]);
    return;
  }
  wins = ecalloc(n, sizeof(*wins));
  for (i = 0; i < n; i++)
    wins[i] = order[i]->win;
  free(order);
  XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)wins, n);
  free(wins);
}

int updategeom(void) {
  int dirty = 0;

#ifdef XINERAMA
  if (XineramaIsActive(dpy)) {
    int i, j, n, nn;
    Client *c;
    Monitor *m;
    XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
    XineramaScreenInfo *unique = NULL;

    for (n = 0, m = mons; m; m = m->next, n++)
      ;
    /* only consider unique geometries as separate screens */
    unique = ecalloc(nn, sizeof(XineramaScreenInfo));
    for (i = 0, j = 0; i < nn; i++)
      if (isuniquegeom(unique, j, &info[i]))
        memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
    XFree(info);
    nn = j;

    /* new monitors if nn > n */
    for (i = n; i < nn; i++) {
      for (m = mons; m && m->next; m = m->next)
        ;
      if (m)
        m->next = createmon();
      else
        mons = createmon();
    }
    for (i = 0, m = mons; i < nn && m; m = m->next, i++)
      if (i >= n || unique[i].x_org != m->mx || unique[i].y_org != m->my ||
          unique[i].width != m->mw || unique[i].height != m->mh) {
        dirty = 1;
        m->num = i;
        m->mx = m->wx = unique[i].x_org;
        m->my = m->wy = unique[i].y_org;
        m->mw = m->ww = unique[i].width;
        m->mh = m->wh = unique[i].height;
        updatebarpos(m);
      }
    /* removed monitors if n > nn */
    for (i = nn; i < n; i++) {
      for (m = mons; m && m->next; m = m->next)
        ;
      while ((c = m->clients)) {
        dirty = 1;
        m->clients = c->next;
        detachstack(c);
        c->mon = mons;
        attachtop ? attach(c) : attachbottom(c);
        attachstack(c);
      }
      if (m == selmon)
        selmon = mons;
      cleanupmon(m);
    }
    free(unique);
  } else
#endif /* XINERAMA */
  {    /* default monitor setup */
    if (!mons)
      mons = createmon();
    if (mons->mw != sw || mons->mh != sh) {
      dirty = 1;
      mons->mw = mons->ww = sw;
      mons->mh = mons->wh = sh;
      updatebarpos(mons);
    }
  }
  if (dirty) {
    selmon = mons;
    selmon = wintomon(root);
  }
  return dirty;
}

void updatenumlockmask(void) {
  unsigned int i, j;
  XModifierKeymap *modmap;

  numlockmask = 0;
  modmap = XGetModifierMapping(dpy);
  for (i = 0; i < 8; i++)
    for (j = 0; j < modmap->max_keypermod; j++)
      if (modmap->modifiermap[i * modmap->max_keypermod + j] ==
          XKeysymToKeycode(dpy, XK_Num_Lock))
        numlockmask = (1 << i);
  XFreeModifiermap(modmap);
}

void updatesizehints(Client *c) {
  long msize;
  XSizeHints size;

  if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
    /* size is uninitialized, ensure that size.flags aren't used */
    size.flags = PSize;
  if (size.flags & PBaseSize) {
    c->basew = size.base_width;
    c->baseh = size.base_height;
  } else if (size.flags & PMinSize) {
    c->basew = size.min_width;
    c->baseh = size.min_height;
  } else
    c->basew = c->baseh = 0;
  if (size.flags & PResizeInc) {
    c->incw = size.width_inc;
    c->inch = size.height_inc;
  } else
    c->incw = c->inch = 0;
  if (size.flags & PMaxSize) {
    c->maxw = size.max_width;
    c->maxh = size.max_height;
  } else
    c->maxw = c->maxh = 0;
  if (size.flags & PMinSize) {
    c->minw = size.min_width;
    c->minh = size.min_height;
  } else if (size.flags & PBaseSize) {
    c->minw = size.base_width;
    c->minh = size.base_height;
  } else
    c->minw = c->minh = 0;
  if (size.flags & PAspect) {
    c->mina = (float)size.min_aspect.y / size.min_aspect.x;
    c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
  } else
    c->maxa = c->mina = 0.0;
  c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
  c->hintsvalid = 1;
}

void updatestatus(void) {
  if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
    strcpy(stext, "dwm-" VERSION);
  drawbar(selmon);
}

/* unified systray dock entry point shared by clientmessage() and scan().
 * Must include the full XEMBED handshake (EmbeddedNotify + XSelectInput +
 * setclientstate) — skipping any of these causes SNI bridge tools like
 * SysTray-X (and their managed icons, e.g. xunlei via SNI) to either draw
 * blank or block waiting for embed confirmation. */
void systraydock(Window w) {
  XWindowAttributes wa;
  XSetWindowAttributes swa;
  XClassHint ch = {"Systray", "systray"};
  Client *c;

  if (!showsystray || !systray || !w)
    return;
  /* prevent double-dock: if the same window is already in the systray
   * (e.g. scan() re-docked it before the client's own dock request),
   * skip to avoid duplicate entries and blank slots. */
  if (wintosystrayicon(w))
    return;

  if (!(c = (Client *)calloc(1, sizeof(Client))))
    die("fatal: could not malloc() %u bytes\n", sizeof(Client));
  c->win = w;
  c->mon = selmon;

  {
    XClassHint hint = {NULL, NULL};
    if (XGetClassHint(dpy, c->win, &hint)) {
      const char *name = hint.res_name ? hint.res_name : hint.res_class;
      if (name) {
        strncpy(c->class, name, sizeof(c->class) - 1);
        c->class[sizeof(c->class) - 1] = '\0';
      }
      if (hint.res_name) XFree(hint.res_name);
      if (hint.res_class) XFree(hint.res_class);
    }
  }

  XSetClassHint(dpy, c->win, &ch);
  XGetWindowAttributes(dpy, c->win, &wa);
  c->x = c->oldx = c->y = c->oldy = 0;
  c->w = c->oldw = wa.width;
  c->h = c->oldh = wa.height;
  c->oldbw = wa.border_width;
  c->bw = 0;
  c->isfloating = True;
  c->tags = 1;
  updatesizehints(c);
  updatesystrayicongeom(c, wa.width, wa.height);
  XAddToSaveSet(dpy, c->win);
  XSelectInput(dpy, c->win,
               StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
  XReparentWindow(dpy, c->win, systray->win, 0, 0);
  swa.background_pixel = scheme[SchemeSystray][ColBg].pixel;
  XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
  XSetWindowBackgroundPixmap(dpy, c->win, ParentRelative);
  sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
            XEMBED_EMBEDDED_NOTIFY, 0, systray->win, XEMBED_EMBEDDED_VERSION);
  XSync(dpy, False);
  setclientstate(c, NormalState);

  {
    int rank = trayrank(c->class);
    Client **cur;
    for (cur = &systray->icons; *cur && trayrank((*cur)->class) < rank; cur = &(*cur)->next);
    c->next = *cur;
    *cur = c;
  }
  updatesystray(1);
}

void updatesystray(int flag) {
  XSetWindowAttributes wa;
  XWindowChanges wc;
  Client *i;
  Monitor *m = systraytomon(NULL);
  unsigned int x = m->mx + m->mw;
  unsigned int w = 1, xpad = 0, ypad = 0;
  xpad = sp;
  ypad = vp;

  int updatebar = flag&1;
  int refresh_icon = flag&2;

  if (!showsystray)
    return;
  if (!systray) {
    /* init systray */
    if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
      die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
    systray->win = XCreateSimpleWindow(dpy, root, x - sp, m->by + vp,
                             w > 2 * tabborderpx ? w - 2 * tabborderpx : 1,
                             bh > 2 * tabborderpx ? bh - 2 * tabborderpx : 1,
                             tabborderpx, scheme[SchemeSystray][ColBorder].pixel,
                             scheme[SchemeSystray][ColBg].pixel);
    wa.background_pixel = scheme[SchemeSystray][ColBg].pixel;
    wa.event_mask        = ButtonPressMask | ExposureMask;
    wa.override_redirect = True;
    wa.border_pixel = scheme[SchemeSystray][ColBorder].pixel;
    XSelectInput(dpy, systray->win, SubstructureNotifyMask);
    XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
    XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel|CWBorderPixel, &wa);
    uint32_t opacity = (uint32_t)(alphas[SchemeSystray][1] * 0x01010101U);
    XChangeProperty(dpy, systray->win, netatom[NetWMWindowOpacity], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&opacity, 1);
    XMapRaised(dpy, systray->win);
    XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
    if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
      sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
      XSync(dpy, False);
    } else {
      fprintf(stderr, "dwm: unable to obtain system tray.\n");
      free(systray);
      systray = NULL;
      return;
    }
  }

  for (w = 0, i = systray->icons; i; i = i->next) {
    wa.background_pixel = scheme[SchemeSystray][ColBg].pixel;
    XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
    XSetWindowBackgroundPixmap(dpy, i->win, ParentRelative);
    XMapRaised(dpy, i->win);
    w += systrayspacing;
    i->x = w;
    XMoveResizeWindow(dpy, i->win, i->x, i->y, i->w, i->h);
    if (refresh_icon)
      XClearArea(dpy, i->win, 0, 0, 0, 0, True);
    w += i->w;
    if (i->mon != m)
      i->mon = m;
  }
  w = w ? w + systrayspacing : 1;
  x -= w;
  XSetWindowBackground(dpy, systray->win, scheme[SchemeSystray][ColBg].pixel);
  XMoveResizeWindow(dpy, systray->win, x - xpad, m->by + ypad,
                    w > 2 * tabborderpx ? w - 2 * tabborderpx : 1,
                    bh > 2 * tabborderpx ? bh - 2 * tabborderpx : 1);
  wc.x = x - xpad;
  wc.y = m->by + ypad;
  wc.width = w > 2 * tabborderpx ? w - 2 * tabborderpx : 1;
  wc.height = bh > 2 * tabborderpx ? bh - 2 * tabborderpx : 1;
  wc.stack_mode = Above;
  wc.sibling = m->barwin;
  XConfigureWindow(dpy, systray->win, CWX | CWY | CWWidth | CWHeight | CWSibling | CWStackMode, &wc);
  XMapWindow(dpy, systray->win);
  XMapSubwindows(dpy, systray->win);
  XSync(dpy, False);

  if (updatebar)
    drawbar(m);
}

void updatesystrayicongeom(Client *i, int w, int h) {
  int newh = bh - systraypad * 2;
  if (i) {
    i->h = newh;
    i->w = (int)((float)w * (float)i->h / (float)h);
    applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
    /* force icons into the systray dimensions if they don't want to */
    if (i->h > newh) {
      i->w = (int)((float)i->w * (float)newh / (float)i->h);
      i->h = newh;
    }
    i->y = (bh - 2 * tabborderpx - newh) / 2;
  }
}

void updatesystrayiconstate(Client *i, XPropertyEvent *ev) {
  long flags;
  int code = 0;

  if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
      !(flags = getatomprop(i, xatom[XembedInfo])))
    return;

  if ((flags & XEMBED_MAPPED) && !i->tags) {
    i->tags = 1;
    code = XEMBED_WINDOW_ACTIVATE;
    XMapRaised(dpy, i->win);
    setclientstate(i, NormalState);
  } else if (!(flags & XEMBED_MAPPED) && i->tags) {
    i->tags = 0;
    code = XEMBED_WINDOW_DEACTIVATE;
    XUnmapWindow(dpy, i->win);
    setclientstate(i, WithdrawnState);
  } else
    return;
  sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
            systray->win, XEMBED_EMBEDDED_VERSION);
}

void show(const Arg *arg) {
  if (selmon->hidsel)
    selmon->hidsel = 0;
  showwin(selmon->sel);
}

void showall(const Arg *arg) {
  Client *c = NULL;
  selmon->hidsel = 0;
  for (c = selmon->clients; c; c = c->next) {
    if (ISVISIBLE(c))
      showwin(c);
  }
  if (!selmon->sel) {
    for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next)
      ;
    if (c)
      focus(c);
  }
  restack(selmon);
}

void showwin(Client *c) {
  if (!c)
    return;

  if (!HIDDEN(c)) {
    /* fallback: flag says visible but frame is actually unmapped
       (e.g. WM_STATE rewritten after hide()); remap or it stays selected-but-invisible */
    XWindowAttributes wa;
    if (XGetWindowAttributes(dpy, c->frame, &wa) && wa.map_state == IsViewable)
      return;
  }

  XMapWindow(dpy, c->frame);
  XMapWindow(dpy, c->win);
  c->hidden = 0;
  setclientstate(c, NormalState);
  updateicon(c);
  arrange(c->mon);
  drawtitle(c);
}

void updatetitle(Client *c) {
  if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
    gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
  if (c->name[0] == '\0') /* hack to mark broken clients */
    strcpy(c->name, broken);
  drawtitle(c);
}

void updateicon(Client *c) {
  int scm = c->mon->sel == c ? SchemeSel : SchemeNorm;
  unsigned int new_alpha;
  int tabsz;

  if (c->hidden)
    scm = SchemeHid;

  new_alpha = alphas[scm][0];
  /* the tab icon is a second picture at a smaller size when the bar keeps
     room under it for the selection dot; 0 when both sizes agree */
  tabsz = tabiconsize();
  if (tabsz == ICONSIZE)
    tabsz = 0;
  if (c->icon_alpha == new_alpha && c->icon && c->tabicon_size == tabsz)
    return;

  freeicon(c);
  c->icon_alpha = new_alpha;
  c->tabicon_size = tabsz;
  c->icon = geticonprop(c->win, &c->icw, &c->ich, new_alpha, ICONSIZE);
  if (tabsz)
    c->tabicon = geticonprop(c->win, &c->tabicw, &c->tabich, new_alpha, tabsz);
}


void updatewindowtype(Client *c) {
  Atom state = getatomprop(c, netatom[NetWMState]);
  Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

  if (state == netatom[NetWMFullscreen])
    setfullscreen(c, 1);
  if (wtype == netatom[NetWMWindowTypeDialog])
    c->isfloating = 1;
}

void updatewmhints(Client *c) {
  XWMHints *wmh;

  if ((wmh = XGetWMHints(dpy, c->win))) {
    if (c == selmon->sel && wmh->flags & XUrgencyHint) {
      wmh->flags &= ~XUrgencyHint;
      XSetWMHints(dpy, c->win, wmh);
    } else
      c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
    if (wmh->flags & InputHint)
      c->neverfocus = !wmh->input;
    else
      c->neverfocus = 0;
    XFree(wmh);
  }
}

void view(const Arg *arg) {
  int i;
  unsigned int tmptag;

  if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
    return;
  takepreview();
  selmon->seltags ^= 1; /* toggle sel tagset */
  if (arg->ui & TAGMASK) {
    selmon->pertag->prevtag = selmon->pertag->curtag;

    if (arg->ui == ~0)
      selmon->pertag->curtag = 0;
    else {
      for (i = 0; !(arg->ui & 1 << i); i++)
        ;
      selmon->pertag->curtag = i + 1;
    }
  } else {
    tmptag = selmon->pertag->prevtag;
    selmon->pertag->prevtag = selmon->pertag->curtag;
    selmon->pertag->curtag = tmptag;
  }

  selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
  selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
  selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
  selmon->lt[selmon->sellt] =
      selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
  selmon->lt[selmon->sellt ^ 1] =
      selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt ^ 1];

  if (selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
    togglebar(NULL);

  selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
  focus(NULL);
  arrange(selmon);
  updatecurrentdesktop();
}

Client *wintoclient(Window w) {
  Client *c;
  Monitor *m;

  for (m = mons; m; m = m->next)
    for (c = m->clients; c; c = c->next)
      if (c->win == w)
        return c;
  return NULL;
}

Monitor *wintomon(Window w) {
  int x, y;
  Client *c;
  Monitor *m;

  if (w == root && getrootptr(&x, &y))
    return recttomon(x, y, 1, 1);
  for (m = mons; m; m = m->next)
    if (w == m->barwin)
      return m;
  if ((c = wintoclient(w)))
    return c->mon;
  if ((c = frameclient(w)))
    return c->mon;
  return selmon;
}

Client *wintosystrayicon(Window w) {
  Client *i = NULL;

  if (!showsystray || !w)
    return i;
  for (i = systray->icons; i && i->win != w; i = i->next)
    ;
  return i;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int xerror(Display *dpy, XErrorEvent *ee) {
  if (ee->error_code == BadWindow ||
      (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch) ||
      (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable) ||
      (ee->request_code == X_PolyFillRectangle &&
       ee->error_code == BadDrawable) ||
      (ee->request_code == X_PolySegment && ee->error_code == BadDrawable) ||
      (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch) ||
      (ee->request_code == X_GrabButton && ee->error_code == BadAccess) ||
      (ee->request_code == X_GrabKey && ee->error_code == BadAccess) ||
      (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
    return 0;
  fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
          ee->request_code, ee->error_code);
  return xerrorxlib(dpy, ee); /* may call exit */
}

int xerrordummy(Display *dpy, XErrorEvent *ee) { return 0; }

/* Startup Error handler to check if another window manager
 * is already running. */
int xerrorstart(Display *dpy, XErrorEvent *ee) {
  die("dwm: another window manager is already running");
  return -1;
}

void xinitvisual() {
  XVisualInfo *infos;
  XRenderPictFormat *fmt;
  int nitems;
  int i;

  XVisualInfo tpl = {.screen = screen, .depth = 32, .class = TrueColor};
  long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;

  infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
  visual = NULL;
  for (i = 0; i < nitems; i++) {
    fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
    if (fmt && fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
      visual = infos[i].visual;
      depth = infos[i].depth;
      cmap = XCreateColormap(dpy, root, visual, AllocNone);
      useargb = 1;
      break;
    }
  }

  XFree(infos);

  if (!visual) {
    visual = DefaultVisual(dpy, screen);
    depth = DefaultDepth(dpy, screen);
    cmap = DefaultColormap(dpy, screen);
  }
}

void zoom(const Arg *arg) {
  Client *c = selmon->sel;

  if (!c || !selmon->lt[selmon->sellt]->arrange || c->isfloating)
    return;
  if (c == nexttiled(selmon->clients))
    if (!(c = nexttiled(c->next)))
      return;
  pop(c);
}

void restorestacking(void) {
  Atom sa = XInternAtom(dpy, "_DWM_STACKING", False);
  Atom actual;
  int fmt;
  unsigned long n, extra, i;
  unsigned char *data = NULL;
  Window *seq;
  Client *c;

  if (!getenv("DWM_RESTART"))
    return;
  if (XGetWindowProperty(dpy, root, sa, 0L, 100000L, False, XA_WINDOW,
                         &actual, &fmt, &n, &extra, &data) != Success || !data || fmt != 32)
    goto cleanup;
  seq = (Window *)data;

  /* rebuild stack list: iterate bottom→top so head-insert puts topmost at head */
  for (i = 0; i < n; i++)
    if ((c = wintoclient(seq[i]))) {
      detachstack(c);
      attachstack(c);
    }

  /* strictly restore X stacking order (bottom → top); stack frames,
     the saved ids are client windows */
  {
    XWindowChanges wc = {0};
    Window prev = 0;
    for (i = 0; i < n; i++) {
      if (!(c = wintoclient(seq[i])) || c->frame == None)
        continue;
      if (prev) {
        wc.sibling = prev;
        wc.stack_mode = Above;
        XConfigureWindow(dpy, c->frame, CWSibling | CWStackMode, &wc);
      }
      prev = c->frame;
    }
  }
  /* restore panel (bar/systray) vs client stacking: the new barwin and
     systray are mapped on top, so lower them below the bottommost
     fullscreen client (not the topmost one, or the bar would end up
     sandwiched between fullscreen clients), matching the normal-state
     model: tiled < bar/systray < fullscreen */
  {
    XWindowChanges wc = {0};
    Monitor *m2;
    Client *c2, *bottomfs;
    for (m2 = mons; m2; m2 = m2->next) {
      bottomfs = NULL;
      for (c2 = m2->stack; c2; c2 = c2->snext)
        if (ISVISIBLE(c2) && c2->isfullscreen)
          bottomfs = c2;
      if (bottomfs) {
        wc.sibling = bottomfs->frame;
        wc.stack_mode = Below;
        XConfigureWindow(dpy, m2->barwin, CWSibling | CWStackMode, &wc);
        if (showsystray && systray && systraytomon(m2) == m2)
          XConfigureWindow(dpy, systray->win, CWSibling | CWStackMode, &wc);
      }
    }
  }
  XSync(dpy, False);
  XDeleteProperty(dpy, root, sa);

cleanup:
  if (data)
    XFree(data);
  unsetenv("DWM_RESTART");
}

/* rebuild each monitor's client list in the saved tile order so the tile
   layout keeps its pre-restart ordering */
void restoreclientorder(void) {
  Monitor *m;
  Client *c, **orig = NULL;
  unsigned long i, j, n, orig_n;
  char name[64];

  for (m = mons; m; m = m->next) {
    Atom actual, ca;
    int fmt;
    unsigned long extra;
    unsigned char *data = NULL;
    Window *seq;

    snprintf(name, sizeof(name), "_DWM_CLIENTORDER_%d", m->num);
    ca = XInternAtom(dpy, name, False);
    if (XGetWindowProperty(dpy, root, ca, 0L, 100000L, False, XA_WINDOW,
                           &actual, &fmt, &n, &extra, &data) != Success ||
        !data || fmt != 32)
      goto cleanup;
    seq = (Window *)data;

    /* snapshot the current list so clients missing from the saved order
       keep their relative order and are appended afterwards */
    orig_n = 0;
    for (c = m->clients; c; c = c->next)
      orig_n++;
    orig = ecalloc(orig_n ? orig_n : 1, sizeof(*orig));
    for (c = m->clients, i = 0; c; c = c->next, i++)
      orig[i] = c;

    m->clients = NULL;
    for (i = 0; i < n; i++)
      for (j = 0; j < orig_n; j++)
        if (orig[j] && orig[j]->win == seq[i] && orig[j]->mon == m) {
          attachbottom(orig[j]);
          orig[j] = NULL;
          break;
        }
    for (i = 0; i < orig_n; i++)
      if (orig[i])
        attachbottom(orig[i]);
    free(orig);
    orig = NULL;

    /* re-tile so window geometry matches the restored order */
    arrangemon(m);

  cleanup:
    if (data)
      XFree(data);
    XDeleteProperty(dpy, root, ca);
  }
  updateclientlist();
}

/* restorestacking() clears m->sel mid-rebuild via detachstack();
   re-validate it so zoom()/pop() never see NULL/invisible sel. */
void restorefocus(void) {
  Monitor *m;
  Client *c;

  for (m = mons; m; m = m->next) {
    if (m->sel && m->sel->mon == m && ISVISIBLE(m->sel) && !HIDDEN(m->sel))
      continue;
    for (c = m->stack; c && (!ISVISIBLE(c) || HIDDEN(c)); c = c->snext)
      ;
    m->sel = c;
  }
  focus(NULL);
  arrange(NULL);
}

int main(int argc, char *argv[]) {
  if (argc == 2 && !strcmp("-v", argv[1]))
    die("dwm-" VERSION);
  else if (argc != 1)
    die("usage: dwm [-v]");
  if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
    fputs("warning: no locale support\n", stderr);
  if (!(dpy = XOpenDisplay(NULL)))
    die("dwm: cannot open display");
  checkotherwm();
  XrmInitialize();
  loadxrdb();
  setup();
#ifdef __OpenBSD__
  if (pledge("stdio rpath proc exec", NULL) == -1)
    die("pledge");
#endif /* __OpenBSD__ */
  scan();
  restorestacking();
  restoreclientorder();
  restorefocus();
  runautostart();
  run();
  if (restart) {
    // Store the client's current monitor, tag and floating state
    Atom da = XInternAtom(dpy, "_DWM_MONITOR", False);
    Atom dta = XInternAtom(dpy, "_DWM_TAG", False);
    Atom dfa = XInternAtom(dpy, "_DWM_FLOATING", False);
    Atom dfs = XInternAtom(dpy, "_DWM_FULLSCREEN", False);
    Monitor *m;
    Client *c;
    for (m = mons; m; m = m->next)
      for (c = m->clients; c; c = c->next) {
        unsigned long fl = c->isfloating;
        unsigned long fstate[7] = {c->isfullscreen, c->oldx, c->oldy, c->oldw,
                                   c->oldh, c->oldstate, c->oldbw};
        setstateprop(c->win, da, (unsigned long *)&m->num, 1);
        setstateprop(c->win, dta, (unsigned long *)&c->tags, 1);
        setstateprop(c->win, dfa, &fl, 1);
        setstateprop(c->win, dfs, fstate, c->isfullscreen ? 7 : 0);
      }
    // Store the stacking order (bottom → top) as client windows; on
    // restart the frames are recreated, so only client ids stay valid
    {
      Atom sa = XInternAtom(dpy, "_DWM_STACKING", False);
      Monitor *sm;
      Client *sc;
      XDeleteProperty(dpy, root, sa);
      for (sm = mons; sm; sm = sm->next)
        /* m->stack head is topmost: prepend so the property reads bottom → top */
        for (sc = sm->stack; sc; sc = sc->snext)
          XChangeProperty(dpy, root, sa, XA_WINDOW, 32, PropModePrepend,
                          (unsigned char *)&sc->win, 1);
    }
    // Store the tile order per monitor
    {
      char name[64];
      for (m = mons; m; m = m->next) {
        Client *c;
        snprintf(name, sizeof(name), "_DWM_CLIENTORDER_%d", m->num);
        Atom ca = XInternAtom(dpy, name, False);
        XDeleteProperty(dpy, root, ca);
        for (c = m->clients; c; c = c->next)
          XChangeProperty(dpy, root, ca, XA_WINDOW, 32, PropModeAppend,
                          (unsigned char *)&c->win, 1);
      }
    }
    setenv("DWM_RESTART", "1", 1);
    XCloseDisplay(dpy);
    // Store the monitor's current tag in the environment variable DWM_TAG_%d
    {
      char buf[32];
      for (m = mons; m; m = m->next) {
        snprintf(buf, sizeof(buf), "DWM_TAG_%d", m->num);
        char val[8];
        snprintf(val, sizeof(val), "%u", m->tagset[m->seltags]);
        setenv(buf, val, 1);
      }
    }
    execvp(argv[0], argv);
    die("dwm: execvp %s:", argv[0]);
  }
  cleanup();
  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
