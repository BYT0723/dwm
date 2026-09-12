# Spec: borderrule（per-rule 边框）

应用上游补丁 `dwm-borderrule-20231226-e7f651b.diff`：`Rule` 新增 `bw` 列，
负数表示沿用全局 `borderpx`（惯例 `-1`），`>= 0` 覆盖指定窗口的边框宽度。
相对上游多一处收紧：`r->bw >= 0` 才覆盖，`-2` 等笔误值不会直达 X 导致
`BadValue` 致命退出。

## 与本仓库的冲突（用户已确认：记住规则边框）

本仓库 `arrangemon()` 每次 arrange 会把所有窗口 `c->bw` 强制写回全局
`borderpx`（borderless monocle 时写 `0`）。若照搬上游补丁，规则边框在
首次 arrange 后即丢失。因此：

- `Client` 新增 `basebw` 字段：`manage()` 头部先置 `borderpx`，transient
  窗口保持该值；非 transient 经 `applyrules()` 后 `basebw = bw = 规则值`。
- transient 窗口（`WM_TRANSIENT_FOR` 命中已管理父窗口）同样跑一遍
  `applyrules()` 以匹配 class/border 规则，但 `mon/tags` 恢复为跟随父窗口
  （对话框不跳 tag 是 suckless 原生行为，保留）。
- `arrangemon()`：`target = borderless ? 0 : c->basebw`。borderless 语义不变。
- `setfullscreen()` 的 `oldbw` 存取逻辑不变（存的是当前 `bw`，规则 `0`
  边框窗口全屏往返依然正确）。

## config.h（用户已确认：全部 -1）

所有现存 rules 补第 7 列 `-1`，零行为变化；用户后续自行配置。

## 热重启

`basebw` 不落盘：重启后 `scan→manage→applyrules` 重新匹配规则，自然恢复。

## Success Criteria

1. `make` 零新增警告。
2. 规则 `bw >= 0` 的窗口在多次 arrange 后边框保持规则值。
3. 无规则窗口行为与之前完全一致；borderless monocle 仍无边框。
4. 实机验证需 X 环境：`Mod+Ctrl+q` 热重启后规则边框不丢（headless 只验编译）。
