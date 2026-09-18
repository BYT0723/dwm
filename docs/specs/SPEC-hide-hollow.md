# Spec: TabModeIcons 隐藏 client 空心圆标识

## Objective
在 `TabModeIcons` 模式下，已隐藏（`HIDDEN(c)`）client 的图标下方绘制一个空心圆，
与选中实心圆点形成对称语言：实心 = 选中，空心 = 隐藏。解决只靠半透明难以区分
隐藏/普通的问题。保留现有半透明，不替代。

## 现状（已验证）
- `config.h:170`：`[SchemeHid] = { BG_ALPHA(0xd0), TAB_HID_BG_ALPHA(0x66), TRANSPARENT }`
- `dwm.c:updateicon()`：`hidden → scm=SchemeHid`，`new_alpha = alphas[scm][0] = 0xd0`，
  `geticonprop(..., new_alpha, ...)` 经 `prealpha()` 取 `min(pixel alpha, 0xd0)`。
- 结论：**hide icon 已设置透明度**，`0xd0 ≈ 81.6%`，普通/选中为 `OPAQUE 0xff`。
- 选中标记：`tabseldot_paint()` 只在 `TabModeIcons`、且 `scm==SchemeSel && tabseldot>0`
  时绘制实心圆，位置为图标下方 `top+ih+gap+r`，颜色为 `SchemeSel` 前景（`col_black`）。

## 需求（用户确认）
1. 范围：仅 `TabModeIcons`。
2. 样式：1px 描边空心圆，颜色用 `SchemeSel` 前景，与选中实心点同色系、同半径 `tabseldot`、同位置。
3. 保留现有 `0xd0` 半透明，双重标识。

## Commands
- Build: `make clean && make`
- Test: `make test`
- Smoke: `./tests/dwm-smoke.sh`（需 Xvfb 等，可选）

## Project Structure
- `dwm.c` → tab 绘制（`tabdraw()` icons 分支、`tabseldot_paint` 旁新增 `tabhiddot_paint`）
- `drw.c/.h` → 新增 `drw_circle_empty()`（`XDrawArc` 1px 描边）
- `config.h` → 不新增配置项，复用 `tabseldot`

## Code Style
沿用现有模式：`tabseldot_paint` 用 scratch scheme 画实心；空心用 `drw_setscheme(SchemeSel)`
+ `XDrawArc`，`r<=0` 直接返回。示例：
```c
static void tabhiddot_paint(int cx, int cy, int r) {
  if (r <= 0) return;
  drw_setscheme(drw, scheme[SchemeSel]);
  drw_circle_empty(drw, cx, cy, r);
}
```

## Testing Strategy
- `make clean && make` 零警告（本仓库门禁）。
- `make test` 全绿（`barparse_test` + `check-status-ids.sh`，回归保障）。
- 本改动为 X 绘制，无新增单元测试；以构建 + 存量测试 + 代码走读验证。
  可选 Xvfb 截图目检：隐藏 client 图标下方出现与选中点同位置的空心圆。

## Boundaries
- Always: 只动 tab icons 分支；`tabseldot==0` 时不画（与选中点一致）；保持 `drw->scheme` 为 `SchemeSel`。
- Ask first: 新增 `config.h` 可调半径/颜色（本次不做，复用 `tabseldot` + `SchemeSel` fg）。
- Never: 不改 `updateicon` alpha 逻辑；不改 `TabMode` 文字模式；不改选中实心点行为（hidden 优先显示空心）。

## Success Criteria
- [ ] `TabModeIcons` 下 hidden client 图标下方有 1px 空心圆，普通 client 无。
- [ ] 空心圆半径/位置与选中实心点一致，颜色为 `SchemeSel` fg。
- [ ] hide 半透明（`0xd0`）保持不变。
- [ ] `tabseldot=0` 时不绘制，不留错位。
- [ ] `make clean && make` 零警告，`make test` 通过。

## Open Questions
无（范围/样式/透明度三问已确认）。hidden+selected 并发时 hidden 优先显示空心（hide 会清 sel，罕见）。
