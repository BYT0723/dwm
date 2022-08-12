// #include <X11/XF86keysym.h>

/* appearance */
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayonleft = 0;   	/* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 5;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;     /* 0 means no systray */
static const int showbar            = 1;     /* 0 means no bar */
static const int topbar             = 1;     /* 0 means bottom bar */
static const Bool viewontag         = True;     /* Switch view on tag switch */
static const char *fonts[]          = {
  "Source Code Pro:style=Semibold:size=10",
  "NotoSansMono Nerd Font:style=Medium:size=10",
  "文泉驿等宽微米黑:style=Regular:size=10"
};
static const char dmenufont[]       = "Source Code Pro:style=Semibold:size=12";
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#ffffff";
static const char col_border[]      = "#42A5F5";

// solarized theme
static const char col_black[]       ="#073642";  /*  0: black    */
static const char col_red[]         ="#dc322f";  /*  1: red      */
static const char col_green[]       ="#859900";  /*  2: green    */
static const char col_yellow[]      ="#b58900";  /*  3: yellow   */
static const char col_blue[]        ="#268bd2";  /*  4: blue     */
static const char col_magenta[]     ="#d33682";  /*  5: magenta  */
static const char col_cyan[]        ="#2aa198";  /*  6: cyan     */
static const char col_white[]       ="#eee8d5";  /*  7: white    */
static const char col_brblack[]     ="#002b36";  /*  8: brblack  */
static const char col_brred[]       ="#cb4b16";  /*  9: brred    */
static const char col_brgreen[]     ="#586e75";  /* 10: brgreen  */
static const char col_bryellow[]    ="#657b83";  /* 11: bryellow */
static const char col_brblue[]      ="#839496";  /* 12: brblue   */
static const char col_brmagenta[]   ="#6c71c4";  /* 13: brmagenta*/
static const char col_brcyan[]      ="#93a1a1";  /* 14: brcyan   */
static const char col_brwhite[]     ="#fdf6e3";  /* 15: brwhite  */

static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_blue, col_black, col_black },
	[SchemeSel]  = { col_black, col_cyan,  col_cyan  },
	[SchemeHid]  = { col_cyan,  col_black, col_cyan  },   // hidden
	[SchemeWarn] =	 { col_black, col_yellow, col_red },  // warn
	[SchemeUrgent]=	 { col_black, col_red,    col_red },  //urgent
};

/* tagging */
// static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static const char *tags[] = { "", "", "", "", "", "", "7", "8", "9" };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class                    instance            title           tags mask     isfloating    monitor */
    { "firefox",                NULL,               NULL,           1<<1,           0,              -1 },
    { "obs",                    NULL,               NULL,           1<<4,           0,              -1 },
    { "qBittorrent",            NULL,               NULL,           1<<5,           0,              -1 },

    // float 
    { "qqmusic",                NULL,               NULL,           1<<3,           1,              -1 },
    { "netease-cloud-music",    NULL,               NULL,           1<<3,           1,              -1 },
    { "xunlei",                 NULL,               NULL,           1<<5,           1,              -1 },
    { "vlc",                    NULL,               NULL,           0,              1,              -1 },
    { "feh",                    NULL,               NULL,           0,              1,              -1 },
    { "wemeet",                 NULL,               NULL,           0,              1,              -1 },
    { "dbeaver",                NULL,               NULL,           0,              1,              -1 },
    //wps
    { "wps",                    NULL,               NULL,           0,              1,              -1 },
    { "wpp",                    NULL,               NULL,           0,              1,              -1 },
    { "et",                     NULL,               NULL,           0,              1,              -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
    /* symbol     arrange function */
	{ "﬿ ",      tile },    /* first entry is default */
	{ "禎",      NULL },    /* no layout function means floating behavior */
	{ " ",      monocle },
};

/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {
    "dmenu_run",
    // "-b",
    // "-l","10",
    "-p",">>> ",
    "-m", dmenumon,
    "-fn", dmenufont,
    "-nb", col_black,
    "-nf", col_blue,
    "-sb", col_cyan,
    "-sf", col_black, NULL
};
static const char *termcmd[]  = { "st", NULL };
static const char *floatcmd[]  = { "st", "-i", "-g", "80x25+550+150", NULL};
static const char *lockcmd[] = { "slock", NULL };
static const char *roficmd[] = { "rofi", "-combi-modi", "drun,run,ssh", "-show", "combi", "-show-icons", "-theme", "onedark", "-font", "hack 14", NULL };

// 调节音量
// static const char *upvol[] = {"./.dwm/vol-up.sh", NULL};
// static const char *downvol[] = {"./.dwm/vol-down.sh",NULL};
// static const char *mutevol[] = {"./.dwm/vol-toggle.sh",NULL};

// 调节亮度
// static const char *upbright[] = {"./.dwm/backlight-up.sh",NULL};
// static const char *downbright[] = {"./.dwm/backlight-down.sh",NULL};

// 打开关闭触摸板
static const char *toggleTouchpad[] = {"./.dwm/touchpad-toggle.sh",NULL};
// 截图
static const char *flameshot[] = {"flameshot", "gui", NULL};

static Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_d,      spawn,          {.v = roficmd} },
	{ MODKEY,                       XK_Return, spawn,          {.v = termcmd} },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
    { MODKEY,                       XK_n,      spawn,          {.v = floatcmd} },
    { MODKEY,                       XK_a,      spawn,          {.v = flameshot} },
    { MODKEY|ShiftMask,             XK_l,      spawn,          {.v = lockcmd} },
    { MODKEY,                       XK_space,  spawn,          {.v = toggleTouchpad} },
	// { 0,                            XF86XK_AudioRaiseVolume,    spawn,          {.v = upvol   } },
    // { 0,                            XF86XK_AudioLowerVolume,    spawn,          {.v = downvol } },
	// { 0,                            XF86XK_AudioMute,           spawn,          {.v = mutevol } },
	// { 0,                            XF86XK_MonBrightnessUp,     spawn,          {.v = upbright } },
	// { 0,                            XF86XK_MonBrightnessDown,   spawn,          {.v = downbright } },
	{ MODKEY,                       XK_j,      focusstackvis,  {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstackvis,  {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_j,      focusstackhid,  {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_k,      focusstackhid,  {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_s,      show,           {0} },
	{ MODKEY|ShiftMask,             XK_h,      hide,           {0} },
	{ MODKEY,                       XK_v,      incnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_s,      incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY|ControlMask,           XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY|ShiftMask,             XK_q,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY|ShiftMask,             XK_f,      fullscreen,     {0} },
	// { MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ControlMask,           XK_space,  togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ControlMask,           XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
	{ ClkWinTitle,          0,              Button1,        togglewin,      {0} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

