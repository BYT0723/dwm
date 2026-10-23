# Spec: policy 控制空 tag 是否隐藏

## Objective
给 bar tags 增加一个 `policy_` 开关：`policy_hide_vacant_tags`，控制无窗口
且未选中的 tag 是否绘制。默认 `0 = 显示所有`（上游 dwm 行为），置 `1`
恢复本 fork 现状（只画 occupied + selected，`dwm.c:drawtags` 的
`continue` 分支）。解决“9 个 `{icon} {name}` 全显示太宽”与“隐藏后位置
漂移、鼠标不可达空 tag”之间的取舍，让用户自行选择。

## 现状（已验证）
- `dwm.c:1479-1481 drawtags()`：`if (!(occ & 1<<i || selected)) continue;`
  空 tag 直接跳过，不绘制也不注册 `ClkTagBar` slot。
- `dwm.c:1443-1450 tagswidth()`：同条件累加宽度；`segdrawn(BarTags)`、
  `barpillwidth()`、`hasdrawnafter()` 都经由它，隐藏时 pill/cap 自动收缩。
- `dwm.c:1715-1734 drawbar()`：`occ/urg` 按 monitor 上所有 client 的
  `c->tags` 累加，与显示策略无关。
- `dwm.c:3190-3207 preview snapshot`：按 `occ` 决定抓拍，与显示策略无关，
  本次不动。
- `config.h:459-464 policy` 区：`policy_show_hidden / policy_attach_top /
  policy_follow_moved_client / policy_jump_on_activate`，新开关放同一区，
  命名跟随 `policy_` 前缀。

## Commands
- Build: `make clean && make`
- Test: `make test`
- Check: `git diff --check`

## Project Structure
- `config.h` → policy 区新增 `policy_hide_vacant_tags`（默认 `0`）。
- `dwm.c` → `tagswidth()` 累加条件、`drawtags()` 的 `lastvis` 循环与
  `continue` 跳过条件分支化；`segdrawn` 等经由 `tagswidth` 自动一致，
  不单独改。
- `docs/specs/SPEC-hide-vacant-tags.md` → 本文件。

## Code Style
沿用本仓库 AGENTS.md：1TBS 大括号（函数签名单行 + 左花括号同行）、
2 空格缩进、无 tab、不重排无关行。示例：
```c
static const int policy_hide_vacant_tags = 0;  /* 1 = hide tags with no windows unless selected; 0 = always show all tags */
```
```c
int show = !policy_hide_vacant_tags || (occ & 1 << i) || (m->tagset[m->seltags] & 1 << i);
if (!show)
  continue;
```

## Testing Strategy
- `make clean && make` 零警告（仓库门禁）。
- `make test` 全绿（`barparse_test` + 脚本检查，回归保障）。
- 本改动为 X 绘制，无新增单元测试；以构建 + 存量测试 + 代码走读验证。
  可选 Xvfb 目检：`0` 时空视图 9 个 tag 全显示；`1` 时只显示选中 1 个。

## Boundaries
- Always: 只分支化“是否可见”判定；可见性定义保持
  `occupied || selected` 不变；`urg` 高亮逻辑不动。
- Ask first: 改 hover preview 抓拍范围（本次不改）；改 `rules` 默认 tag。
- Never: 不改 `occ` 累加逻辑；不改 `ClkTagBar` slot 语义；不顺手改格式/
  无关行；不在 `master` 上直接开发（本任务分支 `feat/hide-vacant-policy`）。

## Success Criteria
- [ ] `policy_hide_vacant_tags = 0`（默认）：空视图下 9 个 tag 全部绘制，
  点击 slot 与 `1<<i` 一一对应。
- [ ] `= 1`：空 tag 不绘制、不占宽、不注册 slot；occupied/selected 正常
  高亮，pill caps 收缩正确。
- [ ] 两种取值下 `make clean && make` 零警告，`make test` 通过，
  `git diff --check` 干净。

## Open Questions
1. 开关类型用 `int`（与其他 `policy_` 一致）可否？默认 `0` 显示所有可否？
2. 命名 `policy_hide_vacant_tags` 可否，还是更倾向 `policy_show_all_tags`？
