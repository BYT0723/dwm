#include <X11/X.h>
#include <X11/XF86keysym.h>

/* appearance */
static const unsigned int borderpx     = 0;   /* border pixel of windows */
static const unsigned int snap         = 32;  /* snap pixel */
static const          int showbar      = 1;   /* 0 means no bar */
static const          int topbar       = 1;   /* 0 means bottom bar */
static const          int barfontpad   = 8;
static const          int vertpad      = 2;  /* vertical padding of bar */
static const          int sidepad      = 2;  /* horizontal padding of bar */
static const          char host[]      = "";

/* tab style; 0:default 1:center 2:custom_width
 * 0 0 0 0 0 0 0 0
 * bit1: center
 * bit2: custom_width
 * radius is controlled by barinnerradius
 */
#define TAB_NONE         0x00
#define TAB_CENTER       0x01
#define TAB_CUSTOM_WIDTH 0x02

static const unsigned int tabstyle    = TAB_CENTER | TAB_CUSTOM_WIDTH;
static const unsigned int tabwidth    = 20;
static const char         tabtext[]   = "{title}"; /* tab text template; placeholders: {title}, {class} */
static const unsigned int tabradius   = 8;
static const unsigned int tabgap      = 4;
static const unsigned int tabborder   = 1; /* tab outline width in px; 0 = no outline */
static const          int autoshowhid = 1; /* 1 = focusstackhid shows hidden windows permanently; 0 = preview, re-hide on switch away */

static const          int hoverinfo      = 1;    /* 1 = hover a client tab to show a tooltip with client info; 0 = disabled */
static const unsigned int hoverdelay     = 500; /* ms of resting on a tab before the tooltip appears */
static const unsigned int previewh       = 240;  /* max live preview height; width scales by aspect */
static const unsigned int previewrefresh = 300; /* ms between live preview refreshes */
static const unsigned int hoverpad       = 12;   /* tooltip content padding from the border */
static const unsigned int hovergap       = 8;  /* gap between the preview and the title */
static const unsigned int previewborder  = 2;   /* px highlight border around the live preview, in colors[x][2] */

static const unsigned int gappih    = 6;
static const unsigned int gappiv    = 6;
static const unsigned int gappoh    = 12;
static const unsigned int gappov    = 12;
static                int smartgaps = 0;   /* 1 means no outer gap when there is only one window */

static const unsigned int systraypinning          = 0;  /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing          = 4;  /* systray spacing */
static const          int systraypinningfailfirst = 1;  /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const          int showsystray             = 1;  /* 0 means no systray */
static const          int systraypad              = 4;
/* systray icon order, left to right; "..." = slot for unlisted icons */
static const char     *systrayorder[]            = { "fcitx", "...", "easyeffects", "nm-applet", "udiskie", NULL };

static const unsigned int attachtop = 0; /* new window is attached to the top of the stack */

static const Bool viewontag = True; /* Switch view on tag switch */

static const Bool focusonmove = True; /* focus follows the client moved by tag/tagmon */

static const int jump_on_activate = 1; /* 1 = _NET_ACTIVE_WINDOW (e.g. rofi -show window) jumps to the window's tag/monitor; 0 = only mark it urgent */

#define ICONSIZE (bh - 2) /* or adaptively preserve 2 pixels each side */
#define ICONSPACING 6 /* space between icon and title */
static const char *fonts[] = {
    "Symbols Nerd Font Mono:pixelsize=14:antialias=true;autohint=true",
    "CaskaydiaCove Nerd Font:pixelsize=14:antialias=true;autohint=true",
    "Noto Sans Mono CJK SC:pixelsize=14:antialias=true;autohint=true",
};

static const char *fonts_highlight[] = {
    "Symbols Nerd Font Mono:pixelsize=14:antialias=true;autohint=true",
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
    /*                    fg            bg            border   */
    // host
    [SchemeHost]    = { col_blue,     col_black,    col_black    },
    // tag
    [SchemeTagNorm] = { col_white,    col_black,    col_black    },
    [SchemeTagSel]  = { col_black,    col_blue,     col_black    },
    // layout
    [SchemeLayout]  = { col_green,    col_black,    col_ab_black },
    // tasks
    [SchemeNorm]    = { col_white,    col_black,    col_black    },
    [SchemeSel]     = { col_blue,     col_black,    col_cyan     },
    [SchemeHid]     = { col_white,    col_ab_black, col_black    },
    // status
    [SchemeStatus]  = { col_white,    col_black,    col_white    },
    // systray
    [SchemeSystray] = { col_white,    col_black,    col_black    },
    // hover tooltip
    [SchemeTooltip] = { col_blue,     col_black,    col_cyan     },
    // empty
    [SchemeEmpty]   = { col_ab_black, col_ab_black, col_black    },
};

#define OPAQUE        0xffU
#define TRANSPARENT   0x00U
#define BG_ALPHA      0xd0U
#define TAB_SEL_BG_ALPHA 0xe0U
#define TAB_HID_BG_ALPHA 0x66U

static const unsigned int alphas[][3]      = {
    /*                    fg         bg                border     */
    // host
    [SchemeHost]    = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // tag
    [SchemeTagNorm] = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    [SchemeTagSel]  = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // layout
    [SchemeLayout]  = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // tab
    [SchemeNorm]    = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    [SchemeSel]     = { OPAQUE,      TAB_SEL_BG_ALPHA, TAB_SEL_BG_ALPHA },
    [SchemeHid]     = { BG_ALPHA,    TAB_HID_BG_ALPHA, TRANSPARENT },
    // Status
    [SchemeStatus]  = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // systray
    [SchemeSystray] = { OPAQUE,      BG_ALPHA,         TRANSPARENT },
    // hover tooltip
    [SchemeTooltip] = { OPAQUE,      TAB_SEL_BG_ALPHA, OPAQUE      },
    // empty
    [SchemeEmpty]   = { TRANSPARENT, TRANSPARENT,      TRANSPARENT },
};

/* tagging */
/* tags[] only defines the number of tags (TAGMASK depends on LENGTH(tags)); its text is not rendered. */
static const char tagtext[] = "{icon} {name}"; /* tag text template; placeholders: {name}, {icon}, {index} */
static const char *tags[] = {"", "", "󰭹", "", "", "", "", "", ""};
static const char *tag_names[] = {"dev", "web", "chat", "util", "misc", "dl", "vid", "mus", "game"};

static const Rule rules[] = {
    /* xprop(1):
     * WM_CLASS(STRING) = instance, class
     * WM_NAME(STRING) = title
     */
    /* class                instance    title     tags mask     isfloating    monitor */
    {"firefox",             NULL,       NULL,     1 << 1,       0,            -1},
    {"chromium",            NULL,       NULL,     1 << 1,       0,            -1},
    {"Tor Browser",         NULL,       NULL,     1 << 1,       0,            -1},

    {"TelegramDesktop",     NULL,       NULL,     1 << 2,       0,            -1},
    {"wechat",              NULL,       NULL,     1 << 2,       0,            -1},
    {"QQ",                  NULL,       NULL,     1 << 2,       0,            -1},

    {"DBeaver",             NULL,       NULL,     1 << 3,       0,            -1},
    {"resp",                NULL,       NULL,     1 << 3,       0,            -1},
    {"sqlitebrowser",       NULL,       NULL,     1 << 3,       0,            -1},

    {"xunlei",              NULL,       NULL,     1 << 5,       1,            -1},
    {"qBittorrent",         NULL,       NULL,     1 << 5,       0,            -1},

    {"obs",                 NULL,       NULL,     1 << 6,       0,            -1},

    {"qqmusic",             NULL,       NULL,     1 << 7,       1,            -1},
    {"netease-cloud-music", NULL,       NULL,     1 << 7,       1,            -1},
    {"OSD Lyrics",          NULL,       NULL,     1 << 7,       1,            -1},

    {"steam",               NULL,       NULL,     1 << 8,       0,            -1},
    {"heroic",              NULL,       NULL,     1 << 8,       0,            -1},

    // all tags
    {"bilichat-tui",        NULL,       NULL,     (1<<10)-1,    1,            -1},

    // other only floating
    {"float-term",          NULL,       NULL,     0,            1,            -1},
    {"Godot_Engine",        NULL,       NULL,     0,            1,            -1},
    {"vlc",                 NULL,       NULL,     0,            1,            -1},
    {"mpv",                 NULL,       NULL,     0,            1,            -1},
    {"feh",                 NULL,       NULL,     0,            1,            -1},
    {"viewnior",            NULL,       NULL,     0,            1,            -1},
    {"peek",                NULL,       NULL,     0,            1,            -1},
    {"flameshot",           NULL,       NULL,     0,            1,            -1},
    {"scrcpy",              NULL,       NULL,     0,            1,            -1},
    {"Yad",                 NULL,       NULL,     0,            1,            -1},
    {"zenity",              NULL,       NULL,     0,            1,            -1},
    // wps
    {"wpsoffice",           NULL,       NULL,     0,            1,            -1},
    {"wpspdf",              NULL,       NULL,     0,            1,            -1},
    {"wps",                 NULL,       NULL,     0,            1,            -1},
    {"wpp",                 NULL,       NULL,     0,            1,            -1},
    {"et",                  NULL,       NULL,     0,            1,            -1},
};

/* layout(s) */
static const float mfact          = 0.55; /* factor of master area size [0.05..0.95] */
static const int   nmaster        = 1;    /* number of clients in master area */
static const int   resizehints    = 1; /* 1 means respect size hints in tiled resizals */
static const int   lockfullscreen = 1; /* 1 will force focus on the fullscreen window */
static const int   refreshrate    = 120; /* refresh rate (per second) for client move/resize */

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

/* key definitions */
#define MODKEY Mod4Mask
#define SUPKEY Mod1Mask
#define TAGKEYS(KEY, TAG)                                                 \
    {MODKEY,                           KEY, view,       {.ui = 1 << TAG}},  \
    {MODKEY | ShiftMask,               KEY, toggleview, {.ui = 1 << TAG}},  \
    {MODKEY | ControlMask,             KEY, tag,        {.ui = 1 << TAG}},  \
    {MODKEY | ControlMask | ShiftMask, KEY, toggletag,  {.ui = 1 << TAG}},

/* toggle the tag preview; TAG is the 0-based tag index */
#define PREVIEWTAGKEYS(KEY, TAG) \
    {SUPKEY, KEY, previewtag, {.ui = TAG}},

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char *[]) { "/bin/sh", "-c", cmd, NULL } }
#define LAUNCHCMD(...) { .v = (const char *[]) { "/bin/sh", "-c", "$HOME/.dwm/dwm-launcher.sh \"$@\"", "dwm-launcher", __VA_ARGS__, NULL } }

// don't change or surround it by {}
static const char *layoutmenu_cmd   = "$HOME/.dwm/dwm-layoutmenu.sh";

static Key keys[] = {
    /* modifier                    key                       function        argument */
    {MODKEY,                       XK_Return,                spawn,          LAUNCHCMD("term")},
    {MODKEY,                       XK_n,                     spawn,          LAUNCHCMD("term", "float")},
    {MODKEY,                       XK_e,                     spawn,          LAUNCHCMD("fm")},
    {0,                            XF86XK_AudioLowerVolume,  spawn,          SHCMD("$HOME/.dwm/tools/volume.sh down")},
    {0,                            XF86XK_AudioRaiseVolume,  spawn,          SHCMD("$HOME/.dwm/tools/volume.sh up")},
    {0,                            XF86XK_AudioMute,         spawn,          SHCMD("$HOME/.dwm/tools/volume.sh toggle")},
    {0,                            XF86XK_MonBrightnessDown, spawn,          SHCMD("$HOME/.dwm/tools/brightness.sh down")},
    {0,                            XF86XK_MonBrightnessUp,   spawn,          SHCMD("$HOME/.dwm/tools/brightness.sh up")},
    // rofi
    {MODKEY,                       XK_w,                     spawn,          LAUNCHCMD("windows")},
    {MODKEY,                       XK_a,                     spawn,          LAUNCHCMD("screenshot", "pure")},
    {MODKEY|ShiftMask,             XK_a,                     spawn,          LAUNCHCMD("screenshot")},
    {MODKEY|ShiftMask,             XK_r,                     spawn,          LAUNCHCMD("screencast")},
    {MODKEY,                       XK_d,                     spawn,          LAUNCHCMD("apps")},
    {MODKEY,                       XK_m,                     spawn,          LAUNCHCMD("modules")},
    {MODKEY|ShiftMask,             XK_w,                     spawn,          LAUNCHCMD("wallpaper")},
    {MODKEY|ShiftMask,             XK_m,                     spawn,          LAUNCHCMD("mpd")},
    {MODKEY|ControlMask,           XK_m,                     spawn,          LAUNCHCMD("powermenu")},
    // layout
    {MODKEY,                       XK_t,                     setlayout,      {.v = &layouts[0]}},
    {MODKEY,                       XK_f,                     setlayout,      {.v = &layouts[1]}},
    {MODKEY|ShiftMask,             XK_t,                     layoutmenu,     {0}},
    // layout adjust
    {MODKEY,                       XK_v,                     incnmaster,     {.i = +1}},
    {MODKEY,                       XK_s,                     incnmaster,     {.i = -1}},
    {MODKEY,                       XK_h,                     setmfact,       {.f = -0.01}},
    {MODKEY,                       XK_l,                     setmfact,       {.f = +0.01}},
    {MODKEY|ControlMask,           XK_h,                     setcfact,       {.f = +0.05}},
    {MODKEY|ControlMask,           XK_l,                     setcfact,       {.f = -0.05}},
    {MODKEY|ControlMask,           XK_o,                     setcfact,       {.f = 0.00}},
    // client manager
    {MODKEY,                       XK_b,                     togglebar,      {0}},
    {MODKEY,                       XK_j,                     focusstackvis,  {.i = +1}},
    {MODKEY,                       XK_k,                     focusstackvis,  {.i = -1}},
    {MODKEY|ShiftMask,             XK_j,                     focusstackhid,  {.i = +1}},
    {MODKEY|ShiftMask,             XK_k,                     focusstackhid,  {.i = -1}},
    {MODKEY|ShiftMask,             XK_s,                     show,           {0}},
    {MODKEY|ControlMask,           XK_s,                     showall,        {0}},
    {MODKEY|ShiftMask,             XK_h,                     hide,           {0}},
    {MODKEY|ShiftMask,             XK_f,                     fullscreen,     {0}},
    {MODKEY|ControlMask,           XK_space,                 togglefloating, {0}},
    {MODKEY|ControlMask,           XK_Return,                zoom,           {0}},
    {MODKEY,                       XK_Tab,                   view,           {0}},
    {MODKEY,                       XK_0,                     view,           {.ui = ~0}},
    {MODKEY|ShiftMask,             XK_0,                     tag,            {.ui = ~0}},
    {MODKEY|ControlMask,           XK_0,                     toggletag,      {.ui = ~0}},
    {MODKEY,                       XK_comma,                 focusmon,       {.i = -1}},
    {MODKEY,                       XK_period,                focusmon,       {.i = +1}},
    {MODKEY|ControlMask,           XK_comma,                 tagmon,         {.i = -1}},
    {MODKEY|ControlMask,           XK_period,                tagmon,         {.i = +1}},
    {MODKEY|ShiftMask,             XK_q,                     killclient,     {0}},
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
    PREVIEWTAGKEYS(XK_1, 0)
    PREVIEWTAGKEYS(XK_2, 1)
    PREVIEWTAGKEYS(XK_3, 2)
    PREVIEWTAGKEYS(XK_4, 3)
    PREVIEWTAGKEYS(XK_5, 4)
    PREVIEWTAGKEYS(XK_6, 5)
    PREVIEWTAGKEYS(XK_7, 6)
    PREVIEWTAGKEYS(XK_8, 7)
    PREVIEWTAGKEYS(XK_9, 8)
};

// status click event command
static const char *statuscmd[] = {"/bin/sh", "-c", "$HOME/.dwm/dwm-statuscmd.sh $INDEX $BUTTON", NULL};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
    /* click          event   mask     button          function argument */
    //hostname
    {  ClkHost,       0,      Button1, spawn,          LAUNCHCMD("apps")},
    {  ClkHost,       0,      Button3, spawn,          LAUNCHCMD("powermenu")},
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
};
