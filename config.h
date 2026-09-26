#include <X11/X.h>
#include <X11/XF86keysym.h>

/*==========================================================================
 * dwm configuration, grouped by area; knobs are named <area>_<what>:
 *   theme   fonts, palette and per-scheme alpha
 *   window  borders, snapping, size hints, refresh rate and per-window rules
 *   layout  master size, presets, gaps and the layout list
 *   bar     geometry, zones, tags, tabs, titlebar, tray, hover previews
 *   keys    key/button bindings and the status click command
 *   policy  stack, hide and focus behaviour
 *========================================================================*/

/*--- theme: fonts, palette and per-scheme alpha -----------------------------*/

static const char *fonts[] = {
    "CaskaydiaCove Nerd Font:pixelsize=14:antialias=true;autohint=true",
    "Noto Sans Mono CJK SC:pixelsize=14:antialias=true;autohint=true",
};

static const char *fonts_highlight[] = {
    "CaskaydiaCove Nerd Font:pixelsize=14:weight=bold:slant=italic:antialias=true;autohint=true",
    "Noto Sans Mono CJK SC:pixelsize=14:weight=bold:antialias=true;autohint=true",
};

static char col_black[]    = "#073642";  /*  0: black    */
static char col_red[]      = "#dc322f";  /*  1: red      */
static char col_green[]    = "#859900";  /*  2: green    */
static char col_yellow[]   = "#b58900";  /*  3: yellow   */
static char col_blue[]     = "#268bd2";  /*  4: blue     */
static char col_magenta[]  = "#d33682";  /*  5: magenta  */
static char col_cyan[]     = "#2aa198";  /*  6: cyan     */
static char col_white[]    = "#eee8d5";  /*  7: white    */
static char col_ab_black[] = "#000000";
static char *colors[][3] = {
    /*                      fg            bg           border    */
    // tag
    [SchemeTagNorm]  = { col_white,    col_black,    col_black    },
    [SchemeTagSel]   = { col_black,    col_blue,     col_black    },
    // layout
    [SchemeLayout]   = { col_green,    col_black,    col_ab_black },
    // tabs
    [SchemeNorm]     = { col_white,    col_black,    col_ab_black },
    [SchemeSel]      = { col_black,    col_cyan,     col_cyan     },
    [SchemeHid]      = { col_white,    col_ab_black, col_ab_black },
    [SchemeTabIcons] = { col_white,    col_black,    col_white    },
    // status
    [SchemeStatus]   = { col_white,    col_black,    col_white    },
    // systray
    [SchemeSystray]  = { col_white,    col_black,    col_white    },
    // hover tooltip
    [SchemeTooltip]  = { col_blue,     col_black,    col_cyan     },
    // empty
    [SchemeEmpty]    = { col_ab_black, col_ab_black, col_black    },
};

#define OPAQUE        0xffU
#define TRANSPARENT   0x00U
#define BG_ALPHA      0xd0U
#define TAB_SEL_BG_ALPHA 0xe0U
#define TAB_HID_BG_ALPHA 0x66U

static const unsigned int alphas[][3]      = {
    /*                     fg            bg               border   */
    // tag
    [SchemeTagNorm]  = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    [SchemeTagSel]   = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // layout
    [SchemeLayout]   = { OPAQUE,      OPAQUE,           TRANSPARENT },
    // tabs
    [SchemeNorm]     = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    [SchemeSel]      = { OPAQUE,      TAB_SEL_BG_ALPHA, TAB_SEL_BG_ALPHA },
    [SchemeHid]      = { BG_ALPHA,    TAB_HID_BG_ALPHA, TRANSPARENT },
    [SchemeTabIcons] = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // Status
    [SchemeStatus]   = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // systray
    [SchemeSystray]  = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // hover tooltip
    [SchemeTooltip]  = { OPAQUE,      TAB_SEL_BG_ALPHA, OPAQUE      },
    // empty
    [SchemeEmpty]    = { TRANSPARENT, TRANSPARENT,      TRANSPARENT },
};

/*--- window: borders, snapping, size hints, refresh rate and rules ----------*/

static const unsigned int borderpx = 2;   /* border pixel of windows */
static const unsigned int snap     = 32;  /* snap pixel */

static const int resizehints    = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1;    /* 1 will force focus on the fullscreen window */
static const int refreshrate    = 144;  /* refresh rate (per second) for client move/resize */

static const Rule rules[] = {
    /* xprop(1):
     * WM_CLASS(STRING) = instance, class
     * WM_NAME(STRING) = title
     */
    /* class                instance    title     tags mask     isfloating    monitor       border width    notitle */
    /* notitle: 1 = never show the frame titlebar for this client (e.g. it draws its own);
       0 = auto (honor _MOTIF_WM_HINTS/_GTK_FRAME_EXTENTS). Trailing fields may be
       omitted and default to 0. Last matching rule wins, like the other fields. */
    {"firefox",             NULL,       NULL,     1 << 1,       0,            -1,          -1},
    {"chromium",            NULL,       NULL,     1 << 1,       0,            -1,          -1},
    {"Tor Browser",         NULL,       NULL,     1 << 1,       0,            -1,          -1},

    {"TelegramDesktop",     NULL,       NULL,     1 << 2,       0,            -1,          -1},
    {"wechat",              NULL,       NULL,     1 << 2,       0,            -1,           0},
    {"QQ",                  NULL,       NULL,     1 << 2,       0,            -1,          -1},

    {"DBeaver",             NULL,       NULL,     1 << 3,       0,            -1,          -1},
    {"resp",                NULL,       NULL,     1 << 3,       0,            -1,          -1},
    {"sqlitebrowser",       NULL,       NULL,     1 << 3,       0,            -1,          -1},

    {"xunlei",              NULL,       NULL,     1 << 5,       1,            -1,          -1},
    {"qBittorrent",         NULL,       NULL,     1 << 5,       0,            -1,          -1},

    {"obs",                 NULL,       NULL,     1 << 6,       0,            -1,          -1},

    {"qqmusic",             NULL,       NULL,     1 << 7,       1,            -1,          -1},
    {"netease-cloud-music", NULL,       NULL,     1 << 7,       1,            -1,          -1},
    {"OSD Lyrics",          NULL,       NULL,     1 << 7,       1,            -1,          -1},

    {"steam",               NULL,       NULL,     1 << 8,       0,            -1,          -1},
    {"heroic",              NULL,       NULL,     1 << 8,       0,            -1,          -1},

    // all tags
    {"bilichat-tui",        NULL,       NULL,     (1<<10)-1,    1,            -1,          -1},

    // other only floating
    {"float-term",          NULL,       NULL,     0,            1,            -1,          -1},
    {"Godot_Engine",        NULL,       NULL,     0,            1,            -1,          -1},
    {"vlc",                 NULL,       NULL,     0,            1,            -1,          -1},
    {"mpv",                 NULL,       NULL,     0,            1,            -1,          -1},
    {"feh",                 NULL,       NULL,     0,            1,            -1,          -1},
    {"viewnior",            NULL,       NULL,     0,            1,            -1,          -1},
    {"peek",                NULL,       NULL,     0,            1,            -1,           0},
    {"flameshot",           NULL,       NULL,     0,            1,            -1,           0},
    {"scrcpy",              NULL,       NULL,     0,            1,            -1,          -1},
    {"Yad",                 NULL,       NULL,     0,            1,            -1,          -1},
    {"zenity",              NULL,       NULL,     0,            1,            -1,          -1},
    // wps
    {"wpsoffice",           NULL,       NULL,     0,            1,            -1,          -1},
    {"wpspdf",              NULL,       NULL,     0,            1,            -1,          -1},
    {"wps",                 NULL,       NULL,     0,            1,            -1,          -1},
    {"wpp",                 NULL,       NULL,     0,            1,            -1,          -1},
    {"et",                  NULL,       NULL,     0,            1,            -1,          -1},
    {"wemeetapp",           NULL,       NULL,     0,            0,            -1,           0},
};

/*--- layout: master size, presets, gaps and the layout list -----------------*/

static const float mfact           = 0.5;  /* factor of master area size [0.05..0.95] */
static const   int nmaster         = 1;    /* number of clients in master area */
static const float mfact_presets[] = { 0.2, 0.33333, 0.5, 0.66667, 0.8 }; /* Mod+R: cycle master area width preset */
static const float cfact_presets[] = { 0.5, 1.0, 2.0 };         /* Mod+Shift+R: cycle focused window size preset */

/* gaps: inner/outer spacing around tiled clients (vanitygaps) */
static const unsigned int gappih    = 6;
static const unsigned int gappiv    = 6;
static const unsigned int gappoh    = 14;
static const unsigned int gappov    = 12;
static                int smartgaps = 0;  /* 1 means no outer gap when there is only one window */

#define FORCE_VSPLIT 1 /* nrowgrid layout: force two clients to always split vertically */
#include "vanitygaps.c"

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=",      tile },    /* first entry is default */
    { "><>",      NULL },    /* no layout function means floating behavior */
    { "[M]",      monocle },
    { "[@]",      spiral },
    { "[\\]",     dwindle },
    { "H[]",      deck },
    { "TTT",      bstack },
    { "===",      bstackhoriz },
    { "HHH",      grid },
    { "###",      nrowgrid },
    { "---",      horizgrid },
    { ":::",      gaplessgrid },
    { "|M|",      centeredmaster },
    { ">M>",      centeredfloatingmaster },
};

/*--- bar: geometry, tray, zones, tags, tabs, titlebar, hover previews -------*/

static const int showbar = 1;  /* 0 means no bar */
static const int topbar  = 1;  /* 0 means bottom bar */
/* BarModeFlat: opaque flat bar with the systray merged into the barwin (needs
 * a real 24-bit TrueColor visual); BarModeArgb: 32-bit ARGB bar with a
 * separate systray window */
typedef enum { BarModeArgb, BarModeFlat } BarMode;
static const      BarMode bar_mode     = BarModeArgb;
static const          int bar_fontpad  = 8;
static const          int bar_pad_v    = 2;  /* vertical padding of bar */
static const          int bar_pad_h    = 2;  /* horizontal padding of bar */
static const unsigned int bar_borderpx = 1;  /* bar and pill outline width in px; 0 = no outline */

#define ICONSIZE (bh - 2 * bar_borderpx - 4) /* or adaptively preserve 2 pixels each side */
#define ICONSPACING 4 /* space between icon and title */

/* systray: XEMBED tray, merged into the bar or a separate window */
static const          int tray_show               = 1;  /* 0 means no systray */
static const unsigned int tray_pinning            = 1;  /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int tray_spacing            = 4;  /* systray spacing */
static const          int tray_pinning_fail_first = 1;  /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const          int tray_pad                = 4;
static const         char *tray_order[]           = { "fcitx", "...", "easyeffects", "blueman", "nm-applet", "pasystray", "udiskie", NULL };

/* bar modules: items are grouped into three zones and each zone is filled
 * in array order. left starts at the bar's left edge, right ends at its
 * right edge (the systray is reserved there) and center is sized to its own
 * content and centred on the monitor's middle (the systray is not part of that
 * middle), clamped so it never runs over a side zone.
 * BarTabs stretches when its fixed width does not fit, so keep it last.
 * Every status block is its own item, ST(Cpu), and consecutive
 * BarTags/BarLayout/BarStatus items share one pill; { BarPillBreak, 0 }
 * starts a new pill. BarTabs always starts its own pill and never joins.
 * A St* id the writer leaves out (empty panel, portrait cut) draws nothing.
 * NOTE: StVolume is not listed in any zone although the writer still emits
 * it; add a ST(Volume) item to show it. */
static const BarItem bar_left[] = {
  { BarLayout, 0 }, { BarTags, 0 },
  { BarPillBreak, 0 },
  ST(Cpu), ST(Mem), ST(Disk),
  { BarPillBreak, 0 },
  ST(Net)
};
static const BarItem bar_center[] = { { BarTabs, 0 } };
static const BarItem bar_right[]  = {
  ST(Rss), ST(Mail), ST(Notify),
  { BarPillBreak, 0 },
  ST(Weather),
  { BarPillBreak, 0 },
  ST(Date),
  { BarPillBreak, 0 },
  ST(Screencast), ST(Singbox), ST(Mpd), ST(Battery)
};

/* portrait monitors (wh > ww) use these zones instead; orientation is a config
 * choice now, so a different set of status ids belongs here too (the writer no
 * longer emits a cut marker) */
static const BarItem bar_left_portrait[] = {
  { BarLayout, 0 }, { BarTags, 0 },
  { BarPillBreak, 0 },
  ST(Cpu), ST(Mem)
};
static const BarItem bar_center_portrait[] = { { BarTabs, 0 } };
static const BarItem bar_right_portrait[]  = {
  ST(Rss), ST(Mail), ST(Notify),
  { BarPillBreak, 0 },
  ST(Weather),
  { BarPillBreak, 0 },
  ST(Date),
  { BarPillBreak, 0 },
  ST(Screencast), ST(Singbox), ST(Mpd), ST(Battery)
};

/* tags[] only defines the number of tags (TAGMASK depends on LENGTH(tags)); its text is not rendered. */
static const char tag_text[]   = "{icon} {name}";  /* tag text template; placeholders: {name}, {icon}, {index} */
static const char *tags[]      = {"", "", "󰭹", "", "", "", "", "", ""};
static const char *tag_names[] = {"dev", "web", "chat", "util", "misc", "dl", "vid", "mus", "game"};

/* pill: shared geometry for every bar pill (tags, layout, status, tabs).
   pill_radius is clamped to lpad at startup (pillr); pill_gap separates
   pills, zones and TabModeIconTitle pills. */
static const unsigned int pill_radius = 8;
static const unsigned int pill_gap    = 6;

static const unsigned int tab_width    = 16;         /* TabModeIconTitle: pill width in characters */
static const         char tab_text[]   = "{title}";  /* TabModeIconTitle only; {title}, {class} */
static const unsigned int tab_icon_gap = 8;          /* TabModeIcons only: space in px between icons in the shared pill */

/* tab rendering mode:
 * TabModeIconTitle: one pill per client, icon + title (tab_text), tab_width wide
 * TabModeIcons:     every icon in a single shared pill, tab_text is ignored
 * Starts in this mode; MODKEY|ShiftMask+b (toggletabmode) flips it at runtime. */
typedef enum { TabModeIconTitle, TabModeIcons } TabMode;
static TabMode tab_mode = TabModeIcons;

/* how TabModeIconTitle sizes its pills:
 * TabFit   keep tab_width while the row fits, level it to the zone otherwise
 * TabFill  always level the row to the zone
 * TabFixed always keep tab_width (pills that would not fit are dropped) */
typedef enum { TabFit, TabFill, TabFixed } TabSize;
static const TabSize tab_size = TabFit;

/* TabModeIcons selection mark: a filled circle in SchemeTabIcons foreground,
 * centred under the selected client's icon. This is the radius in px; 0 turns
 * it off. 5..10 reads well and the tab icons shrink to leave room for it. */
static const unsigned int tab_sel_dot = 2;

static const char tab_icon_path[] = "$HOME/.dwm/tab-fallback.png";  /* for clients without _NET_WM_ICON; missing file = no icon */

/* titlebar: per-client decorations */
static const          int title_show      = 1;          /* 0 means no titlebar, 1 shows a per-client titlebar (height = font height + 2 * title_pad) */
static const          int title_hide_fullscreen = 1;   /* 1 = hide the titlebar in fullscreen (Mod+Shift+f: monocle layout with the bar hidden); [M] and maximize (Mod+f) keep theirs; floating windows keep theirs */
static const unsigned int title_pad       = 6;          /* per-side padding; titlebar height = font height + 2 * title_pad */
static const          int title_show_icon = 1;          /* 1 = draw the client icon at the left of the titlebar, 0 = text only */
static const          int title_align     = 1;          /* text alignment, icon stays left: 0 = left, 1 = center (true center of full width), 2 = right */
static const         char title_text[]    = "{title}";  /* titlebar text template; placeholders: {title}, {class} (same as tab_text) */
static const         char *title_btns[]   = { "", "", "" }; /* titlebar buttons, left to right: minimize, maximize, close. The order is fixed (0 = hide, 1 = togglefloating, 2 = killclient); only the labels are configurable. */

// tag and client preview, off by default: enabling it makes every tag switch
// snapshot the whole monitor (for the tag hover preview) and the client-tab
// tooltip re-composite its window while hovered
static const          int hover_previews         = 0;    /* 1 = tag snapshots + client-tab hover tooltip; 0 = disabled */
static const unsigned int hover_delay            = 500;  /* ms of resting on a tab before the tooltip appears */
static const unsigned int hover_preview_h        = 240;  /* max live preview height; width scales by aspect */
static const unsigned int hover_preview_refresh  = 300;  /* ms between live preview refreshes */
static const unsigned int hover_pad              = 6;    /* tooltip content padding from the border */
static const unsigned int hover_gap              = 4;    /* gap between the preview and the title */
static const unsigned int hover_preview_borderpx = 2;    /* px highlight border around the live preview, in colors[x][2] */

/*--- keys: key/button bindings and the status click command -----------------*/

#define MODKEY Mod4Mask
#define SUPKEY Mod1Mask
#define TAGKEYS(KEY, TAG)                                                 \
    {MODKEY,                           KEY, view,       {.ui = 1 << TAG}},  \
    {MODKEY | ShiftMask,               KEY, toggleview, {.ui = 1 << TAG}},  \
    {MODKEY | ControlMask,             KEY, tag,        {.ui = 1 << TAG}},  \
    {MODKEY | ControlMask | ShiftMask, KEY, toggletag,  {.ui = 1 << TAG}},

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char *[]) { "/bin/sh", "-c", cmd, NULL } }
#define LAUNCHCMD(...) { .v = (const char *[]) { "/bin/sh", "-c", "$HOME/.dwm/dwm-launcher.sh \"$@\"", "dwm-launcher", __VA_ARGS__, NULL } }

// don't change or surround it by {}
static const char *layoutmenu_cmd = "$HOME/.dwm/dwm-layoutmenu.sh";

static Key keys[] = {
    /* modifier                    key                       function        argument */
    {MODKEY,                       XK_t,                     spawn,          LAUNCHCMD("term")},
    {MODKEY,                       XK_n,                     spawn,          LAUNCHCMD("term", "float")},
    {MODKEY,                       XK_e,                     spawn,          LAUNCHCMD("fm")},
    {0,                            XF86XK_AudioLowerVolume,  spawn,          SHCMD("$HOME/.dwm/tools/volume.sh down")},
    {0,                            XF86XK_AudioRaiseVolume,  spawn,          SHCMD("$HOME/.dwm/tools/volume.sh up")},
    {0,                            XF86XK_AudioMute,         spawn,          SHCMD("$HOME/.dwm/tools/volume.sh toggle")},
    {0,                            XF86XK_MonBrightnessDown, spawn,          SHCMD("$HOME/.dwm/tools/brightness.sh down")},
    {0,                            XF86XK_MonBrightnessUp,   spawn,          SHCMD("$HOME/.dwm/tools/brightness.sh up")},
    // rofi
    {MODKEY,                       XK_d,                     spawn,          LAUNCHCMD("apps")},
    {MODKEY|ControlMask,           XK_d,                     spawn,          LAUNCHCMD("powermenu")},
    {MODKEY,                       XK_w,                     spawn,          LAUNCHCMD("windows")},
    {MODKEY|ShiftMask,             XK_w,                     spawn,          LAUNCHCMD("wallpaper")},
    // screenshot
    {MODKEY,                       XK_a,                     spawn,          LAUNCHCMD("screenshot", "pure")},
    {MODKEY|ShiftMask,             XK_a,                     spawn,          LAUNCHCMD("screenshot")},
    // common modules
    {MODKEY,                       XK_m,                     spawn,          LAUNCHCMD("modules")},
    {MODKEY|ShiftMask,             XK_m,                     spawn,          LAUNCHCMD("mpd")},
    // layout
    // {MODKEY,                       XK_t,                     setlayout,      {.v = &layouts[0]}},
    // {MODKEY,                       XK_f,                     setlayout,      {.v = &layouts[1]}},
    {MODKEY|ShiftMask,             XK_t,                     layoutmenu,     {0}},
    // focus follows master: focused window holds sole master slot in tile
    {MODKEY,                       XK_z,                     focusmaster,    {0}},
    {MODKEY,                       XK_f,                     maximize,       {0}},
    {MODKEY|ShiftMask,             XK_f,                     fullscreen,     {0}},
    // layout adjust
    {MODKEY|ControlMask,           XK_v,                     incnmaster,     {.i = +1}},
    {MODKEY|ControlMask,           XK_s,                     incnmaster,     {.i = -1}},
    // preset switching (niri-style, forward-only)
    {MODKEY,                       XK_r,                     cyclemfact,     {0}},
    {MODKEY|ShiftMask,             XK_r,                     cyclecfact,     {0}},
    // client manager
    {MODKEY,                       XK_b,                     togglebar,      {0}},
    {MODKEY|ShiftMask,             XK_b,                     toggletabmode,  {0}},
    {MODKEY,                       XK_j,                     focusstackvis,  {.i = +1}},
    {MODKEY,                       XK_k,                     focusstackvis,  {.i = -1}},
    {MODKEY|ShiftMask,             XK_j,                     focusstackhid,  {.i = +1}},
    {MODKEY|ShiftMask,             XK_k,                     focusstackhid,  {.i = -1}},
    {MODKEY,                       XK_s,                     show,           {0}},
    {MODKEY,                       XK_h,                     hide,           {0}},
    {MODKEY|ShiftMask,             XK_s,                     showall,        {0}},
    {MODKEY|ControlMask,           XK_space,                 togglefloating, {0}},
    {MODKEY,                       XK_Return,                zoom,           {0}},
    {MODKEY,                       XK_Tab,                   view,           {0}},
    {MODKEY,                       XK_0,                     view,           {.ui = ~0}},
    {MODKEY|ShiftMask,             XK_0,                     tag,            {.ui = ~0}},
    {MODKEY|ControlMask,           XK_0,                     toggletag,      {.ui = ~0}},
    {MODKEY,                       XK_comma,                 focusmon,       {.i = -1}},
    {MODKEY,                       XK_period,                focusmon,       {.i = +1}},
    {MODKEY|ControlMask,           XK_comma,                 tagmon,         {.i = -1}},
    {MODKEY|ControlMask,           XK_period,                tagmon,         {.i = +1}},
    {MODKEY,                       XK_q,                     killclient,     {0}},
    {MODKEY|ControlMask,           XK_q,                     quit,           {1}}, // hot restart
    {MODKEY|ShiftMask|ControlMask, XK_q,                     quit,           {0}}, // kill dwm
    // gap manager
    {MODKEY|SUPKEY,                XK_u,                     incrgaps,       {.i = +1}},
    {MODKEY|SUPKEY,                XK_i,                     incrigaps,      {.i = +1}},
    {MODKEY|SUPKEY,                XK_o,                     incrogaps,      {.i = +1}},
    {MODKEY|SUPKEY,                XK_6,                     incrihgaps,     {.i = +1}},
    {MODKEY|SUPKEY,                XK_7,                     incrivgaps,     {.i = +1}},
    {MODKEY|SUPKEY,                XK_8,                     incrohgaps,     {.i = +1}},
    {MODKEY|SUPKEY,                XK_9,                     incrovgaps,     {.i = +1}},
    {MODKEY|SUPKEY,                XK_0,                     togglegaps,     {0}},
    {MODKEY|SUPKEY|ShiftMask,      XK_u,                     incrgaps,       {.i = -1}},
    {MODKEY|SUPKEY|ShiftMask,      XK_i,                     incrigaps,      {.i = -1}},
    {MODKEY|SUPKEY|ShiftMask,      XK_o,                     incrogaps,      {.i = -1}},
    {MODKEY|SUPKEY|ShiftMask,      XK_6,                     incrihgaps,     {.i = -1}},
    {MODKEY|SUPKEY|ShiftMask,      XK_7,                     incrivgaps,     {.i = -1}},
    {MODKEY|SUPKEY|ShiftMask,      XK_8,                     incrohgaps,     {.i = -1}},
    {MODKEY|SUPKEY|ShiftMask,      XK_9,                     incrovgaps,     {.i = -1}},
    {MODKEY|SUPKEY|ShiftMask,      XK_0,                     defaultgaps,    {0}},
    TAGKEYS(XK_1, 0)
    TAGKEYS(XK_2, 1)
    TAGKEYS(XK_3, 2)
    TAGKEYS(XK_4, 3)
    TAGKEYS(XK_5, 4)
    TAGKEYS(XK_6, 5)
    TAGKEYS(XK_7, 6)
    TAGKEYS(XK_8, 7)
    TAGKEYS(XK_9, 8)
};

// status click event command
static const char *statuscmd[] = {"/bin/sh", "-c", "$HOME/.dwm/dwm-statuscmd.sh $INDEX $BUTTON", NULL};

/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, ClkTitleBar, or ClkRootWin */
static Button buttons[] = {
    /* click          event   mask     button          function argument */
    // tag
    {  ClkTagBar,     0,      Button1, view,           {0}},
    {  ClkTagBar,     0,      Button3, toggleview,     {0}},
    {  ClkTagBar,     MODKEY, Button1, tag,            {0}},
    {  ClkTagBar,     MODKEY, Button3, toggletag,      {0}},
    // layout
    {  ClkLtSymbol,   0,      Button1, setlayout,      {0}},
    {  ClkLtSymbol,   0,      Button3, layoutmenu,     {0}},
    // task
    {  ClkWinTitle,   0,      Button1, togglewin,      {0}},
    {  ClkWinTitle,   0,      Button2, killclient,     {0}},
    {  ClkWinTitle,   0,      Button3, zoom,           {0}},
    // status
    {  ClkStatusText, 0,      Button1, spawn,          {.v = statuscmd}},
    {  ClkStatusText, 0,      Button2, spawn,          {.v = statuscmd}},
    {  ClkStatusText, 0,      Button3, spawn,          {.v = statuscmd}},
    {  ClkStatusText, 0,      Button4, spawn,          {.v = statuscmd}},
    {  ClkStatusText, 0,      Button5, spawn,          {.v = statuscmd}},
    // window         client
    {  ClkClientWin,  MODKEY, Button1, movemouse,      {0}},
    {  ClkClientWin,  MODKEY, Button2, togglefloating, {0}},
    {  ClkClientWin,  MODKEY, Button3, resizemouse,    {0}},
    // titlebar       (per-client titlebar, only active when title_show = 1)
    {  ClkTitleBar,   0,      Button1, movemouse,      {0}},
    {  ClkTitleBar,   0,      Button3, togglefloating, {0}},
};

/*--- policy: stack, hide and focus behaviour --------------------------------*/

static const          int policy_show_hidden      = 1;  /* 1 = focusstackhid shows hidden windows permanently; 0 = preview, re-hide on switch away */
static const unsigned int policy_attach_top       = 0;  /* new window is attached to the top of the stack */
static const          int policy_focus_on_move    = 1;  /* switch view and focus follow the client moved by tag/tagmon */
static const          int policy_jump_on_activate = 1;  /* 1 = _NET_ACTIVE_WINDOW (e.g. rofi -show window) jumps to the window's tag/monitor; 0 = only mark it urgent */
