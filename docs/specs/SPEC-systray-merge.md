# Spec: merge bar and systray into one 24-bit window

## Objective

Today the bar (`m->barwin`, 32-bit ARGB for translucency) and the systray
(`systray->win`, 24-bit via `XCreateSimpleWindow`) are two windows: tray
icons misbehave under a 32-bit ARGB parent, so they need a 24-bit home.
Merge them: the bar window itself becomes the tray-icon parent, created with
a 24-bit TrueColor visual. One window per monitor, zero stacking hacks.

Decisions already taken with the user:
- Dual mode, gated by `config.h`: default stays 32-bit ARGB bar + separate
  systray window (today's behavior, untouched); setting the new switch
  enables the 24-bit merged bar.
- 24-bit mode is FLAT: today's modular look comes from #000000 + full
  transparency + pills; without alpha the bar gets one unified background
  and pills are not drawn (no rounded caps, no outlines). Zones, items,
  gaps and positions stay exactly as now.
- Systray area follows the same rule: flat, no pill, icons `ParentRelative`
  on the unified background.
- Visual selection (24-bit mode): prefer 24-bit TrueColor, fall back to
  `DefaultVisual`.

## Tech Stack

C99, X11/Xlib + Xrender + Xft (existing `drw.c`), no new dependencies.

## Commands

```
Build: make
Test:  make test        # barparse unit (189 checks) + status-ids check
Smoke: make smoke        # headless Xvfb bar render incl. status pills
Manual: host run with real tray apps (e.g. pasystray, nm-applet)
```

## Project Structure

```
dwm.c            → everything: xinitvisual, barwin creation, updatesystray*,
                   dock/redock, togglebar, cleanup, drawbar stw reservation
drw.c / drw.h    → drawing primitives (visual/depth flow through, no API change)
config.h         → showsystray/pinning/spacing/order/pad unchanged
tests/           → make test + make smoke (no new harness; tray icons need
                   real X clients, covered by manual checklist below)
docs/specs/SPEC-systray-merge.md → this file
```

## Code Style

Match the file: terse `static` functions, one-line `/* */` intent comments,
no new patterns. Example of the target shape:

```c
/* config.h: 0 = 32-bit ARGB bar + separate systray window (default, current
   behavior); 1 = 24-bit opaque flat bar, systray merged into the barwin */
static const int bar24bit = 0;
```

```c
/* the pinned (or selected) monitor's barwin owns the tray selection and
   parents the icons, so there is exactly one tray window per setup
   (24-bit mode only; 32-bit mode keeps the separate systray window) */
systray->mon = systraytomon(NULL);
```

## Testing Strategy

- `make test`: must stay green in both modes (default 32-bit path untouched;
  ids check unchanged — no config key renamed).
- `make smoke`: must stay green on the default 32-bit path; add a 24-bit
  smoke variant (build with the switch on, or a second run) proving the flat
  bar renders.
- Manual tray checklist (host, real apps, 24-bit mode on):
  1. icons visible in the bar, correct order (`systrayorder`), correct spacing;
  2. icon menus open on click (events reach the icon clients);
  3. kill + restart a tray app → re-docks without restart of dwm;
  4. `togglebar` hides icons with the bar and restores them;
  5. pinned mode: icons stay on the pinned monitor;
  6. sloppy mode (`systraypinning = 0`): icons follow `selmon`;
  7. `xprop -root _NET_SYSTEM_TRAY_S0` owner == owner barwin;
  8. bar renders fully opaque with one unified background, no pill shapes,
     zones/gaps/positions identical to 32-bit mode.

## Boundaries

- Always: `make` zero-warning, `make test` + `make smoke` green before commit;
  keep `stw` reservation/`centerx` math behavior identical.
- Ask first: removing `SchemeSystray`/`alphas` entries (proposal: KEEP them,
  unused entries are cheaper than re-indexing every scheme table); any
  `config.h` default change.
- Never: break existing bar rendering or click resolution; leave a second
  tray window alive; regress multi-monitor bar geometry.

## Success Criteria

1. Default mode is pixel- and behavior-identical to today (32-bit ARGB bar +
   separate systray window); all existing tests green without modification.
2. With the switch on: single window, no `systray->win` lifecycle
   (`XCreateSimpleWindow`, sibling-stacking above `barwin`, offscreen parking
   in `togglebar` all bypassed).
3. `xinitvisual` picks visuals per the switch (32-bit ARGB as today vs 24-bit
   TrueColor, else DefaultVisual); `useargb` reflects reality.
4. 24-bit bar is flat: unified `SchemeSystray` background, no rounded caps
   or outlines on tags/layout/status/tabs pills; zones, items, gaps,
   positions unchanged. The bar itself gets a real X border (`barborderpx`,
   systray border color, inside the bar rect like the 32-bit systray
   window's: the inner geometry is one border smaller per side, so the outer
   footprint stays `ww - 2 * sp` wide and `bh` tall) which picom can round.
5. Tray selection owner + `_NET_SYSTEM_TRAY_ORIENTATION` live on the owner
   barwin (24-bit mode); XEMBED notify/activate messages reference it.
6. Icons reparent directly to the owner barwin at `barw - stw` offsets,
   `ParentRelative` background, vertically centered per `systraypad`.
7. Sloppy mode reparents icons + migrates selection on `selmon` change;
   `togglebar` needs no tray-specific code (children hide with the bar).
8. All 8 manual checklist items pass on host in 24-bit mode.

## Open Questions (resolved during implementation)

1. Owner-barwin unmapped via `togglebar`: icons hide with the bar (children
   move offscreen with it, verified y=-33 → restored). Accepted.
2. `NetWMWindowOpacity`: kept on the 32-bit path, not set on the flat path.
3. Flat-mode unified background: `SchemeSystray` bg — confirmed by user,
   verified as `(7,54,66)` in Xvfb screenshots.
