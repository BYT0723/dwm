# Spec: EWMH resize 8 directions (0-7)

## ASSUMPTIONS I'M MAKING

1. EWMH `_NET_WM_MOVERESIZE` direction语义按标准：0=TL, 1=T, 2=TR, 3=R, 4=BR, 5=B, 6=BL, 7=L, 8=move（只做 0-7，8 已有）。
2. `resizemouse` 签名不变 `(const Arg *arg, unsigned int event)`，方向经 `arg->i` 传入；`config.h` 中 `MODKEY+Button3` 改为 `{.i = 4}` 保持现有右下角行为。
3. 光标按 EWMH 方向加载主题版本（`sizecursor[8]`：`nw/n/ne/e/se/s/sw/w-resize`，缺主题时逐项回退到 core 角/边 shape），`MODKEY+Button3` 走 dir=4（SE）。

## Objective

现在 `clientmessage()` 中 `_NET_WM_MOVERESIZE` 的 0-7 全部折叠进同一个 `resizemouse(NULL, ...)`（右下角 SE resize），CSD 客户端从上/左/四角发起的 resize 方向全错。目标：8 个方向各自按直觉工作，MOD+右键行为不变。

成功 = 每个方向拖拽时固定对边、活动边跟随指针；`make` 无警告；现有 Xvfb 回归全绿。

## Tech Stack

C99, Xlib。与 `moveresize-check.sh` 同一套 harness：Xvfb + `python3-xlib` 发 ClientMessage + `xdotool` 模拟 motion/release + `xwininfo` 断言。

## Commands

```
Build:     make clean && make
Lint:      make 输出零警告（本仓库无独立 lint）
Test one:  ./tests/moveresize-check.sh :96
Test new:  ./tests/moveresize-resize-dirs-check.sh :97
Test all:  make test
Check:     git diff --check; git diff --stat
```

## Project Structure

```
dwm.c                      → clientmessage() 传方向 + resizemouse() 按方向 warp/解算
config.h                   → ClkClientWin MODKEY+Button3 从 {0} 改为 {.i = 4}
tests/moveresize-resize-dirs-check.sh → 新增：0-7 每个方向的 Xvfb 回归
tests/moveresize-minsize-check.sh     → 新增：最小尺寸钳制 + 对边固定 + 松开不跳变
tests/cursor-theme-check.sh           → 新增：光标主题名契约
docs/specs/SPEC-ewmh-resize-8dir.md   → 本文件
```

## Code Style

沿用包围代码风格：1TBS 大括号、2 空格缩进、early-return 守卫、简短 `/* */` 注释。示例：

```c
} else if (cme->message_type == netatom[NetWMMoveResize]) {
  /* CSD resize: data.l[2] 0-7 direction, 8 move */
  long dir = cme->data.l[2];
  ...
  Arg a = {.i = (int)dir };
  resizemouse(&a, KeyPressMask);
```

`resizemouse` 内部用 delta 解算（与 `movemouse` 一致：`getrootptr` 取起点，`nw = ow ± (x - ox)`），西/北方向对请求几何做一次 `applysizehints` pre-flight，按实际落地的 `fw/fh` 重锚定拖拽边以固定对边。

## Testing Strategy

无单测 harness（仓库现状）。以 Xvfb shell 回归为验收：

- 新测试逐个方向：发 `dir` → `mousemove` 定量偏移 → `mouseup 1` → 断言窗口 `x/y/w/h` 按方向变化（如 dir=0 TL：x 增大、y 增大、w/h 减小；dir=4 BR：x/y 不动、w/h 增大）。
- 回归：`moveresize-check.sh`（dir=8 move）、`moveresize-window-check.sh`、`moveresize-minsize-check.sh`（最小尺寸 + 对边固定 + 松开不跳变）、`cursor-theme-check.sh`（主题名契约）、`make test` 全绿。
- 覆盖要求：8 个方向各至少一次 + MOD+右键（dir=4）手动路径不变 + fullscreen 拒绝 + 非法 dir 忽略。

## Boundaries

- Always: `make clean && make` 零警告；`git diff --check` 干净；最小 diff，不碰无关行；tiled 客户端保持现有 `togglefloatingclient` 语义。
- Ask first: 改 `config.h` 默认键绑定之外的值；动 `movemouse`；换光标/抓取语义。
- Never: 为 tiled 布局 cliente 直接定几何（布局拥有 tiled 几何）；提交 secrets； allowable 之外的 dir（<0, >8, 9-11 keyboard/cancel）必须忽略；直接在 `main` 上开发（本 spec 在 `feat/ewmh-resize-8dir` worktree）。

## Success Criteria

1. `dir 0-7` 每个方向拖拽几何正确：东系（2,3,4）右移增大 w；西系（0,6,7）左移增大 w 且右边缘固定；南系（4,5,6）下移增大 h；北系（0,1,2）上移增大 h 且下边缘固定；四角为两轴叠加；单轴方向另一轴不变。
2. `MODKEY+Button3` 仍为 SE（dir=4）行为，与改前一致。
3. `dir=8` move 路径不受影响；`isfullscreen` 拒绝 resize；非法 dir 直接 return。
4. `make clean && make` 零警告，`make test` 全绿，`git diff --check` 干净。

## Open Questions

1. 西/北钳到最小 `1px` 时，是固定对边（推荐）还是允许窗口跳动？倾向固定对边。
2. 四边中点 warp 落点是否需要避开 titlebar 按钮区？倾向不需要，按边中点即可。
3. 按方向换光标：已做（`sizecursor[8]`，主题名 + core 回退，`cursor-theme-check.sh` 覆盖）。
