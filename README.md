# dwm

Personal fork of [suckless dwm](https://dwm.suckless.org/) (base: 6.6) with a modular
three-zone bar, tab pills, per-client titlebars, vanitygaps layouts, and a
`status-ids` wire protocol for the external status daemon.

## Preview

![preview](https://i.imgur.com/yqzK360.png)

> Tips: 配合 dwm 运行的大量 shell 被存储在 [scripts](https://github.com/BYT0723/scripts) 中，建议完整阅读文档并安装依赖，再配合本仓库的 dwm 使用。

## Features

- **Modular bar**: `bar_left / bar_center / bar_right` zones in `config.h`, each a list
  of `{BarTags, BarLayout, BarTabs, BarStatus}` items. Separate portrait configs
  (`bar_*_portrait[]`) apply when `wh > ww`.
- **Status pills**: every status block is its own `ST(*)` item; consecutive
  items share one rounded pill, `{BarPillBreak, 0}` starts a new one.
  Shell emits content+color, dwm owns layout.
- **Tab modes**: `TabModeIcons` (single shared pill, default) vs `TabModeIconTitle`
  (one pill per client), toggled at runtime with `Mod+Shift+b`. Sizing via
  `TabFit / TabFill / TabFixed`.
- **Titlebar**: optional per-client titlebar with icon, alignment and button labels.
- **Layouts**: vanitygaps set (`tile`, `spiral`, `dwindle`, `deck`, `bstack`,
  `grid`, `nrowgrid`, …) plus `mfact`/`cfact` presets, gaps control, `focusmaster`,
  `maximize`/`fullscreen`, hide/show window set.
- **Systray**: with configurable pinning, spacing and icon order.
- **Status protocol**: `status-ids.def` is the single source of truth. `make`
  derives `status-ids.h` (C enum) and `contrib/status-ids.sh` (shell ids);
  `make test` fails on any drift.
- **Performance**: instrumented bar hot paths, see `PERF.md` (notes ledger in `docs/PERF.md`).

## Quick Start

### Option A: full desktop bootstrap (Arch Linux only)

`install.sh` rebuilds the whole environment on bare Arch: compilers, X11,
fonts, shell, terminal, `picom`/`rofi`/`dunst`, apps, dwm build, dotfile deploy.

```shell
git clone https://github.com/BYT0723/dwm.git
cd dwm
chmod +x install.sh

./install.sh            # interactive phase picker (dialog checklist)
./install.sh --all      # all 10 phases, no prompts
./install.sh --all --cn # same, with CN mirrors (ghfast + archlinuxcn)
./install.sh --dry-run  # preview only
```

Phases: 0 bootstrap (paru/dialog) → 1 build deps → 2 X11 tools → 3 fonts →
4 shell → 5 terminal/CLI → 6 desktop components → 7 apps → 8 `make clean install`
→ 9 deploy `~/.dwm` + dotfiles → 10 verify.

### Option B: build dwm only

```shell
sudo pacman -S --needed base-devel libx11 libxinerama libxft libxrender imlib2 fontconfig freetype2
make clean && make
sudo make install
```

Debian/Ubuntu build deps: `libx11-dev libxinerama-dev libxft-dev libxrender-dev libimlib2-dev libfontconfig1-dev libfreetype6-dev`.

## Requirements

Build (see `install.sh` Phase 1 / `config.mk` `LIBS`):

- `base-devel` (make, cc), `libX11`, `libXinerama`, `libXft`, `libXrender`
- `imlib2` (client icon loading, status tab icons), `fontconfig` + `freetype2`

Runtime (full list + consumer script per package: `install.sh` Phases 2–7):

- `picom`, `xautolock` (+ `xprintidle`), `network-manager-applet`, `fcitx5`, `udiskie`
- `imlib2` for status bar tab icons (already in the Option B one-liner above)

## Commands

| Command                                       | Description                                                                    |
| --------------------------------------------- | ------------------------------------------------------------------------------ |
| `make` / `make clean && make`                 | Build `dwm` (must stay warning-free)                                           |
| `sudo make install`                           | Install binary + man page (`PREFIX=/usr/local`, see `config.mk`)               |
| `make install-ids [STATUS_IDS_DIR=...]`       | Ship generated `contrib/status-ids.sh` to `~/.dwm` (only when changed)         |
| `make test`                                   | Unit tests: `barparse` geometry/parsing + status-ids drift check (no X needed) |
| `make smoke`                                  | E2E bar test on a private Xvfb display (screenshot + click scan)               |
| `make clean` / `make dist` / `make uninstall` | Clean artifacts / build tarball / uninstall                                    |
| `./install.sh [--all] [--cn] [--dry-run]`     | Full Arch environment bootstrap (see Quick Start)                              |

`make smoke` additionally needs `Xvfb`, `xterm`, `xdotool`, `xwd` and
ImageMagick (`convert`/`compare`). It starts dwm on a throwaway `HOME` and
screenshots the bar, so run it on a machine where starting a headless X server
is acceptable.

## Configuration

All user configuration lives in `config.h` (edit, then rebuild):

- **Bar**: `bar_left / bar_center / bar_right` (+ `*_portrait` when `wh > ww`).
  Every status block is an `ST(*)` item (`status-ids.def`); consecutive
  `BarTags`/`BarLayout`/`BarStatus` items share one pill, `{BarPillBreak, 0}`
  starts a new one. `BarTabs` stretches — keep it last. `bar24bit` switches
  to a 24-bit opaque flat bar (unified `SchemeSystray` background, no pill
  shapes, same layout, real X border for picom rounding) with the systray
  merged into the bar window.
- **Tabs/titlebar**: `tabwidth`, `tabgap`, `tabmode` (`TabModeIcons` default),
  `tabsize`, titlebar icon/alignment/buttons.
- **Tags/rules/layouts**: `tags[]` (count only), `tagtext`, per-class `rules[]`,
  `mfact`/`cfact` presets, gaps, `smartgaps`.
- **Keys/buttons/status**: `keys[]` (`MODKEY = Mod4`), `buttons[]`,
  `statuscmd[]` (`$HOME/.dwm/dwm-statuscmd.sh $INDEX $BUTTON`).
- **Status ids**: edit only `status-ids.def`, then rebuild. `make` regenerates
  both consumers; `make install-ids` ships the shell side; restart the status
  daemon afterwards.

Key bindings worth knowing: `Mod+t` terminal, `Mod+d` launcher,
`Mod+w` windows, `Mod+j/k` focus, `Mod+h/s` hide/show, `Mod+b` bar,
`Mod+Shift+b` tab mode, `Mod+r` / `Mod+Shift+r` mfact/cfact presets,
`Mod+q` kill client, `Mod+Ctrl+q` hot restart. Full list in `config.h`.

## Architecture

```
dwm.c          → window management, bar drawing, events, layouts wiring
drw.c / drw.h  → font / color / rounded-pill drawing primitives (Xft/Xrender)
barparse.c/h   → X-free status block parsing + pill grouping + center geometry (unit-tested)
vanitygaps.c   → gap-aware layouts (included from `config.h`)
transient.c    → upstream demo tool for transient windows (not part of the build)
util.c / util.h→ shared helpers
config.h       → THE config: bar zones, pills, tabs, tags, rules, keys
status-ids.def → single source of truth for status block ids (→ status-ids.h + contrib/status-ids.sh)
contrib/       → generated status-ids.sh (shipped to ~/.dwm)
tests/         → barparse_test.c, check-status-ids.sh, dwm-smoke.sh, gen-status-ids.sh
docs/specs/    → feature specs (bar modules, tab modes, titlebar, presets, …)
PERF.md        → measured bar hot-path optimizations (glyph cache, color cache, …)
```

Design split: the shell status daemon (`~/.dwm`, from the
[scripts](https://github.com/BYT0723/scripts) repo) owns **content and color**;
dwm owns **layout and grouping**. Status blocks are prefixed with a control
character id (`status-ids.def`); clicks report the id back as `$INDEX`.

## Related Repos

- Runtime scripts (`~/.dwm`, status daemon, launcher, tools): [BYT0723/scripts](https://github.com/BYT0723/scripts)
- Dotfiles deployed by `install.sh` Phase 9: [BYT0723/dotfile](https://github.com/BYT0723/dotfile)
- Upstream: [suckless dwm](https://git.suckless.org/dwm) (tracked as the `suckless` git remote)

## Contributing

1. Read the relevant `docs/specs/SPEC-*.md` before changing bar/tabs/titlebar behavior.
2. `status-ids.def` is append/reserve-only in practice — never renumber a live id
   (`$INDEX` is consumed by `dwm-statuscmd.sh`).
3. Keep `make` warning-free and `make test` green; add cases to
   `tests/barparse_test.c` for parsing/geometry changes.
4. Run `make smoke` on a machine with Xvfb before claiming bar visual changes.

## License

MIT/X Consortium License — see [LICENSE](LICENSE) for the full text.

- Copyright holders are the upstream suckless contributors listed in `LICENSE`
  (© 2006–2022: Anselm R. Garbe, Jukka Salmi, Sander van Dijk, Hiltjo Posthuma,
  and others — full list in-file). This fork adds no new copyright notice and
  is distributed under the same terms.
- You may use, copy, modify, merge, publish, distribute, sublicense, and sell
  the software, provided the copyright and permission notices are kept in all
  copies or substantial portions.
- The software is provided "as is", without warranty of any kind.
