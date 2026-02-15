# DWM - Dynamic Window Manager

This is a customized build of dwm (dynamic window manager) - a minimal, fast X11 window manager. The codebase is written in C and follows suckless philosophy.

## Building and Installing

```bash
# Build dwm
make

# Install to system (default: /usr/local/bin)
make install

# Clean build artifacts
make clean
```

The build system uses `config.mk` for compiler flags and library paths. Key dependencies:
- X11 libraries (Xlib, Xinerama, Xft, Xrender)
- Imlib2 (for status bar icons)
- freetype2 (for font rendering)

## Architecture Overview

### Core Components

**dwm.c** (3252 lines) - Main window manager logic:
- Event-driven architecture: handlers organized in array for O(1) dispatch
- Monitor management: each monitor has its own client list and stack
- Client management: X windows organized as linked list with tag bitmasks
- Layout system: tiling, floating, and custom layouts

**config.h** (390 lines) - All configuration and customization:
- Appearance settings (colors, gaps, borders, fonts)
- Keybindings and mouse bindings
- Window rules for application-specific behavior
- Tags (workspaces), layouts, and statusbar configuration

**drw.c/drw.h** - Drawing abstraction layer:
- Wraps X11/Xft drawing primitives
- Font management (supports multiple fonts with fallback)
- Color scheme management with alpha channel support
- Icon rendering with Imlib2

**util.c/util.h** - Utility functions (die, ecalloc)

### Key Concepts

**Tags vs Workspaces**: Uses 9 tags (not traditional workspaces). Windows can have multiple tags, allowing them to appear in multiple "views" simultaneously.

**Client/Monitor/Stack**: 
- **Client** = managed window with metadata (tags, size, floating state)
- **Monitor** = physical screen with tagset and client list
- **Stack** = focus history per monitor

**Event Handlers**: Each X event type has dedicated handler function in `handlers[]` array indexed by event type.

## Configuration Conventions

### Color Schemes
Colors are defined as hex strings in `config.h` with corresponding scheme enums:
- SchemeNorm, SchemeSel, SchemeHid (window states)
- SchemeTagNorm, SchemeTagSel (tag display)
- SchemeHost, SchemeLayout, SchemeStatus, SchemeSystray

Each scheme has fg/bg/border colors and alpha values in parallel arrays.

### Patches Applied
This build includes multiple patches (see README.md "Patches" section):
- systray: system tray support
- status2d/statuscmd: enhanced status bar with 2D drawing and click commands
- awesomebar: improved task display
- vanitygaps/cfacts: configurable window gaps and client sizing
- pertag: per-tag layout memory
- autostart: autostart script support
- fullscreen, hide_vacant_tags, viewontag, attachbottom

When modifying code, be aware these patches interact with core dwm functions.

### Key Bindings Pattern
Keys defined in `config.h` as array of Key structs:
```c
static const Key keys[] = {
    { modkey, keysym, function, argument },
};
```
Use Mod4Mask (Super/Windows key) as primary modifier.

### Window Rules
Application-specific behavior defined in `rules[]` array:
```c
static const Rule rules[] = {
    { class, instance, title, tags_mask, isfloating, monitor },
};
```

## Modifying the Codebase

**config.h is the single source of truth** for customization. Avoid modifying dwm.c unless implementing new functionality.

**Static functions**: All dwm.c functions are static - this is intentional for performance and encapsulation.

**TEXTW macro**: Use for calculating text width with padding: `TEXTW("text")`

**Client visibility**: Use `ISVISIBLE(c)` macro to check if client should be shown on current tag.

**Color scheme switching**: Use `drw_setscheme(drw, scheme[SchemeXxx])` before drawing operations.

## Alternative Color Schemes

See `colorschemes/` directory for pre-configured color definitions (tokyonight variants). These are header files that can be included in config.h.

## Dependencies for Full Functionality

Runtime dependencies (see README.md):
- picom (compositor)
- xautolock (screen locking)
- nm-applet (network)
- fcitx5 (input method)
- udiskie (automount)
- Scripts from companion repository for statusbar functionality

## Testing

No automated test suite. Test by:
1. Build with `make`
2. Run in nested X server: `Xephyr -br -ac -noreset -screen 1280x720 :1 &`
3. Launch dwm: `DISPLAY=:1 ./dwm`
