# Spec: EWMH batch (close / moveresize-window / frame-extents / attention / wintype-float)

Five independent capabilities, zero inter-dependencies, any build order.
Module ids are stable kebab-case: `close-window`, `moveresize-window`,
`frame-extents`, `demands-attention`, `window-type-float`.

## Objective

dwm advertises `_NET_SUPPORTED` but ignores several EWMH requests that
well-behaved clients rely on. Result: `wmctrl -c` can't close windows,
programmatic moves (`wmctrl -r -e`, Electron `setBounds`) misbehave, CSD
clients can't learn our frame size, chat-app pings never light up the tag,
and splash/utility/notification windows get tiled and wreck the layout.

Success = each of the five works for floating clients, tiled layout is
never disturbed by any of them, `make` stays warning-free, existing
Xvfb regressions stay green.

## Tech Stack

C99, Xlib, Xinerama. No new dependencies. Test: `python3-xlib`
(installed) + `xdotool`/`xwininfo`/`xprop` under Xvfb, same harness as
`tests/moveresize-check.sh`.

## Commands

```
Build:     make
 Lint:      make output must contain no warnings (project has no separate lint)
 Test one:  ./tests/<name>-check.sh :<free display>
 Test all:  ./tests/floating-center.sh :95 && ./tests/dwm-smoke.sh
```

## Project Structure

```
dwm.c                      → all WM logic (enum netatom, setup, clientmessage,
                             propertynotify, updatewindowtype, manage)
tests/moveresize-check.sh  → existing pattern for new *-check.sh regressions
docs/specs/                → this file (committed)
tasks/                     → plan-ewmh.md, todo-ewmh.md (local, gitignored)
```

## Code Style

Match surrounding code: early-return guards, terse `/* */` comments naming
the EWMH section, no new abstractions unless used twice. Example:

```c
} else if (cme->message_type == netatom[NetCloseWindow]) {
  /* EWMH close: same path as the titlebar close button */
  if (c != selmon->sel)
    focus(c);
  if (selmon->sel == c)
    killclient(NULL);
```

New atoms go in the `Net*` enum next to their neighbours and are interned
in `setup()` next to the other `netatom[]` lines. They are auto-advertised
via the wholesale `_NET_SUPPORTED` publish; no extra publish code.

## Testing Strategy

One Xvfb shell test per module, mirroring `moveresize-check.sh`:
start Xvfb + dwm, drive with `python3-xlib` ClientMessages, assert with
`xwininfo`/`xprop`. Levels: no unit tests (no harness in repo); each
`*-check.sh` is the acceptance test. Coverage bar: every new branch
(move vs resize, set vs unset, floating vs tiled) exercised at least once.

## Boundaries

- Always: `make` zero warnings; run the touched module test + floating-center before commit; one commit per module.
- Ask first: touching `showhide`/hide-snapshot code, changing `config.h` defaults, touching bar/tab rendering.
- Never: commit secrets; weaken existing grabs/focus semantics; honor geometry for tiled clients (layout owns tiled geometry, same as `configurerequest`).

## Success Criteria (per module)

1. `close-window`: `_NET_CLOSE_WINDOW` on a client closes it (graceful
   `WM_DELETE`, fallback kill — same as `killclient`). Non-managed window
   in the message is ignored.
2. `moveresize-window`: `_NET_MOVERESIZE_WINDOW` with all-flags + NW gravity
   moves/resizes a floating client to the requested geometry (client-size
   semantics, frame-offset math and `fitmonitor` clamp identical to
   `configurerequest`); tiled clients keep layout geometry (request ignored,
   like `configurerequest`); partial flags apply only present fields;
   other gravities treated as NorthWest (documented).
3. `frame-extents`: `_NET_FRAME_EXTENTS` (left,right,top,bottom =
   `bw,bw,bw+titleh,bw`) is set on the client at `manage()` and refreshed on
   `_NET_REQUEST_FRAME_EXTENTS`; `xprop` on the client shows correct values
   for titled and `notitle` windows.
4. `demands-attention`: adding `_NET_WM_STATE_DEMANDS_ATTENTION` lights the
   client's tag (same `isurgent` path as `XUrgencyHint`); removing it or
   focusing the client clears; `_NET_WM_STATE` is scanned as a full list
   (current `getatomprop` reads the first atom only).
5. `window-type-float`: `SPLASH/UTILITY/NOTIFICATION/MENU` (+ existing
   DIALOG) set `isfloating`; a splash window no longer steals the master
   area in `tile`.

## Post-review fixes

1. `_NET_CLOSE_WINDOW` closes via a shared `closeclient()` (extracted from
   `killclient`), so it never steals focus and also works for hidden
   clients and clients on other tags/monitors.
2. `_NET_FRAME_EXTENTS` is refreshed when `setfullscreen` changes `bw`
   and when a runtime `ConfigureRequest` changes the border width.
3. `_NET_WM_STATE_DEMANDS_ATTENTION` is removed from `_NET_WM_STATE` when
   the client is focused (EWMH 5.7: the WM owns clearing it), so a stale
   atom cannot re-light the tag on the next state change.
4. A pre-map `_NET_REQUEST_FRAME_EXTENTS` (window not yet managed) is
   answered with the config-default extents, as EWMH 4.5 intends.
5. A runtime `_NET_WM_WINDOW_TYPE` change that flips `isfloating` now
   triggers `arrange()` so the new float state takes effect immediately.

## Open Questions

None. Capability map and NorthWest simplification approved by human.
