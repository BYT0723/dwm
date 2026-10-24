# Spec: tags-pill empty dot 改为极小实心点

## ASSUMPTIONS I'M MAKING
1. 只改 `TagPillCompact` 右段 empty 点的画法，不动 `occ/sel/urg` 和几何/点击。
2. 保留固定 `pitch`/`tagdotswidth()`，空位仍占位可点击，只是视觉弱化。
3. 不新增颜色配置：empty 与 occ 同用 `SchemeTagNorm fg` 实心，靠半径区分（小 vs 大）。
4. 单分支 `feat/tags-empty-dot`，工作区 `~/Workspace/Worktrees/dwm-feat-tags-empty-dot`。

## Objective
`drawtagscompact` 当前 empty 用 `SchemeTagNorm fg(#eee8d5)` 画 `r=4` 空心圆，和 urgent 圆环同语言，9 tag 中空位多时满排白圈太抢。改为 `tag_empty_dot_radius`（默认 `1`）的 `SchemeTagNorm` 实心小点：面积约为 occ 点的 1/16，`0` 即不画。靠大小区分 empty/occ，不引入新颜色。

## Tech Stack
- C11 + X11/Xft/Xrender，本仓库 dwm fork（1TBS、2 空格、无 tab）。
- 直接复用 `tabseldot_paint(SchemeTagNorm, ...)`，只新增 `tagemptydotr()`。

## Commands
- Build: `make clean && make`（必须零警告）
- Test: `make test`
- Check: `git diff --check`

## Project Structure
- `config.h` → 新增 `tag_empty_dot_radius = 1`（`0` = 不画），更新 tags pill 注释 `hollow = empty` → `tiny dot = empty`。
- `dwm.c` → 新增 `tagemptydotr()`；`drawtagscompact` empty 分支改调 `tabseldot_paint(..., tagemptydotr())`；更新函数注释。
- `docs/specs/SPEC-tags-pill-empty-dot.md` → 本文件。

```c
static const unsigned int tag_empty_dot_radius = 1;
```

## Code Style
AGENTS.md：签名单行+左花括号同行，2 空格，无 tab，不动无关行。示例：
```c
static int tagemptydotr(void) {
  return MAX(0, MIN((int)tag_empty_dot_radius, bh / 2 - 1));
}
```

## Testing Strategy
- `make clean && make` 零警告；`make test` 全绿；`git diff --check` 干净。
- 走读：empty 小白点/occ 大白点/sel 蓝大点/urg 蓝填白圈可辨；`radius=0` 时空位不画但 slot 还在；`bh` 极小时钳制不越界。

## Boundaries
- Always: 只动 empty 分支；`pitch`/`zone`/`addslot` 不动；Full 模式不动。
- Ask first: 另加 `Scheme` 或改 urgent 语言。
- Never: 不改 `policy_hide_vacant_tags`；不顺手改格式；不直推 `master`。

## Success Criteria
- [x] empty 为小实心点，不再是白圈；occ/sel/urg 保持可辨。
- [x] 切换 tag/占用变化不抖动（`tagdotswidth` 不动）。
- [x] `make clean && make` 零警告，`make test` 全绿。

## Open Questions
1. 默认 `r=1` 在你屏上够吗？不够改 `2`。
