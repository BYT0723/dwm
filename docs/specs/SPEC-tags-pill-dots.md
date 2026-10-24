# Spec: Tags pill 左右结构（左选中信息 + 右圆点总览）

## ASSUMPTIONS I'M MAKING
1. 改造对象是左区 `BarTags` pill（用户已确认 A），不是中区 `BarTabs`。
2. 右侧画 `LENGTH(tags)` 个圆（当前 9 个），全部常显，忽略 `policy_hide_vacant_tags`（用户已确认）。
3. 圆点语义：空心 = 无窗口，NORM 实心 = 有窗口非当前，SEL 实心 = 当前选中；urgent 用 invert 填充（用户已确认“可以”）。
4. 仅圆点可点击跳转（`ClkTagBar` + `1<<i`），hitbox 独立于视觉半径；左段不注册点击 slot（用户已确认）。
5. 多选 tagset 时左段不显示任何名称，选中组合状态完全由右侧多个 SEL 实心圆表达（用户已确认）。
6. 点区背景跟随 pill bg，靠点色区分状态（用户已确认 A）。
7. 单分支 `feat/tags-pill-dots`，工作区 `~/Workspace/Worktrees/dwm-feat-tags-pill-dots`，不在 `master` 上开发。
→ 有误现在纠正，否则按此推进。

## Objective
把 `BarTags` 从 9 个 `{icon} {name}` 平铺改为一颗左右结构 pill：
左边只画选中 tag 的 `[icon name]`（`tag_text` 模板展开），右边画一排状态圆点。
解决 9 tag 全铺太宽、占用左区空间的问题；圆点一眼看占用/选中总览，且宽度固定防抖动。
用户：本 fork 日常使用者（9 tag、多窗口）。
成功：选中信息可读、9 点状态正确、可点击、宽度稳定、构建测试全绿。

## Tech Stack
- C11 + X11/Xft/Xrender，本仓库 dwm fork（1TBS、2 空格、无 tab）。
- 绘制原语：`drw_text`、`drw_rounded`/`drawleftcap`、实心 `drawdisc(dwm.c:2694)`、空心 `drw_circle_empty(drw.h:76)`；配色 `SchemeTagSel/SchemeTagNorm`。

## Commands
- Build: `make clean && make`（必须零警告，仓库门禁）
- Test: `make test`
- Check: `git diff --check`
- 目检（可选）: `./tests/dwm-smoke.sh`（需 Xvfb）

## Project Structure
- `dwm.c` → `drawtags()` 按 mode 分支：Full 分支逐行保持现状；Compact 分支画左右两段（左文本 + 右圆点）；`tagswidth()` 同步同口径（`segdrawn/barpillwidth/hasdrawnafter` 经由它自动一致）；圆点绘制复用 `tabseldot_paint/tabhiddot_paint` 旁的画圆模式。
- `config.h` → 新增 `TagPillMode` 开关（`TagPillFull` = 现状 9 tag 平铺，`TagPillCompact` = 本 spec 左右结构，默认 Compact）+ 圆点几何 `tag_dot_radius = 4`、`tag_sel_dot_radius = 6`（选中点独享大半径，同 pitch 格居中，钳制不蹭邻居/右 cap）、`tag_dot_gap = 6`；`tag_text`、`tags[]`、`tag_names[]` 不动。命名/注释风格跟随 `TabMode`（`config.h:278-283`）。
```c
typedef enum { TagPillFull, TagPillCompact } TagPillMode;
static TagPillMode tag_pill_mode = TagPillCompact;
```
- `docs/specs/SPEC-tags-pill-dots.md` → 本文件。

## Code Style
沿用 AGENTS.md：函数签名单行 + 左花括号同行、2 空格、无 tab、不重排无关行。示例：
```c
static void tagdot_paint(int scm, int cx, int cy, int r, int filled) {
  if (r <= 0)
    return;
  if (filled)
    tabseldot_paint(scm, cx, cy, r);
  else
    drw_circle_empty(drw, cx, cy, r);
}
```

## Testing Strategy
- `make clean && make` 零警告；`make test` 全绿；`git diff --check` 干净；`git diff --stat` 无无关文件。
- X 绘制无新增单元测试，以构建 + 存量测试 + 代码走读验证。
- Xvfb 目检矩阵：空视图（9 空心）、部分占用（NORM 实心混空心）、选中切换（SEL 实心跟随）、urgent（invert 填充）、多选 tagset（左段无文本 + 多 SEL 点）、窄 bar（圆点不溢出）、Full 模式（与现状逐像素一致）。

## 需求明细（已确认）
1. 左段（仅 Compact，单选时）：`tagindex(tagset)` 最低位选中 tag，用 `tag_text` 模板展开绘制，scheme `SchemeTagSel`；urgent 时 `invert`（沿用 `dwm.c:1502` 反色）。左段 cell 定宽 = 9 个 tag 文本的最大 `TEXTW`（`tagleftw()`），短名左对齐、余下留空，切换 tag 总宽恒定。Full 模式下绘制逻辑保持现状不变。
2. 左段（Compact，多选时）：不显示单个 tag 名，改画 `tag_combo_text`（默认 `" combo ×{count}"`，config 可改；占位符 `{count}`）；左段保留完整 cell 宽度（cell 取单名最大宽与满数 combo 文本的最大者，永不截断）；任一被看 tag urgent 时整段 invert；选中组合由右侧多个 SEL 实心圆表达；不注册点击 slot。
3. 右段（仅 Compact）：`LENGTH(tags)` 个圆横排，顺序即 tag 序号；i 状态由 `occ/urg/sel` 决定，优先级 `sel > urg > occ > empty`。
4. 填充（点区底为 `SchemeTagNorm` bg）：`empty` 空心（`SchemeTagNorm` fg 描边）；`occ` 实心（`SchemeTagNorm` fg 白）；`sel` 实心（`SchemeTagSel` bg 蓝）；`urg` 实心蓝填 + `SchemeTagNorm` fg 白圈（黑底上沿用“填+对比圈”以区别于纯 SEL）。左段保持 `SchemeTagSel`；pill 尾部右 cap 用 `SchemeTagNorm` bg。
5. 几何：`tag_dot_radius = 4`、`tag_sel_dot_radius = 6`、`tag_dot_gap = 6`；圆点区宽度固定 = `n*d + (n-1)*gap + 2*lpad + pillr`（尾部多留 `pillr` 给右 cap，last 与非 last 点位一致），不随 `occ/sel` 变化；所有圆点在 pitch 格内居中，选中点大半径仍在格内；hitbox 每点 ≥10px；`bh` 过小时钳制直径。
6. 点击：每圆独立 `addslot(m, x, w, ClkTagBar, {.ui = 1u<<i})`，与现有 `dwm.c:1503` 语义一致；左段不注册 slot（纯展示）。
7. 胶囊：左右同属一颗 pill，共享外 caps + `drawtabborder`；`first/last` 落到实际绘制段（参考 `drawbarpill` 的 `appendcap` 逻辑）；`flatbar` 下全 bleed 无 caps。Full 模式下 caps/测量/绘制全部保持现状。
8. `tagswidth()` 必须与 `drawtags()` 同口径，否则 `barzonewidth`/`centerx` 错位；按 `tag_pill_mode` 分支，Full 分支保持现有累加逻辑逐字不动。
9. 运行时切换：本次只做 `config.h` 编译期开关，不做按键运行时翻转（`tab_mode` 有 `toggletabmode`，本次不跟随）。

## Boundaries
- Always: 只动 `BarTags` 的测量 + 绘制，并按 `tag_pill_mode` 分支；Full 分支逐行保持现状；`occ/urg` 累加逻辑不动；`ClkTagBar` 语义不动；`pillrun`/`drawbarpill` 框架不动。
- Ask first: 改 `BarTabs` 圆点语言统一半径；改 hover preview；运行时按键翻转 mode（本次不做）。
- Never: 不改 `policy_hide_vacant_tags` 本身语义（点区恒显是本次特例，仅限点区）；不改 `rules` 默认 tag；不顺手改格式/无关行；不直推 `master`。

## Success Criteria
- [x] `TagPillFull`（编译期切回）：与现状逐像素一致，9 tag 平铺、`policy_hide_vacant_tags` 行为不变。
  - 验证：Full/Compact 双编译零警告；diff 纯加法（129+/0-），Full 分支逐字未动。
- [x] `TagPillCompact`（默认）：单选时左段显示选中 tag 的 icon+name，选中切换跟随；多选时左段无文本，右侧多个 SEL 点表达组合。
  - 验证：Xvfb 目检（点击 dot3 → 左段切到 chat；super+shift+2 多选 → 左段无文本 + SEL@1,2）。
- [x] 右段恒为 N 个圆，`empty` 空心 / `occ` NORM 实心 / `sel` SEL 实心 / `urg` invert 填充可辨。
  - 验证：像素级分类（9 圆逐个采样中心+描边色）；urgent 经 `bellIsUrgent` + BEL 端到端触发。
  - 配色终版（黑底点区）：空心白圈黑心 / OCC 纯白 / SEL 纯 Sel 蓝 / URG 蓝填+白圈；左段 Sel 不变，右 cap 跟 Norm 底。
- [x] 每圆可点击且 slot 与 `1<<i` 一一对应；左段无 slot。
  - 验证：reset+click sweep（左段/间隙 ae=0，9 点区每点切换，zone 末端 282=dx+132 精确收尾）。
- [x] pill 总宽固定（`occ` 变化不抖动），caps/border 在圆角/直角/`flatbar` 下正确。
  - 验证：`BarModeFlat` + `pill_radius=0` 各 rebuild + smoke 全过并目检；左段定宽后切遍 9 tag + 多选，pill 左右缘逐张全等（`left=25/rightcap=287`，dx=152 九张一致）。
- [x] `make clean && make` 零警告，`make test` 全绿，`git diff --check` 干净。

## Open Questions
1. 多选时左段内容宽为 0：cap/pad 保留（胶囊左半只剩圆角）还是左段整体折叠、点区顶到左 cap？（我倾向保留 cap/pad，改动最小）
2. 左右段之间是否需要分隔符/额外间距（当前只有点区 `lpad`）？
3. 空心圆是否需要低 alpha（防 9 空心太抢）？
