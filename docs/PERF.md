# Performance notes

Ledger of measured optimizations, so a discarded idea is not tried again.

## Harness

Synthetic, reproducible, no user session touched:

- Xvfb 1280x800x24, throwaway `HOME`/`XDG_DATA_HOME` (so `runautostart` is a
  no-op), dwm on a private display.
- Two workloads:
  - **status redraw**: `XStoreName` on the root window, paced at 3 ms so dwm's
    flood guard (`run()` skips `PropertyNotify` when >50 events are queued)
    does not drop the redraw. Each update runs `updatestatus -> drawbar`.
  - **tag switch**: `xdotool key super+N` cycling tags; each `view()` runs
    `takepreview()` (screen snapshot for tag previews) plus `arrange`/`drawbar`.
- CPU = `utime + stime` of dwm **and** Xvfb from `/proc/<pid>/stat`. dwm only
  marshals requests; the rendering happens in the X server, so measuring dwm
  alone hides most of the cost. Unit = clock ticks (10 ms), median of 5 runs.

## Kept

| Change | Workload | Before | After | Note |
|---|---|---|---|---|
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
