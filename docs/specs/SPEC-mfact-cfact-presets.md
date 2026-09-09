# Spec: niri 风格 mfact / cfact 预设切换

## Objective

仿照 niri 的 preset 键位（`Mod+R { switch-preset-column-width; }` 系列），为 dwm
新增 mfact 与 cfact 的"下一个预设"循环切换。

映射关系（由用户确认）：

| dwm 概念 | niri 对应概念 | 语义 |
|---|---|---|
| `mfact` | preset column width | 主区域（master column）相对输出宽的比例 |
| `cfact` | preset window height | 聚焦客户端相对同区域其他客户端的大小因子 |

用户明确只做**向前循环（next）**，不做 prev、不做单独的 reset 键。

## Key bindings

| 按键 | 动作 |
|---|---|
| `Mod+R` | mfact 切换到下一个预设（数组内循环，含回绕） |
| `Mod+Shift+R` | cfact 切换到下一个预设（数组内循环，含回绕） |

cfact 作用于**聚焦窗口**。无聚焦窗口 / 无 tiling arrange 时不生效
（guard 内联在 `cyclecfact` 中）。

## 预设数值（config.h 可自定义数组）

- `mfact_presets[]`（升序）默认取 niri 默认列宽：`{ 0.33333, 0.5, 0.66667 }`，
  均在 mfact 合法区间 `[0.05, 0.95]`。
- `cfact_presets[]`（升序）默认：`{ 0.5, 1.0, 2.0 }`，均在 cfact 合法区间
  `[0.25, 4.0]`。

## 循环定位策略（用户已确认）

按预设数组索引循环：找到当前值在预设数组中的索引，前进一位，末尾回绕到开头。
若当前值不在数组中（例如被 `Mod+h/l` 微调过，或为默认 `mfact=0.55`），则吸附到
**严格大于当前值的最小预设**；若当前值已大于等于最大预设，回绕到第一个预设。

（实现为值驱动、无持久索引状态，对 per-tag/per-client 天然正确；行为与"从命中
索引前进"在值命中预设时完全一致。）

## State / 持久化

- `mfact` 落盘到 `selmon->pertag->mfacts[curtag]`（`cyclemfact` 内联写入）。
- `cfact` 写聚焦客户端 `c->cfact`（`cyclecfact` 内联写入）。

## 受影响范围 / Code Style

- `config.h`：新增两个 `static const float` 预设数组（mfact 定义附近）+ 2 条 key。
- `dwm.c`：新增两个 handler `cyclemfact`/`cyclecfact`（原型在函数声明区，实现放
  在 `nextpreset` 之后）。函数签名 `void X(const Arg *arg)`，无需读取 arg；沿用本仓库
  2 空格缩进、无括号单语句风格。

## 命令

```
Build:  make          # config.mk: -std=c99 -pedantic -Wall
Clean:  make clean
```

## Testing Strategy

本项目为 X11 窗口管理器，无可运行单测框架。验证策略：

- `make` 无警告编译通过；
- 编译后静态核对 handler 与 key 表中函数指针一致性；
- 交由用户 hot-restart（`Mod+Ctrl+q`）后实机验证键位。

## Boundaries

- Always: 不引入新依赖、不改动不相关键位、不破坏 pertag per-tag mfact 语义。
- Ask first: 改动 `Pertag`/`Client` 结构体、增加状态字段。
- Never: 改动用户当前未提交的 config.h 其他内容范围之外的东西。

## Success Criteria

1. `Mod+R` 使 mfact 依次取 `{0.33333, 0.5, 0.66667}` 并回绕。
2. `Mod+Shift+R` 使聚焦窗口 cfact 依次取 `{0.5, 1.0, 2.0}` 并回绕。
3. 两预设数组可被用户在 config.h 直接增删改数值。
4. per-tag 切换后 mfact 仍为该 tag 独立状态。
5. `make` 编译零警告。
