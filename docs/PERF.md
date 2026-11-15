# Performance notes

Ledger of measured optimizations, so a discarded idea is not tried again.

## Harness

Synthetic, reproducible, no user session touched: `tests/perf-harness.sh`.

- Xvfb 1600x900x24, throwaway `HOME`/`XDG_DATA_HOME` (so `runautostart` is a
  no-op), dwm on a private display, three xterms as clients.
- Scenarios (`tests/perf-harness.sh <scenario> <iterations>`):
  - **status redraw**: `XStoreName` on the root window, paced at 10 ms so dwm's
    flood guard (`run()` skips `PropertyNotify` when >50 events are queued)
    does not drop the redraw. Each update runs `updatestatus -> drawbar`.
  - **tag switch**: `xdotool key super+N` cycling tags; each `view()` runs
    `arrange`/`restack` plus `drawbar`.
  - **focus**: `xdotool key super+j`; each `focus()` runs `drawbar` +
    `drawtitles`.
  - **switcher**: open/close the overlay (`super+grave`, Escape).
  - **idle**: no events; must stay at 0 ticks.
- CPU = `utime + stime` of dwm **and** Xvfb from `/proc/<pid>/stat`. dwm only
  marshals requests; the rendering happens in the X server, so measuring dwm
  alone hides most of the cost. Unit = clock ticks (10 ms), median of 3 runs.
- Self-time attribution: `perf` is blocked by `kernel.perf_event_paranoid=2`
  here, so build with `-pg` and read the `gprof` flat profile (100 Hz
  `ITIMER_PROF` samples + exact call counts). `GRACE=1` quits dwm through its
  keybinding so `gmon.out` is written; `GMON_OUT_PREFIX` keeps runs apart.

## Kept

| Change | Workload | Before | After | Note |
|---|---|---|---|---|
| `drw_rounded` shape cache sized to the live shape set (`ROUNDED_SHAPES` 3 → 8, `PERF.md`) | status 800, tags 200, focus 200 | dwm ticks 41 / 21.5 / 25 | dwm ticks 16 / 11 / 14 | one redraw asks for four `(r,h)` shapes at once, so a 3-slot round-robin evicted a shape per request and rebuilt every AA mask (`AA_SAMPLES^2` subsamples per pixel) on every redraw; Xvfb also dropped (57/53/68 → 50/51/66) because the mask uploads are gone |
| Tag/client previews off by default (`previews = 0`, `SPEC-preview-option.md`) | 120 tag switches, 3 clients | 157 ticks (68 dwm + 89 xvfb) | 41 ticks (6 + 35) | the eager whole-monitor `XGetImage` on every tag switch no longer runs at all; opt back in with `previews = 1` |
| `takepreview()` returns before `XGetImage` when no occupied tag is in the current view | 120 tag switches, no clients | 90 ticks (48 dwm + 42 xvfb) | 19 ticks (2 + 17) | switching to an empty tag used to read back the whole 1280x800 screen for nothing |
| `takepreview()` uploads the capture once and reuses the source picture per tag | all-tags/multi-tag views | one full `XPutImage` per captured tag | one for all tags | `XPutImage` is a whole-image transfer, so per-tag uploads multiplied it; single-tag views unchanged |
| `drw_rounded()` keeps several `(radius, height)` shapes cached | 120 tag switches, 3 clients | 170 ticks (76 + 94) | 157 ticks (68 + 89) | the bar draws caps at `(tabr, bh)` and, with `TabModeIcons`, the selection dot at `(tabseldot, 2*tabseldot)`; the single-entry cache was evicted and its `AA_SAMPLES^2` masks rebuilt every redraw |
| `statusparse()` splits `stext` once per status update, not once per zone measure + draw | status redraw | (unmeasured) | — | removes ~3 of 4 `bar_blocks` runs per redraw; the pill build still runs per zone. Kept for clarity, effect below the harness resolution |

## Reverted / not pursued

| Idea | Result | Why |
|---|---|---|
| Cache the built status pills per `ids` to drop the remaining repeated `statuspills_build` | not done | measure-and-draw are separate passes, so reusing them needs per-zone copies of ~2.5 KB of built state; the status path is ~0.26 ms/redraw at ~1 Hz, so the complexity is not repaid |
| Skip the whole-screen `XGetImage` for occupied views too | not done | the tagmap must be captured while the view is still on screen; the readback is inherent to the feature |
| Split `dwm.c` (bar/status/tooltip) into a module | not done | large structural change with no functional gain; `barparse.c` already isolates the unit-testable core |
