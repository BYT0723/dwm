#include <X11/XF86keysym.h>

/* appearance */
static const unsigned int borderpx     = 0;   /* border pixel of windows */
static const unsigned int snap         = 32;  /* snap pixel */
static const          int showbar      = 1;   /* 0 means no bar */
static const          int topbar       = 1;   /* 0 means bottom bar */
static const          int barfontpad   = 6;
static const          int vertpad      = 6;  /* vertical padding of bar */
static const          int sidepad      = 6;  /* horizontal padding of bar */
static const          char host[]      = "";
static const          int tabradius   = 1;
static const          int tabcenter   = 1;
static const          int statusradius = 0;

static const unsigned int gappih    = 8;  /* horiz inner gap between windows */
static const unsigned int gappiv    = 8;  /* vert inner gap between windows */
static const unsigned int gappoh    = 16;  /* horiz outer gap between windows and screen edge */
static const unsigned int gappov    = 16;  /* vert outer gap between windows and screen edge */
static                int smartgaps = 0;   /* 1 means no outer gap when there is only one window */

static const unsigned int systraypinning          = 0;  /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing          = 2;  /* systray spacing */
static const          int systraypinningfailfirst = 1;  /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const          int showsystray             = 1;  /* 0 means no systray */
static const          int systraypad              = 2;

static const unsigned int attachtop = 0; /* new window is attached to the top of the stack */

static const Bool viewontag = True; /* Switch view on tag switch */
static const char *fonts[] = {
    "Noto Color Emoji:style=Regular:size=11",
    "Symbols Nerd Font Mono:style=2048-em:size=11",
    "CaskaydiaCove Nerd Font:style=Regular:size=11",
    "LXGW WenKai Mono:style=Bold:size=11",
};

static const char col_black[]   = "#073642";  /*  0: black    */
static const char col_red[]     = "#dc322f";  /*  1: red      */
static const char col_green[]   = "#859900";  /*  2: green    */
static const char col_yellow[]  = "#b58900";  /*  3: yellow   */
static const char col_blue[]    = "#268bd2";  /*  4: blue     */
static const char col_magenta[] = "#d33682";  /*  5: magenta  */
static const char col_cyan[]    = "#2aa198";  /*  6: cyan     */
static const char col_white[]   = "#eee8d5";  /*  7: white    */

static const char col_ab_black[]   = "#000000";

static const unsigned int emptyalpha  = 0x00;
static const unsigned int baralpha    = 0xd0;
static const unsigned int borderalpha = OPAQUE;
static const char *colors[][3] = {
    /*                    fg            bg            border   */
    // host
    [SchemeHost]    = { col_blue,     col_black,    col_black    },
    // tag
    [SchemeTagNorm] = { col_white,    col_black,    col_black    },
    [SchemeTagSel]  = { col_black,    col_blue,     col_black    },
    // layout
    [SchemeLayout]  = { col_green,    col_black,    col_ab_black },
    // tasks
    [SchemeNorm]    = { col_cyan,     col_black,    col_ab_black },
    [SchemeSel]     = { col_white,    col_black,    col_white    },
    [SchemeHid]     = { col_cyan,     col_ab_black, col_ab_black },
    // systray
    [SchemeSystray] = { col_white,    col_black,    col_black    },
    // empty
    [SchemeEmpty]   = { col_ab_black, col_ab_black, col_black    },
};
static const unsigned int alphas[][3]      = {
    /*                    fg          bg          border     */
    // host
    [SchemeHost]    = { OPAQUE,     OPAQUE,     emptyalpha },
    // tag
    [SchemeTagNorm] = { OPAQUE,     OPAQUE,     emptyalpha },
    [SchemeTagSel]  = { OPAQUE,     OPAQUE,     emptyalpha },
    // layout
    [SchemeLayout]  = { OPAQUE,     OPAQUE,     emptyalpha },
    // task
    [SchemeNorm]    = { OPAQUE,     OPAQUE,     emptyalpha },
    [SchemeSel]     = { OPAQUE,     OPAQUE,     OPAQUE     },
    [SchemeHid]     = { OPAQUE,     0x80,       emptyalpha },
    // systray
    [SchemeSystray] = { OPAQUE,     baralpha,   emptyalpha },
    // empty
    [SchemeEmpty]   = { emptyalpha, emptyalpha, emptyalpha },
};

/* task icon */
static const char *taskWidth = "                    ";
static const TaskIcon icons[] = {
    /* class                title     icon */
    // default
    {NULL,                  NULL,      " "},
    // terminal
    {"st",                  NULL,      " "},
    {"Alacritty",           NULL,      " "},
    // browser
    {"firefox",             NULL,      " "},
    {"Chromium",            NULL,      " "},
    {"google-chrome",       NULL,      " "},
    // website
    {NULL,                  "YouTube", " "},
    // application
    {"neovide",             NULL,      " "},
    {"TelegramDesktop",     NULL,      " "},
    {"discord",             NULL,      "󰙯 "},
    {"electronic-wechat",   NULL,      " "},
    {"wechat.exe",          NULL,      " "},
    {"qq",                  NULL,      " "},
    {"qqmusic",             NULL,      " "},
    {"netease-cloud-music", NULL,      " "},
    {"vlc",                 NULL,      "󰕼 "},
    {"mpv",                 NULL,      " "},
    {"DBeaver",             NULL,      " "},
    {"Pcmanfm",             NULL,      " "},
    {"Lxappearance",        NULL,      " "},
    {"thunderbird",         NULL,      " "},
    {"steam",               NULL,      " "},
    {"Godot_Engine",        NULL,      " "},
};

/* tagging */
// static const char *tags[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
static const char *tags[] = { "", "", "", "", "", "", "", "", ""};

static const Rule rules[] = {
    /* xprop(1):
     *	WM_CLASS(STRING) = instance, class
     *	WM_NAME(STRING) = title
     */
    /* class                instance    title     tags mask     isfloating    monitor */
    {"firefox",             NULL,       NULL,     1 << 1,       0,            -1},

    {"TelegramDesktop",     NULL,       NULL,     1 << 2,       0,            -1},
    {"electronic-wechat",   NULL,       NULL,     1 << 2,       0,            -1},
    {"wechat.exe",          NULL,       NULL,     1 << 2,       0,            -1},
    {"QQ",                  NULL,       NULL,     1 << 2,       0,            -1},

    {"DBeaver",             NULL,       NULL,     1 << 3,       0,            -1},
    {"resp",                NULL,       NULL,     1 << 3,       0,            -1},

    {"xunlei",              NULL,       NULL,     1 << 5,       1,            -1},
    {"qBittorrent",         NULL,       NULL,     1 << 5,       0,            -1},

    {"obs",                 NULL,       NULL,     1 << 6,       0,            -1},

    {"qqmusic",             NULL,       NULL,     1 << 7,       1,            -1},
    {"netease-cloud-music", NULL,       NULL,     1 << 7,       1,            -1},
    {"OSD Lyrics",          NULL,       NULL,     1 << 7,       1,            -1},

    {"steam",               NULL,       NULL,     1 << 8,       0,            -1},
    {"heroic",              NULL,       NULL,     1 << 8,       0,            -1},

    // other only floating
    {"Godot_Engine",        NULL,       NULL,     0,            1,            -1},
    {"vlc",                 NULL,       NULL,     0,            1,            -1},
    {"mpv",                 NULL,       NULL,     0,            1,            -1},
    {"feh",                 NULL,       NULL,     0,            1,            -1},
    {"viewnior",            NULL,       NULL,     0,            1,            -1},
    {"peek",                NULL,       NULL,     0,            1,            -1},
    {"flameshot",           NULL,       NULL,     0,            1,            -1},
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

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd)                                                             \
  {                                                                            \
    .v = (const char *[]) { "/bin/sh", "-c", cmd, NULL }                       \
  }

/* commands */
static const char *termcmd[]         = {"./.dwm/term.sh",            NULL};
static const char *floatcmd[]        = {"./.dwm/term.sh",            "float",  NULL};
static const char *roficmd[]         = {"./.dwm/app.sh",             NULL};
static const char *powercmd[]        = {"./.dwm/powermenu.sh",       NULL};
static const char *mpdcmd[]          = {"./.dwm/mpd.sh",             NULL};
static const char *linkcmd[]         = {"./.dwm/quicklinks.sh",      NULL};
static const char *modulecmd[]       = {"./.dwm/module.sh",          NULL};
static const char *toggleTouchpad[]  = {"./.dwm/touchpad-toggle.sh", NULL};
static const char *screenshot[]      = {"./.dwm/screenshot.sh",      NULL};
static const char *screencast[]      = {"./.dwm/screencast.sh",      NULL};
static const char *flameshot[]       = {"flameshot",                 "gui",    NULL};
static const char *vol_up[]          = {"./.dwm/volume.sh",          "up",     NULL};
static const char *vol_down[]        = {"./.dwm/volume.sh",          "down",   NULL};
static const char *vol_toggle[]      = {"./.dwm/volume.sh",          "toggle", NULL};
static const char *brightness_up[]   = {"./.dwm/brightness.sh",      "up",     NULL};
static const char *brightness_down[] = {"./.dwm/brightness.sh",      "down",   NULL};
static const char *wallpaper_next[]  = {"./.dwm/wallpaper.sh",       "-n",     NULL};
// don't change or surround it by {}
static const char *layoutmenu_cmd   = "./.dwm/layoutmenu.sh";

/* commands spawned when clicking statusbar, the mouse button pressed is
 * exported as BUTTON */
static const StatusCmd statuscmds[] = {
    {"./.dwm/statuscmd.sh date       $BUTTON", 1},
    {"./.dwm/statuscmd.sh battery    $BUTTON", 2},
    {"./.dwm/statuscmd.sh volume     $BUTTON", 3},
    {"./.dwm/statuscmd.sh brightness $BUTTON", 4},
    {"./.dwm/statuscmd.sh wifi       $BUTTON", 5},
    {"./.dwm/statuscmd.sh disk-root  $BUTTON", 6},
    {"./.dwm/statuscmd.sh memory     $BUTTON", 7},
    {"./.dwm/statuscmd.sh cpu        $BUTTON", 8},
    {"./.dwm/statuscmd.sh weather    $BUTTON", 9},
    {"./.dwm/statuscmd.sh mpd        $BUTTON", 10},
    {"./.dwm/statuscmd.sh netSpeed   $BUTTON", 11},
};
static const char *statuscmd[] = {"/bin/sh", "-c", NULL, NULL};

static Key keys[] = {
    /* modifier                     key         function        argument */
    // custom shell script
    {MODKEY,                        XK_Return,                spawn,          {.v = termcmd}},
    {MODKEY,                        XK_n,                     spawn,          {.v = floatcmd}},
    {MODKEY,                        XK_space,                 spawn,          {.v = toggleTouchpad}},
    {MODKEY,                        XK_a,                     spawn,          {.v = flameshot}},
    {MODKEY | ShiftMask,            XK_a,                     spawn,          {.v = screenshot}},
    {MODKEY | ShiftMask,            XK_r,                     spawn,          {.v = screencast}},
    {0,                             XF86XK_AudioLowerVolume,  spawn,          {.v = vol_down}},
    {0,                             XF86XK_AudioRaiseVolume,  spawn,          {.v = vol_up}},
    {0,                             XF86XK_AudioMute,         spawn,          {.v = vol_toggle}},
    {0,                             XF86XK_MonBrightnessDown, spawn,          {.v = brightness_down}},
    {0,                             XF86XK_MonBrightnessUp,   spawn,          {.v = brightness_up}},
    {MODKEY | ShiftMask,            XK_n,                     spawn,          {.v = wallpaper_next}},
    // rofi
    {MODKEY,                        XK_d,                     spawn,          {.v = roficmd}},
    {MODKEY,                        XK_m,                     spawn,          {.v = modulecmd}},
    {MODKEY | ShiftMask,            XK_m,                     spawn,          {.v = mpdcmd}},
    {MODKEY | ControlMask,          XK_m,                     spawn,          {.v = powercmd}},
    {MODKEY | ShiftMask,            XK_l,                     spawn,          {.v = linkcmd}},
    // layout
    {MODKEY,                        XK_t,                     setlayout,      {.v = &layouts[0]}},
    {MODKEY | ShiftMask,            XK_t,                     layoutmenu,     {0}},
    {MODKEY,                        XK_f,                     setlayout,      {.v = &layouts[1]}},
    // layout adjust
    {MODKEY,                        XK_v,                     incnmaster,     {.i = +1}},
    {MODKEY,                        XK_s,                     incnmaster,     {.i = -1}},
    {MODKEY,                        XK_h,                     setmfact,       {.f = -0.01}},
    {MODKEY,                        XK_l,                     setmfact,       {.f = +0.01}},
    {MODKEY | ControlMask,          XK_h,                     setcfact,       {.f = +0.05}},
    {MODKEY | ControlMask,          XK_l,                     setcfact,       {.f = -0.05}},
    {MODKEY | ControlMask,          XK_o,                     setcfact,       {.f = 0.00}},
    // client manager
    {MODKEY,                        XK_b,                     togglebar,      {0}},
    {MODKEY,                        XK_j,                     focusstackvis,  {.i = +1}},
    {MODKEY,                        XK_k,                     focusstackvis,  {.i = -1}},
    {MODKEY | ShiftMask,            XK_j,                     focusstackhid,  {.i = +1}},
    {MODKEY | ShiftMask,            XK_k,                     focusstackhid,  {.i = -1}},
    {MODKEY | ShiftMask,            XK_s,                     show,           {0}},
    {MODKEY | ControlMask,          XK_s,                     showall,        {0}},
    {MODKEY | ShiftMask,            XK_h,                     hide,           {0}},
    {MODKEY | ShiftMask,            XK_f,                     fullscreen,     {0}},
    {MODKEY | ControlMask,          XK_space,                 togglefloating, {0}},
    {MODKEY | ControlMask,          XK_Return,                zoom,           {0}},
    {MODKEY,                        XK_Tab,                   view,           {0}},
    {MODKEY,                        XK_0,                     view,           {.ui = ~0}},
    {MODKEY | ShiftMask,            XK_0,                     tag,            {.ui = ~0}},
    {MODKEY | ControlMask,          XK_0,                     toggletag,      {.ui = ~0}},
    {MODKEY,                        XK_comma,                 focusmon,       {.i = -1}},
    {MODKEY,                        XK_period,                focusmon,       {.i = +1}},
    {MODKEY | ShiftMask,            XK_comma,                 tagmon,         {.i = -1}},
    {MODKEY | ShiftMask,            XK_period,                tagmon,         {.i = +1}},
    {MODKEY | ShiftMask,            XK_q,                     killclient,     {0}},
    {MODKEY | ControlMask,          XK_q,                     quit,           {0}},
    // gap manager
    {MODKEY | SUPKEY,               XK_u,                     incrgaps,       {.i = +1}},
    {MODKEY | SUPKEY,               XK_i,                     incrigaps,      {.i = +1}},
    {MODKEY | SUPKEY,               XK_o,                     incrogaps,      {.i = +1}},
    {MODKEY | SUPKEY,               XK_6,                     incrihgaps,     {.i = +1}},
    {MODKEY | SUPKEY,               XK_7,                     incrivgaps,     {.i = +1}},
    {MODKEY | SUPKEY,               XK_8,                     incrohgaps,     {.i = +1}},
    {MODKEY | SUPKEY,               XK_9,                     incrovgaps,     {.i = +1}},
    {MODKEY | SUPKEY,               XK_0,                     togglegaps,     {0}},
    {MODKEY | SUPKEY | ShiftMask,   XK_u,                     incrgaps,       {.i = -1}},
    {MODKEY | SUPKEY | ShiftMask,   XK_i,                     incrigaps,      {.i = -1}},
    {MODKEY | SUPKEY | ShiftMask,   XK_o,                     incrogaps,      {.i = -1}},
    {MODKEY | SUPKEY | ShiftMask,   XK_6,                     incrihgaps,     {.i = -1}},
    {MODKEY | SUPKEY | ShiftMask,   XK_7,                     incrivgaps,     {.i = -1}},
    {MODKEY | SUPKEY | ShiftMask,   XK_8,                     incrohgaps,     {.i = -1}},
    {MODKEY | SUPKEY | ShiftMask,   XK_9,                     incrovgaps,     {.i = -1}},
    {MODKEY | SUPKEY | ShiftMask,   XK_0,                     defaultgaps,    {0}},
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

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
    /* click          event   mask     button          function argument */
    //hostname
	{  ClkHost,       0,      Button1, spawn,          {.v = roficmd}},
 	{  ClkHost,       0,      Button3, spawn,          {.v = powercmd}},
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
