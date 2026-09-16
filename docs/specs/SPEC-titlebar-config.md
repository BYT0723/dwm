# Spec: titlebar 四配置项（icon 开关 / 文本模板 / 文本对齐 / 高度内边距）

## Objective
给 per-client frame titlebar 增加四个 `config.h` 配置项，把标题栏的可配置性对齐到 tab (`tabtext`) 已有的水平：
1. `showtitleicon` —— 是否在 titlebar 左侧绘制 client icon（默认 1，显示；icon 位置固定居左，不跟随对齐）；
2. `titlebartext[]` —— titlebar 文本模板，复用 `tabtext` 的 `{title}`/`{class}` 占位符（默认 `"{title}"`）；
3. `titlebaralign` —— **只管文字**对齐：0=贴左/1=居中/2=贴右（默认 1）；其中居中是相对**整个标题栏宽度**的真居中（无视按钮区），右对齐以按钮区左侧为右边界；
4. `titlebarpad` —— 每边内边距，titlebar 高度不再跟 `bh`，改为 `font_height + 2 * titlebarpad`（默认 6）。

成功标准：四个开关各自独立生效；`make` 零警告；Xvfb 回归无变化（CSD/预览/快照不受影响）。

## Tech Stack
- C (dwm fork, Xlib/Xrender)，无单元测试框架
- 涉及文件：`config.h`（新增四项）、`dwm.c`（`drawtitle()` + `setup()` 里 `th` 的计算）

## Commands
```
Build:  make clean && make        # 要求零新增 warning
Smoke:  Xvfb :99 + xterm fixture + xdotool/xwd 截图验证
```

## Project Structure
```
dwm.c            → drawtitle() 是唯一行为改动点（+复用 tab_placeholder）
config.h         → // title bar 段新增三项
docs/specs/      → 本文件 SPEC-titlebar-config.md
tasks/           → plan-titlebar-config.md / todo-titlebar-config.md（不碰 tasks/plan.md、tasks/todo.md，那是 CSD 的）
```

## Code Style
沿用现有惯例，示例：
```c
// config.h // title bar 段
static const int  showtitleicon = 1;          /* 1 = draw the client icon in the titlebar */
static const char titlebartext[] = "{title}"; /* placeholders: {title}, {class} (same as tabtext) */
static const int  titlebaralign = 1;          /* 0 = left, 1 = center, 2 = right */
static const unsigned int titlebarpad = 6;    /* per-side padding; height = font height + 2 * titlebarpad */
// dwm.c drawtitle(): template_expand(titlebartext, tab_placeholder, c, ...)
// 对齐只改变文本起点 cx 的计算，按钮区与 icon 布局逻辑不动
// dwm.c setup(): th = showtitlebar ? drw->fonts->h + 2 * titlebarpad : 0（不再跟 bh）
// 按钮区/frame/client 几何都经 th/titleh() 间接跟随，无需逐处改
```

## Testing Strategy
本仓库无测试框架（纯 X11 C），采用等价的 RED→GREEN 验证：
- RED：每个增量先在 Xvfb 下演示"现状不符合"（如 icon 仍显示、模板不生效、对齐不分档）；
- GREEN：改完后同 fixture 验证符合 + `make` 零警告；
- 回归：CSD 最小化/最大化、hover 预览、隐藏快照、clean exit 走一遍已有脚本。

## Boundaries
- Always: 只碰 `drawtitle()` 与 `config.h` title bar 段；每增量构建验证；默认行为按本 spec（用户已确认）；
- Ask first: 改动 tab/tag 绘制、改动三个默认之外的默认值、新增占位符；
- Never: 动 `tasks/plan.md`、`tasks/todo.md`（CSD 未结）；提交 secrets；为验证杀用户正常 dwm/X。

## Success Criteria
- [ ] `showtitleicon=0` 时 titlebar 无 icon（即使 client 有 icon）；`=1` 时 icon 固定居左绘制（含 `ICONSPACING` 间距），且不随 `titlebaralign` 移动。
- [ ] `titlebartext="{class} - {title}"` 渲染出如 `XTerm - foo`；未知占位符按 `template_expand` 现状原样保留。
- [ ] `titlebaralign=0/1/2` 文字分别贴左/真居中（整宽）/贴右（以按钮区左侧为右边界）；三档下 icon 红块位置不变。
- [ ] 默认 `titlebarpad=6` 时 titlebar 高度 `== drw->fonts->h + 12`（不再等于 `bh`）；frame/client 几何与按钮区自动跟随；改大 pad 后标题栏变高且内容不错位。
- [ ] `make clean && make` 零警告；Xvfb 回归（CSD/预览/快照/clean exit）全绿。

## Assumptions
1. 对齐枚举用 `0/1/2 = 左/中/右` plain int，不复用 `TAB_CENTER` 位掩码。
2. 默认值（icon 开、对齐居中）接近旧外观，区别是标题栏更矮（font+12 vs `bh`=font+16）——用户已 hand-tune 确认。
3. icon 开关只影响 titlebar，不影响 bar tab 里的 icon。
4. `titlebarpad` 是每边 pad，`th = font_height + 2 * titlebarpad`（与 `bh = font + barfontpad * 2` 同惯例），`font_height` 即 `drw->fonts->h`。
5. icon 仍按 `ICONSIZE`（基于 `bh`）绘制；默认 `th`=font+12 已高于 icon（font+10），不裁剪；若把 pad 调得很小（`th < icon 高`），icon 上下会被对称裁剪——默认不受影响，不在本任务内做缩放。
6. 相关修复（同树，单独提交）：`tabradius=0` 时 host 左侧 cap 区无人绘制、露出黑底；修法是 host 的 `drw_text` 按 `tabr` 缩进（`lpad=tabr, skip_pad=(tabr>0)`），与 `drawtabs` 的 `cxx = x + tabr` 同 pattern。默认 `tabradius` 现为 6。

## Open Questions
- 无（命名 `showtitleicon` / `titlebartext[]` / `titlebaralign` 按问答确认；有异议请在 review 时提出）。
