# Spec: Show Desktop 按住 Peek（自持 release）

## Objective
给当前 `selmon` 加 Windows 式显示桌面：按住 `MOD+Shift+d` 隐藏全部可见窗口看壁纸，松开原样恢复。`bar` 保留，隐藏态 `tab` 灰显可点回单个。按住期间的 release 由动作自己 grab 等待，不进通用按键分发。

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
- `dwm.c` → `grabkeys` 单表；`keyevent(e)` 只处理 `KeyPress`（严格 `CLEANMASK` 匹配）并把 `KeyPressMask` 交给动作；`showdesktop(arg, event)` 在 press 里 `XGrabKeyboard`、`desktophide()`、自循环等 chord 松开、`desktoprestore()`；鼠标入口 `peekdesktop` 同构（`XGrabPointer` + `ButtonRelease`），与键盘入口共用 `desktophide/desktoprestore/desktopevent`
- `config.h` → 单张 `keys[]`，一行一次绑定：`{mod, key, func, arg}`，无事件列
- `docs/specs/SPEC-show-desktop-hold.md` → 本文件
- `tasks/plan.md` + `tasks/todo.md` → 计划与任务

## Code Style
沿用现有模式：`hidewin` 只 `XUnmapWindow(frame)+hidden=1+IconicState`，`showwin` 只 `XMapWindow+hidden=0+NormalState`。批量循环后统一一次 `arrange+focus/restack`，不逐个 `arrange`。绑定一次：
```c
{MODKEY|ShiftMask, XK_d, showdesktop, {0}},
```
```c
/* 所有绑定动作统一签名；hold 动作在 press 里抓设备并自己等释放 */
void showdesktop(const Arg *arg, unsigned int event) {
  if (event != KeyPressMask) return;
  /* XGrabKeyboard → desktophide → 等 chord 松开 → desktoprestore */
}
```
`keyevent(e)` 是唯一按键分发：只匹配 `KeyPress` 和 `CLEANMASK`；释放语义属于动作本身（`showdesktop`、`switcher`），chord 由 `keys[]` 里 `func == showdesktop` 的那一行反查，自动重复用 `XQueryKeymap` 查按键是否仍按下（合成的 `KeyRelease` 可能先于它的 `KeyPress` 入队，不能靠回查队列判断）。`keys[]` 不再有事件列。

## Testing Strategy
- `make clean && make` 零警告（仓库门禁）。
- `make test` 全绿（`barparse_test` + `check-*.sh`）。
- X 逻辑无单元测试，以构建 + 存量测试 + 代码走读验证；可选 Xvfb 目检按住藏/松开回。
- TDD：`tests/show-desktop-check.sh` 静态检查 `keys[]` 无事件列、无 `handler[KeyRelease]`、无 `holdrow`、`showdesktop` 自持 grab/循环、chord 释放顺序无关、与 `peekdesktop` 共用 `desktophide/desktoprestore/desktopevent`。

## Boundaries
- Always: 只动当前 `selmon` 的 `ISVISIBLE` 窗口；press 无窗口直接返回；release 只恢复 `desktop_hidden` 的客户端；`focus(NULL)/focus(save)+restack` 配对。
- Ask first: 改键位、改全局全显示器、藏 `bar`、新增可调配置项。
- Never: 不改 `hidewin/showwin` 单体语义；不改现有键位；不引入定时器/后台线程；不恢复通用 `KeyRelease` 分发；不提交 `master`，停留特性分支。
- 约束：事件类型是绑定动作的正常参数（`unsigned int event`），不进入 `Arg`；`keys[]` 一行绑定一次，不再声明事件集合；hold 的释放由动作自己 grab 等待。

## Success Criteria
- [ ] 按住 `MOD+Shift+d`：当前 `selmon` 可见窗口全部 `hidden=1+IconicState`，壁纸可见，`bar` 还在。
- [ ] 松开同组合：全部恢复 `NormalState`，焦点回到按下前 `sel`（若还在同 `selmon`）。
- [ ] 按下时无可见窗口 / 松开时无隐藏窗口：无操作不崩。
- [ ] 连击/自动重发不闪烁（`keyrepeat` 去抖）。
- [ ] 松开 `d`，或 chord 修饰键全部松开后恢复；先松哪个修饰键都不会提前结束。
- [ ] `make clean && make` 零警告，`make test` 通过，`git diff --stat` 只有 `dwm.c/config.h/tests/*`。

## Open Questions
- `fullscreen`（EWMH）窗口是否跳过？默认不跳过，全藏。
- 按住期间新 `manage` 的窗口：保持可见（Windows 一致），不强制再藏。
