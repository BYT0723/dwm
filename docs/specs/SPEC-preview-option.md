# Spec: preview 开关（tag / client preview，默认关闭）

## Objective

把预览类功能收敛为一个显式配置项并**默认关闭**。当前：

- **tag preview**：`view()`/`toggleview()` 每次切 tag 都调用 `takepreview()`，
  无条件 `XGetImage` 整块显示器（1280x800x32 ≈ 4 MB）到客户端内存，再按当前视图
  内占用的 tag 各做一次缩放上传。实测空工作区切 tag ≈ 0.6 ms/次纯浪费，占用视图
  更高。
- **client preview**：hover client tab 的 tooltip（`hoverinfo`）会周期性地把窗口
  图片缩放合成进 tooltip。

用户：希望 dwm 默认零预览开销、需要时再打开的 dwm 使用者。
成功 = 默认配置下切 tag 不再发生全屏回读；把开关打开后 tag/client 预览行为与今天
一致。

## Assumptions（已按用户指令取定，review 时可改）

1. 用一个总开关 `previews` 取代现有的 `hoverinfo`，同时管辖 tag preview 与
   client preview（用户把两者一并称为 "preview"）。
2. `previews = 0` 为默认：`takepreview()` 直接返回（不截图、不建 tagmap），
   hover 不再弹 client tooltip / tag 预览。
3. `Mod1+<n>`（`PREVIEWTAGKEYS` / `previewtag()`）键位与函数**移除**，无论开关
   状态；tag 预览只经由 hover tag。
4. `previews = 1` 时 tag/client 预览行为与改动前一致（除键位已移除）。

## Tech Stack

- C99 dwm fork，仅 Xlib/XRender；无新依赖、无新文件（除本 spec）。
- 涉及：`config.h`（开关 + 注释）、`dwm.c`（gate 点）、`tests/dwm-smoke.sh`（断言）。

## Commands

```
Build:  make clean >/dev/null 2>&1 && make    # 零 warning
Unit:   make test
Smoke:  make smoke                            # Xvfb 行为 + 截图比对
Perf:   /tmp/opencode/scenario.sh 120 5 1     # 本地基准（切 tag CPU）
```

## Project Structure

```
config.h                  -> previews 开关（替换 hoverinfo）
dwm.c                     -> takepreview()/showtagpreview()/hover*() 的 gate
tests/dwm-smoke.sh        -> 新增：默认关闭时 hover 不得出现 client tooltip / tag 预览
docs/PERF.md              -> 记录关闭预览后的切 tag 开销
tasks/plan-preview-option.md / tasks/todo-preview-option.md -> 计划与任务
```

## Code Style

```c
/* previews: tag 快照 + client tab tooltip，默认关。
 * 0 = 关闭：切 tag 不做全屏回读，hover 不弹 tooltip；
 * 1 = 开启：行为同以往。 */
static const int previews = 0;

/* dwm.c 的 gate 点，集中在函数入口 */
void takepreview(void) {
  if (!previews)
    return;
  ...
}
```

## Testing Strategy

仓库无 X 行为单测框架，沿用现有 Xvfb 行为测试：

1. **RED**：先在 `tests/dwm-smoke.sh` 加断言——离开一个 tag 后把指针沿 tag
   行扫过，屏幕必须不出现预览窗口（每帧与基线一致）。改动前默认会弹预览，
   断言失败。
2. **GREEN**：加 `previews` gate 后同断言通过。
3. **翻转**：`config.h` 里 `previews = 1` 时，同一断言改为要求至少一帧截图
   **变化**（用 `grep -E 'previews[[:space:]]*=[[:space:]]*1' config.h` 分支）。
4. **回归**：`make` 零 warning；`make test` 全绿；`make smoke` 既有 4 项仍 PASS。
5. **性能**：切 tag 基准（占用视图）应显著下降。

## Boundaries

- Always：每次增量后 `make` 零 warning；gate 只加在预览入口，不动 tag/arrange/bar。
- Ask first：改 `previews` 默认值、拆成 tag/client 两个独立开关、删预览键位。
- Never：为预览改动删既有功能；泄漏 Pixmap/Picture；提交临时基准脚本与日志。

## Success Criteria

- [x] `previews = 0`（默认）时 `takepreview()` 不发生 `XGetImage`。
- [x] 默认下 hover tag 不出现预览窗口（smoke 逐帧截图一致）。
- [x] `Mod1+<n>` 键位与 `previewtag()` 已移除，无残留引用。
- [x] 默认下 hover client tab 不弹 tooltip。
- [x] `previews = 1` 时 tag/client 预览与改动前一致。
- [x] `make` 零 warning，`make test` / `make smoke` 全绿。

## Decisions（已定）

1. tag / client 预览**共用一个总开关** `previews`，不拆分。
2. `Mod1+<n>` 预览键位**已移除**，无论 `previews` 开关状态。
