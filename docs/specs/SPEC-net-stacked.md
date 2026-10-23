# Spec: Net pill 上下叠排（方案B，bar 高度不变）

## Objective

把 network traffic 监控 pill 从左右排（` RX  TX` 一行）
改成上下叠排：上行 ` RX速度`，下行 ` TX速度`，挤在现有 `bh`
（~33px）内，用小字（默认 10px，`stacked_fontsize` 可配）画两行，其他 pill 保持单行不变。

用户故事：

- 展开态：Net pill 显示两行，`bh` 不变，bar 不加高。
- 折叠态（`~/.local/state/dwm/status/net-traffic-collapse` 存在）：
  保持现有单行图标行为不变。
- 点击 Net pill 仍上报 `StNet`（`$INDEX=11`），上下两行同一点击区。

ASSUMPTIONS（已和用户对齐）：

1. 小字 10px，`bh` 严格不变（`bh = fonts->h + bar_fontpad*2`，`dwm.c:4944`）。
2. 内容为上 `↓RX` / 下 `↑TX`，各沿用 `human_speed` 格式。
3. 折叠模式保留。

## Tech Stack

- C99 + Xlib/Xft（`dwm.c`，`drw.c`），Nerd Font 图标。
- writer：`~/.dwm/dwm-status-print.sh:print_speed`（仓外，需同步改）。

## Commands

```sh
Build: make clean && make            # 必须零警告
Test:  make test                     # 全绿
Check: git diff --check && git diff --stat
```

## Project Structure

```
dwm.c                → 状态解析/度量/绘制（status2d_runwidth、drawstatusseg、setup小字体）
config.h             → stacked_fontsize（默认 10px）+ 开关（默认开）
barparse.c/.h        → 若需行切分helper，放这里以便单测（X-free）
tests/barparse_test.c → 行切分/行宽max单测
docs/specs/SPEC-net-stacked.md → 本文件
~/.dwm/dwm-status-print.sh → print_speed输出分隔符（仓外联动）
```

dwm 内协议：writer 在 Net block 内用新 `status2d` 码 `^s^`
分隔上下行（`s` 当前空闲；`c/b/d/r/f/(/)` 已占用）。
`bar_blocks` 不动（控制字节仍是 block id），`^s` 走 `^...^`
文本码通道，与颜色码同路。

## Code Style

遵守 `AGENTS.md`：1TBS 大括号（定义也不换行）、2空格、无tab、
最小 diff。示例：

```c
int status2d_rows(const char *s, const char **top, const char **bot) {
  return 0;
}
```

## Testing Strategy

- 框架：仓内 `make test`（含 `tests/barparse_test.c` + shell checks）。
- 新增单测：切分 `a^s^b` → 两行；`runwidth = max(top, bottom)`；
  无 `^s` 时行为与现在一致（单行）。
- 回归：`make test` 全绿；`make clean && make` 零警告；
  `git diff --check` 干净。
- 手动：`xsetroot -name` 喂 `ST_NET_BYTE` 的 `上行^s^下行` 与
  collapse 两种输出，截图确认两行 10px、无裁剪、点击 `$INDEX=11`。

## Boundaries

- Always：先读源码+单测再改；`make clean && make` + `make test` +
  `git diff --check/--stat` 自检；小字体图标可读性抽查。
- Ask first：`status2d` 码字母（拟 `s`）、`config.h` 旋钮名
  （拟 `stacked_fontsize` + `stacked_rows`）、writer 改动（仓外脚本）。
- Never：不改 `bh`（全 bar 不加高）；不动其他 block 的绘制/度量；
  不改点击语义（仍单 slot）；不提交 `node_modules/dist/build`、
  secrets，不顺手重构。

## Success Criteria

1. `bh` 与改前一致（`setup` 公式不变）。
2. 展开态 Net pill 为两行 10px，上 ` RX` 下 ` TX`，行内
   `^c/^b/^d` 颜色语义与单行一致。
3. pill 宽 = `max(上行宽, 下行宽)`，caps 为全高左右圆角，与单行
   pill 同高。
4. 折叠态输出与现在逐字节一致行为（单行图标）。
5. 点击上下任意行都上报 `StNet`。
6. `make clean && make` 零警告，`make test` 全绿，
   `git diff --check` 干净。

## Open Questions

1. 10px 下 Nerd Font 的 `/` 是否过小难认——要不要图标用正常字号、
   只有数字用小字？（当前 spec 按整行小字；若验收时难看再切。）
2. 两行不等宽时对齐：拟左对齐（沿用 `human_speed` 的 `%5d` 右对齐数字）。
   要不要右对齐速度列？

## Writer 联动（仓外，需用户同步）

`print_speed` 展开分支改成一行输出两段，中间插 `^s^`：

```sh
printf "%s " "${icons[traffic_rx]}"  # 上行
human_speed $rx
printf "^s^"
printf "%s " "${icons[traffic_tx]}"  # 下行
human_speed $tx
```

折叠分支不动。
