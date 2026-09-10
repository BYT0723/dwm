# Spec: rofi windows 按 tag 分组、组内按 bar 顺序

## Objective

`Mod+w`（`LAUNCHCMD("windows")` → `rofi -show window`）的窗口列表按 tag 分组显示：
每 tag 一组（空 tag 不显示），组内窗口顺序与 topbar 的 tab 顺序（`drawtabs` 遍历 `m->clients` 并过滤 `ISVISIBLE` 后的顺序）一致，保证“所见即所选”。

用户故事：
- 按 `Mod+w`，rofi 列表先按 tag（1→9）分组，每组有 `icon+name` 标题（如 ` dev`），组内顺序与切到该 tag 时 bar 上从左到右一致（含 hidden 窗口）。
- 回车激活窗口时跳到其 tag/monitor（复用现有 `jump_on_activate=1` 行为）。

## ASSUMPTIONS（已确认 + 待确认）

1. 多 tag 窗口只在首 tag（最低位，`ffs`）出现一次 —— 已确认。
2. 组内含 hidden（`HIDDEN(c)`）窗口，与 bar 一致 —— 已确认。
3. 标题用 `icon + name`（`tags[i]` + `tag_names[i]`，如 ` dev`）—— 已确认。
4. 范围：默认当前 monitor？跨 monitor 时标题是否加 `[M0]` 前缀？→ 待你确认（推荐：默认汇总全部 monitor，标题加 monitor 前缀以区分）。
5. 实现方式：推荐 `dwm.c` EWMH 补齐为主（原生 `rofi -show window` 自动分组），不再另维护一套 `-dmenu` 脚本。

## EWMH DESKTOP = tag 还是 monitor？（回答你的问题）

结论：**DESKTOP 映射到 tag，monitor 另行表达，不混用。**

- EWMH 里 `_NET_WM_DESKTOP` = 虚拟桌面 = “一次只看一组窗口”的维度。dwm 里这个维度是 **tag**（一次 `view` 一组 tag），不是 monitor。
- monitor 是物理屏维度（Xinerama），EWMH 没有“每个桌面 × 每个屏”的二维概念；dwm 多屏是每个 monitor 独立 `tagset`，窗口同时归属 `(monitor, tags)`。
- 业界 dwm 的 `ewmhtags` 补丁也是 `desktop ↔ tag`（`_NET_NUMBER_OF_DESKTOPS = LENGTH(tags)`，`_NET_CURRENT_DESKTOP = 首选 tag 索引`，每个窗口 `_NET_WM_DESKTOP = 首 tag 索引`）。
- monitor 在 rofi 侧建议作为次级信息：标题/条目加 `[M0]` 前缀，或只按当前 monitor 过滤（由脚本参数决定），而不是占用 DESKTOP 编号。

## Tech Stack

- C99 + Xlib（`dwm.c`），EWMH（`_NET_*`）。
- rofi（`rofi -show window`，按 `_NET_CLIENT_LIST` + `_NET_WM_DESKTOP` 分组/排序）。
- 验证工具：`xprop -root _NET_CLIENT_LIST _NET_NUMBER_OF_DESKTOPS _NET_DESKTOP_NAMES _NET_CURRENT_DESKTOP`、`xprop _NET_WM_DESKTOP`。

## Commands

```
Build: make
Clean: make clean
Verify EWMH: xprop -root _NET_CLIENT_LIST _NET_NUMBER_OF_DESKTOPS _NET_DESKTOP_NAMES _NET_CURRENT_DESKTOP
Verify win: xprop _NET_WM_DESKTOP | grep <win>
```

## Project Structure

```
dwm.c            → Net 原子表、updateclientlist、manage/unmanage、view/tag 更新 desktop 属性
config.h         → tags[] / tag_names[]（标题来源，不新增配置项）
docs/specs/SPEC-rofi-windows-by-tag.md → 本 spec
```

## Code Style

沿用本仓风格：静态函数、小写下划线、`LENGTH()` 宏；新增原子追加到 `Net*` 枚举尾部（`NetLast` 之前），避免打乱已发布 `NetSupported` 顺序的语义（`NetSupported` 按 `netatom` 全量发布，顺序变化无 ABI 影响，但保持尾部追加最小 diff）。

```c
/* 主 tag：多 tag 只取最低位；全 tag(sticky) 视为 0xFFFFFFFF 由调用方处理 */
static int
clientdesktop(Client *c) {
    if (c->tags == ~0u)
        return 0xFFFFFFFF;
    return __builtin_ffs(c->tags) - 1;
}
```

## Testing Strategy

- `make` 零新增警告（`-Wall -Wextra` 现有旗标下）。
- Xephyr 或实机：开 2 monitor × 3 tag × 若干窗口，`xprop -root` 检查：
  1. `_NET_NUMBER_OF_DESKTOPS == 9`；
  2. `_NET_DESKTOP_NAMES` == `icon+name` 顺序；
  3. `_NET_CLIENT_LIST` 按 `(monitor, 主tag, m->clients 相对顺序)` 稳定分组；
  4. 每个窗口 `_NET_WM_DESKTOP == 主tag 索引`（sticky 除外）。
- `rofi -show window` 目检：tag 组顺序 1→9，组内顺序与切到该 tag 的 bar tab 顺序一致；回车跳转正确（`jump_on_activate`）。
- hot-restart（`Mod+Ctrl+q`）后属性不丢。

## Boundaries

- Always: 组内相对顺序严格等于 `m->clients` 链表相对顺序（即 bar 顺序）；空 tag 不发分组；`make` 零警告。
- Ask first: 改 `attach`/`attachbottom` 默认挂载策略；动 `drawtabs` 渲染；新增外部脚本依赖（`wmctrl/xdotool`）。
- Never: 为 rofi 改变窗口管理语义（tags/monitor 归属）；提交 secrets；删除失败测试。

## Success Criteria

1. `make` 零新增警告。
2. `xprop -root _NET_NUMBER_OF_DESKTOPS` = 9，`_NET_DESKTOP_NAMES` 与 `tags[]+tag_names[]` 一致。
3. `_NET_CLIENT_LIST` 按 `(主tag升序，组内 bar 顺序)` 排列；`xprop _NET_WM_DESKTOP` 每窗等于其主 tag。
4. `rofi -show window` 分组 + 组内顺序与 bar 一致；多 tag 窗只出现一次；空 tag 无分组。
5. `view/tag/tagmon` 切换后 `_NET_CURRENT_DESKTOP` 与各窗口 `_NET_WM_DESKTOP` 实时更新；hot-restart 后恢复。

## Implementation Notes（2026-09-10 已实现，未提交）

- `dwm.c`：`Net*` 追加 4 原子；新增 `clientdesktop/updatenumberofdesktops/updatedesktopnames/updatecurrentdesktop/updatewmdesktop`；`setup()` 发布 number/names/current；`manage/tag/toggletag/sendmon` 同步窗口 desktop + 重排 client list；`view/toggleview/setcurrentmon` 更新 current；`updateclientlist()` 全局按主 tag 稳定分组（不动 `m->clients` 链表；单次 Replace 原子发布）。
- `contrib/rofi-windows.sh`（新增，可执行）：读 `_NET_CLIENT_LIST` 保序；每窗一次 xprop 批量取字段，列宽收集时同步计算 → 查 `_NET_WM_DESKTOP`/title/class/hidden → `xrandr+xdotool` 推断 monitor → 输出 flat 四列 `[tag]\t[monitor]\t[class]\t[title]`（无标题行，filter 直达；空 tag 自然无行）→ `rofi -dmenu -format i -show-icons`（行尾 \0icon\x1f<小写class>，NUL 经 tr 写入）→ `xdotool windowactivate`（复用 `jump_on_activate`）。tag 标签读 `_NET_DESKTOP_NAMES`（与 dwm 同源，无需同步；取不到时回退硬编码）。tag/monitor/class 三列垫齐到最长；monitor 列显示 xrandr 输出短名（DisplayPort-0→DP-0，HDMI-A-0→HDMI-0，映射冲突回退全名），当前 monitor（指针所在 ≈ selmon）的行前标 `*`（其余行空格补齐）；几何未知显示 `[?]`。
- 原生 `rofi -show window` 同步受益（desktop + 有序 client list）。
- 兼容性：此前窗口无 `_NET_WM_DESKTOP`，alttab 类工具默认即显示全部窗口；补齐后它们按“当前 desktop”过滤，只剩 selmon 主 tag 的窗口。这是 EWMH 标准行为（同 ewmhtags 补丁），修工具侧配置：alttab 用 `-d 1`（或 Xresource `alttab.desktops: 1`，1 = 全部 desktops，见 alttab(1)）。_NET_CLIENT_LIST 的 tag 排序只影响顺序，不影响可见性。

## Verification

- [x] `make` 零警告（`-Wall`）。
- [x] `bash -n contrib/rofi-windows.sh` 通过；分组稳定排序逻辑单测通过（python 模拟 + `sort -s` 实测）。
- [ ] 实机 `xprop` 清单（需 X 环境，headless 未跑）：见 Success Criteria 2–5；`Mod+w` 目检；`Mod+Ctrl+q` hot-restart 后复查。

## Open Questions（已确认 2026-09-10）

- [x] 范围：**全部 monitor 汇总**（仅当前 monitor 对 tile WM 无意义）。条目加 `[M0/M1]` 前缀区分。
- [x] sticky（`~0`，如 `bilichat-tui`）：**归第一组**（`ffs(~0)-1 = 0`，自然落到 tag1，只出现一次）。
- [x] `_NET_DESKTOP_NAMES` 用 **`icon+name`**（如 ` dev`），与标题一致。
- [x] 实现策略：**两者都要** —— `dwm.c` EWMH 补齐（`rofi -show window` 原生分组可用）+ `contrib/rofi-windows.sh` `-dmenu` 显式 tag 标题行（保证“按 tag 分行”体验）。EWMH 是前置，脚本读 EWMH 属性。
