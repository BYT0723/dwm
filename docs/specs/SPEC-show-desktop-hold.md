# Spec: Show Desktop 按住 Peek + KeyRelease 骨架

## Objective
给当前 `selmon` 加 Windows 式显示桌面：按住 `MOD+Shift+d` 隐藏全部可见窗口看壁纸，松开原样恢复。`bar` 保留，隐藏态 `tab` 灰显可点回单个。顺带引入通用 `KeyRelease` 骨架，供以后复用。

## ASSUMPTIONS
1. 范围仅当前 `Monitor` 当前 `tagset` 可见窗口，不碰其他显示器（用户确认）。
2. 恢复方式主 `hold-peek`，`toggle` 为降级（用户接受）。
3. `bar` 不藏（用户确认）。
4. `MOD+d` 已被 `LAUNCHCMD(apps)` 占用，用 `MOD+Shift+d` 占位。

## Commands
- Build: `make clean && make`
- Test: `make test`
- Smoke（可选，需 Xvfb）: `./tests/dwm-smoke.sh`

## Project Structure
- `dwm.c` → 事件骨架（`handler[KeyRelease]`、`grabkeys` 单表、`keyevent(e, ev)` 统一分发 + 去抖）+ `showdesktop(arg, event)` 按事件掩码分相位
- `config.h` → 单张 `keys[]`，一行一次绑定：`{mod, key, func, arg, ev}`；`ev` 是该行响应的完整事件集合，每行显式声明，无隐式默认
- `docs/specs/SPEC-show-desktop-hold.md` → 本文件
- `tasks/plan.md` + `tasks/todo.md` → 计划与任务

## Code Style
沿用现有模式：`hidewin` 只 `XUnmapWindow(frame)+hidden=1+IconicState`，`showwin` 只 `XMapWindow+hidden=0+NormalState`。批量循环后统一一次 `arrange+focus/restack`，不逐个 `arrange`。绑定一次，事件掩码由分发器传参：
```c
{MODKEY|ShiftMask, XK_d, showdesktop, {0}, KeyPressMask | KeyReleaseMask},
```
```c
/* 所有绑定动作统一签名，event 为触发它的事件掩码 */
void showdesktop(const Arg *arg, unsigned int event) {
  if (event == KeyPressMask) { /* 藏 */ } else { /* 恢复 */ }
}
```
`keyevent(e, ev)` 是唯一分发：先按 `keys[i].ev & ev` 过滤，press 严格匹配 `CLEANMASK`，release 只要求绑定修饰键仍按住，然后把 `ev` 作为参数交给动作。普通键位显式写 `KeyPressMask`；新事件类型只需再加一个掩码位。

## Testing Strategy
- `make clean && make` 零警告（仓库门禁）。
- `make test` 全绿（`barparse_test` + `check-status-ids.sh` + `check-pill-radius.sh`）。
- X 逻辑无单元测试，以构建 + 存量测试 + 代码走读验证；可选 Xvfb 目检按住藏/松开回。
- TDD：先写 `tests/show-desktop-check.sh` 静态检查（`KeyReleaseMask`、`handler[KeyRelease]`、`keysRelease`、`showdesktop_hold/release` 存在且成对），RED 跑通后再 GREEN 实现。

## Boundaries
- Always: 只动当前 `selmon` 的 `ISVISIBLE` 窗口；press 无窗口直接返回；release 只恢复 `desktop_hidden` 的客户端；`focus(NULL)/focus(save)+restack` 配对。
- Ask first: 改键位、改全局全显示器、藏 `bar`、新增可调配置项。
- Never: 不改 `hidewin/showwin` 单体语义；不改现有 85 行键位；不引入定时器/后台线程；不提交 `master`，停留特性分支。
- 约束：事件类型是绑定动作的正常参数（`unsigned int event`），不进入 `Arg`；`keys[]` 一行绑定一次，`ev` 显式列出该行响应的全部事件（普通键位 `KeyPressMask`，hold 键位 `KeyPressMask | KeyReleaseMask`）。

## Success Criteria
- [ ] 按住 `MOD+Shift+d`：当前 `selmon` 可见窗口全部 `hidden=1+IconicState`，壁纸可见，`bar` 还在。
- [ ] 松开同组合：全部恢复 `NormalState`，焦点回到按下前 `sel`（若还在同 `selmon`）。
- [ ] 按下时无可见窗口 / 松开时无隐藏窗口：无操作不崩。
- [ ] 连击/自动重发不闪烁（`keyrelease` 去抖）。
- [ ] 先松 `MOD` 再松 `D` 也能恢复（宽松匹配）。
- [ ] `make clean && make` 零警告，`make test` 通过，`git diff --stat` 只有 `dwm.c/config.h/tests/*`。

## Open Questions
- `fullscreen`（EWMH）窗口是否跳过？默认不跳过，全藏。
- 按住期间新 `manage` 的窗口：保持可见（Windows 一致），不强制再藏。
