# Spec: Tags pill 左段 showdesktop（press-hide / release-restore）

## ASSUMPTIONS I'M MAKING
1. 只做 `TagPillCompact` 左段（选中 tag `[icon name]` / 多选 `combo` 文本区），`TagPillFull` 不动（用户已确认 press/release 方案，hover 已否决）。
2. 复用现有 `showdesktop(arg, event)` hold-peek 语义：`event == KeyPressMask` 藏，else 恢复；鼠标 press 传 `KeyPressMask`，release 传 `KeyReleaseMask`（与 `MOD+Shift+d` 同一分支，`dwm.c:4042`）。
3. 左段目前无 slot（纯展示，`SPEC-tags-pill-dots.md:4`），位置空闲；圆点 `ClkTagBar + 1<<i` 语义不动。
4. 单分支 `feat/tags-pill-showdesktop`，工作区 `~/Workspace/Worktrees/dwm-feat-showdesktop-pill`，不在 `master` 上开发。
→ 有误现在纠正，否则按此推进。

## Objective
给 tags pill 左段加 Windows 式 peek：按住左键隐藏当前 `selmon` 可见窗口看壁纸，松开原样恢复。
用户：本 fork 日常使用者（Compact pill，9 tag）。
成功：按住左段藏、松开回、误触少、圆点点击不受影响、构建测试全绿。

## Tech Stack
- C11 + X11，本仓库 dwm fork（1TBS、2 空格、无 tab）。
- 事件：`buttonpress` 现只处理 `ButtonPress` 并恒传 `KeyPressMask`（`dwm.c:940-941`）；`barwin` 现只选 `ButtonPressMask`（`dwm.c:6079`）；需补 `ButtonRelease` 通路。
- 绘制：`drawtagscompact` 左右两段同属一颗 pill（`dwm.c:1581`）；左 cell 宽 `tagleftw()`，`tagswidth()` 已含它。

## Commands
- Build: `make clean && make`（必须零警告，仓库门禁）
- Test: `make test`
- Check: `git diff --check`
- 目检（可选）: `./tests/dwm-smoke.sh`（需 Xvfb）

## Project Structure
- `dwm.c` → `Clk` 枚举新增 `ClkTagLeft`；`drawtagscompact` 单选/多选两分支各注册左段 slot（`addslot(m, x0, lw, ClkTagLeft, {0})`，先于圆点注册、无重叠）；新增 `peekdesktop(arg, event)` 自包含 handler（对齐 `movemouse/resizemouse` 模式：先 `XGrabPointer`，成功才调 `showdesktop press`，`XMaskEvent` 等到 `ButtonRelease` 再调 `showdesktop release`，任意位置松开都恢复）；`buttonpress` 保持只处理 `ButtonPress`，`barwin` event_mask 不动。
- `config.h` → `buttons[]` 只加一行（`{ClkTagLeft, 0, Button1, peekdesktop, {0}}`）；点击注释补 `ClkTagLeft`。不给 `Button` 加 `ev` 列，不对齐 `keys[]`。
- `docs/specs/SPEC-tags-pill-showdesktop.md` → 本文件。
- `tasks/plan.md` + `tasks/todo.md` → 计划与任务（Phase 2/3 再建）。

## Code Style
沿用 AGENTS.md：函数签名单行 + 左花括号同行、2 空格、无 tab、不重排无关行。绑定动作统一签名 `(const Arg *arg, unsigned int event)`，`peekdesktop` 内部显式传 `KeyPressMask` / `KeyReleaseMask` 给 `showdesktop`（与键盘同分支，`dwm.c:4042`），grab/loop 对齐 `movemouse`（`dwm.c:4569-4605`）：
```c
void peekdesktop(const Arg *arg, unsigned int event) {
  XEvent ev;
  (void)arg; (void)event;
  if (XGrabPointer(dpy, root, False, BUTTONMASK, GrabModeAsync, GrabModeAsync,
                   None, cursor[CurNormal]->cursor, CurrentTime) != GrabSuccess)
    return;
  showdesktop(NULL, KeyPressMask);
  do {
    XMaskEvent(dpy, BUTTONMASK | ExposureMask | SubstructureRedirectMask, &ev);
    if (ev.type == ConfigureRequest || ev.type == Expose ||
        ev.type == MapRequest || ev.type == ClientMessage)
      handler[ev.type](&ev);
  } while (ev.type != ButtonRelease);
  XUngrabPointer(dpy, CurrentTime);
  showdesktop(NULL, KeyReleaseMask);
}
```
注：loop 里多转一份 `ClientMessage`（`movemouse` 没有），只为 `indesktophold` 期间 `_NET_ACTIVE_WINDOW` 仍能记入 `desktop_pending`（`dwm.c:1130`），松开后按现有逻辑回放。

## Testing Strategy
- `make clean && make` 零警告；`make test` 全绿；`git diff --check` 干净；`git diff --stat` 无无关文件。
- X 逻辑无单元测试，以构建 + 存量测试 + 代码走读验证。
- TDD：先加 `tests/` 静态检查（左段 slot 注册、`ClkTagLeft`、`peekdesktop` 内 `XGrabPointer` + `ButtonRelease` 循环 + press/release 成对调用、`buttons[]` 只有一行新增），RED 后再 GREEN 实现。
- Xvfb 目检矩阵：左段按住藏/松开回；按住后拖出 bar 再松开仍恢复；左段单击（快速按松）闪一下即回；圆点点击仍切 tag；多选 combo 左段同样 peek；空视图按住无崩；`TagPillFull` 下无左段 peek（或保持现状）。

## Boundaries
- Always: 只动 `BarTags` Compact 左段加 slot 注册 + 新增 `peekdesktop`；release 只恢复 `desktop_hidden` 的客户端（沿用 `showdesktop` 现有逻辑）；`focus(NULL)/focus(save)+restack` 配对不动；press 无可见窗口直接返回（`showdesktop` 内已有）；grab 失败则不 press。
- Ask first: 改键位 `MOD+Shift+d`、改全显示器范围、藏 `bar`、给 `Button` 加 `ev` 列对齐 `keys[]`、新增可调配置项（如左段 peek 开关）。
- Never: 不改 `hidewin/showwin` 单体语义；不改圆点 `ClkTagBar + 1<<i` 语义；不改 `pillrun/drawbarpill` 框架和 `barwin` event_mask；不改 `buttonpress` 分发；不引入定时器/后台线程；不直推 `master`。

## Success Criteria
- [ ] 左段按住左键：当前 `selmon` 可见窗口全部 `hidden=1+IconicState`，壁纸可见，`bar` 还在。
- [ ] 松开左键：全部恢复 `NormalState`，焦点回到按下前 `sel`（若还在同 `selmon`）。
- [ ] 按住后指针拖出 bar 再松开：仍恢复，不卡死在隐藏态。
- [ ] 圆点点击/右键 `toggleview`/`MOD+` 行为不变；左段右键默认无操作。
- [ ] 空视图按住左段：无操作不崩。
- [ ] `make clean && make` 零警告，`make test` 通过，`git diff --stat` 只有 `dwm.c/config.h/tests/*`。

## Open Questions
1. 左段右键（Button3）是否也 peek，还是保持无操作？（我倾向保持无操作，改动最小）
2. `TagPillFull` 下是否需要同等 peek 区，还是本次只做 Compact？（我倾向只做 Compact，Full 保持现状）
3. 是否需要 `config.h` 开关（如 `tag_left_peek = 1`）让用户可关？还是写死启用？（我倾向先写死，开关等有人要再加）
4. 松开位置是否限制在同一 `barwin` 内，还是 bar/root 上任意 release 都恢复？（我倾向任意 release 都恢复，防卡死）
