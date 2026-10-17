# Spec: window shell hook (open/close)

## Objective
窗口 open/close 时异步调用 shell 脚本，传入 action 和窗口信息，
用于外部联动（如统计、自动布局、通知）。默认不阻塞 dwm，
脚本缺失/命令为空时静默跳过。

## Tech Stack
C (dwm.c + config.h)，`/bin/sh -c` fork 执行，与 `spawn`/`statuscmd` 一致。

## Commands
Build: `make`
Smoke: `sh tests/dwm-smoke.sh`
Close 回归: `sh tests/close-window-check.sh`

## Project Structure
- `config.h` → 新增 `windowhook_cmd`
- `dwm.c` → 新增 `runwindowhook()`，在 `manage()`/`unmanage()` 调用
- `docs/specs/SPEC-windowhook.md` → 本文件

## Code Style
跟随现有风格：函数 `static void name(...)`，2 空格缩进，
`snprintf` 构造 env，`fork()+close(dpy)+setsid()+execl("/bin/sh"...)`，
与 `spawn()` 相同。示例：
```c
runwindowhook(c, "open");
```

## Testing Strategy
- `make` 编译通过。
- Xvfb 集成测试：临时 HOME 下放 `dwm-windowhook.sh` 记录 env，
  打开/关闭 xterm，断言出现 `ACTION=open` 和 `ACTION=close` 且
  `CLASS/TITLE/WID` 非空。
- 回归：`tests/close-window-check.sh` 通过。

## Boundaries
- Always: hook 异步 fork，不阻塞/不等待；读 `Client` 只读字段；
  空命令直接返回；systray 图标不触发（只 hook `manage/unmanage`）。
- Ask first: 新增 ACTION 取值（如 focus/minimize）、新增 env 字段。
- Never: 同步 `system()`/`popen()` 阻塞事件循环；提交 secrets；
  改动无关文件。

## Success Criteria
- [ ] `config.h` 有 `windowhook_cmd`，为空则不调用。
- [ ] `manage()` 末尾触发 `open`，`unmanage()` 开头触发 `close`。
- [ ] shell 可读到 `ACTION/WID/CLASS/TITLE/TAGS/MON`。
- [ ] 脚本不存在时 dwm 不崩、不卡。
- [ ] `make` + 上述测试通过。

## Open Questions
无（已确认：config.h 配置；manage/unmanage 触发；env 传参由实现者定）。
