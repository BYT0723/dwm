# Spec: 隐藏窗口快照预览 (hidden-snapshot)

## Objective

hover 标题栏 tab 时，已隐藏（`c->hidden == 1`）的窗口 tooltip 当前只显示应用图标或
"no preview"。目标：**在隐藏发生的瞬间抓取该窗口最后可见内容，hover 隐藏窗口时
展示这张快照**。用户：最小化后仍需辨认窗口内容的 dwm 使用者。

成功 = hover 隐藏窗口的 tooltip 中出现其隐藏前最后一帧的真实内容（可抓图比对），
且可见窗口、icon 回退路径行为不变。

## Assumptions

1. 只覆盖 minimize 路径（`hidewin`：titlebar `_`、Mod+h、`WM_CHANGE_STATE/IconicState`、
   focus-hidsel、`togglewin`）；tag 切换 offscreen 位移不走此路径，不快照。
2. 快照只保留**最近一次**隐藏的结果；再次隐藏时覆盖；`showwin` 不刷新不删除；
   客户端销毁时释放。
3. 隐藏时 frame 实际不可见（理论上不应发生）则跳过本次抓取，保留旧快照。
4. Xvfb 隔离测试是唯一的验证手段（仓库无单元测试框架）。
5. **隐藏预览与可见预览同尺寸**：共用 `previewscale` 算法，tooltip 与可见时等大
  （2026-09-16 用户确认，不再使用 160x100 固定框）。尺寸输入取 frame 几何
  （`c->w/c->h`，与 `hovershow` 同源）以保证快照与预览框精确贴合；源仍是客户端
  窗口本身（与实时路径一致，略有标题栏高度的拉伸）。

## Tech Stack

- C99 dwm fork；仅 Xlib + XRender（本次不再需要 XComposite，已为预览移除其依赖）。
- 直接复用已验证的“窗口 Picture + FilterGood + 缩放变换 + PictOpOver”序列。

## Commands

```
Build:  make clean >/dev/null 2>&1 && make    # 必须零 warning
Repro:  Xvfb :NN + /tmp/dwm-* 二进制 + xdotool 扫 tab（见测试策略）
```

## Project Structure

```
dwm.c                    → 全部改动（Client 结构体、hidewin、hoverpreview、unmanage）
docs/specs/SPEC-hidden-snapshot.md → 本 spec
tasks/plan-hidden-snapshot.md      → 实现计划（tasks/plan.md、todo.md 被 CSD 占用，不覆盖）
tasks/todo-hidden-snapshot.md      → 任务清单
```

## Code Style

- 贴着现有风格：静态函数、terse 注释、无新依赖；新字段随 `Picture icon` 风格放一起。
- 快照生命周期镜像 `icon`：`hidewin` 覆盖写入，`unmanage` 释放（`freeicon` 旁）。

```c
/* example: per-client cached snapshot, mirroring icon's lifecycle */
c->hspm = ...; c->hspic = ...; c->hspw = ...; c->hsph = ...;
```

## Testing Strategy

Xvfb 行为测试，无单元框架：

1. **RED**：xterm 跑出文字内容 → Super+h 隐藏 → hover 其 tab → tooltip 出 icon/占位
   （无真实内容），抓图固定当前基线。
2. **GREEN**：同流程，tooltip 出隐藏前最后一帧（抓图对比隐藏前内容一致）。
3. **回归**：可见窗口实时预览仍有内容；InputOnly 不被接管、无 fatal；
   无合成器 hover 不崩；`make` 零 warning。
4. 生命周期：隐藏→恢复→再隐藏，旧快照被覆盖；unmanage 后无悬空（代码走查）。

## Boundaries

- Always: 每次增量后 `make` 零 warning；只在 Xvfb 验证通过后才交付；预览相关 X 调用
  全部留在 `xerrordummy` 保护区内。
- Ask first: 改 tooltip 布局尺寸、动公共 struct 以外内存模型、删 icon 回退。
- Never: 改变可见窗口/占位回退的行为；泄漏 Pixmap/Picture；提交临时调试二进制与日志。

## Success Criteria

- [ ] 隐藏前 tooltip 出现该窗口最后一帧内容（抓图与隐藏前内容一致，非空白）。
- [ ] 无 icon 的隐藏窗口同样显示快照（替代 "no preview"）。
- [ ] 第二次隐藏覆盖旧快照；恢复显示不清除快照。
- [ ] 全部回归项通过；`git diff` 只动 dwm.c（含本次，不含未提交的 CSD 部分则更佳）。

## Open Questions（默认已定，如需改请说）

1. 快照目标尺寸与可见预览一致（`previewscale`），tooltip 等大；不再使用 160x100 固定框。
2. tag 切换不触发快照（frame 保持 mapped，实时路径本来就可用）。
3. 快照上不叠 icon / 不叠边框之外的装饰，纯内容居中。
4. 无快照的隐藏分支（极端路径）仍回退 icon → "no preview"，只是框变大了（居中逻辑不变）。

## Deviations（实现中发现并已处理）

1. `hovershow` 改为先 `XMapRaised` 后 `drw_map`：画到未 map 窗口上的内容会在
   首次 map 时被丢弃（`showtagpreview` 已有同样注脚），否则隐藏 tooltip 永远空白。
   影响可见/隐藏两条路径，回归已验证。
2. Drive-by 修复（与本特性无关，测试中发现的预先存在崩溃）：`cleanup()` 里
   `tooldrw` 与 `drw` 共享 `fonts_set`，退出时 double-free（`drw_fontset_free`
   递归中 SIGSEGV；凡是展示过一次 tooltip 后退出必现）。修法：释放 `tooldrw`
   前先将其 `fonts` 置 NULL。建议单独成 commit。
