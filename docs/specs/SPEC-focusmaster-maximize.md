# Spec: per-tag 聚焦居中主窗（Mod+z → centeredfloatingmaster）+ 最大化（Mod+f → monocle）

> 档位 1 = focusmaster（宿主 centeredfloatingmaster，聚焦窗几何提升）。
> 档位 2 = maximize（宿主 **monocle**，全部平铺窗带 outer gap 叠放、接受透明穿透）。

## Objective

统一 per-tag "聚焦窗口占大比例"状态机，两键操作不同档位、不同宿主布局呈现：

- **`Mod+z`（focusmaster）**：进入"聚焦居中"模式——**自动切到 `centeredfloatingmaster`
  布局**，并在该布局内做**几何提升**（`m->sel` 占据中央主区，其余窗口留在两侧，
  **不改变 `m->clients` 列表顺序**）。此后每次切焦点，新聚焦窗几何跟随为中央主窗。
  再次 `Mod+z` 退出：恢复进入前布局。
- **`Mod+f`（maximize）**：**自动切到 `monocle` 布局**，所有平铺窗全屏叠放
  （四周留 outer gap，原生 monocle() 已加 gap），聚焦窗置顶，其余窗口在其下层
  （透明窗口可透见）；再次 `Mod+f` 退出：恢复进入前布局。

## 交互 / 键位

| 按键 | 档位 | 动作 |
|---|---|---|
| `Mod+z` | 0 ↔ 1 | 进入：记录当前布局，切到 centeredfloatingmaster + 几何提升；退出：恢复记录布局 |
| `Mod+f` | 0 ↔ 2 | 进入：记录当前布局，切到 monocle（带 gap 叠放）；退出：恢复记录布局 |

档位 1 与 2 互斥（进入一方自动清另一方）。焦点切换（`Mod+j/k`、鼠标）后
`focus()` 触发 `arrange`，几何提升/叠放置顶跟随。两者均仅当前 tag 生效。

## 布局切换与恢复

两档位均带**宿主布局 + fmlast 恢复**，对称实现：

| 档位 | 宿主布局 | 进入动作 |
|---|---|---|
| 1 (Mod+z) | centeredfloatingmaster | 记录当前布局到 `fmlast[curtag]` → 切到 CFM |
| 2 (Mod+f) | monocle | 记录当前布局到 `fmlast[curtag]` → 切到 monocle |

退出（再按同键）→ 清档位 + `setlayout` 回 `fmlast[curtag]` 记录布局。

- 档位 1 与 2 互斥（进入一方直接覆盖另一方档位值并按其宿主布局切换）。
- **几何提升代码同时存在于 tile() 与 centeredfloatingmaster()**（档位 1 不绑定宿主布局，
  手动切到 tile 后档位 1 提升仍生效）。档位 2 由 monocle 宿主承担，tile()/CFM
  不再含 maximize 屏外逻辑。
- `fmlast` 随 tag 独立；焦点切换（Mod+j/k、鼠标）后 focus() arrange，聚焦窗跟随。
- 手动 setlayout 切换布局时档位**保留**（几何逻辑仅在宿主布局可观，其它布局不读取、无脏状态）。

## 宿主布局改动

- `dwm.c` `monocle()`：**仅在作为 maximize 宿主（档位 2, `focusmaster==2`）时**带
  outer gap 叠放（每窗四周缩进 `gappoh/gappov`）。普通手动切 `[M]` 与
  `Mod+Ctrl+f`(fullscreen，内部切 monocle) 处于档位 0 → **无 gap 铺满**。
- 已知边界（接受）：档位 2 激活期间按 fullscreen 仍带 gap（档位未清，避免为
  fullscreen 暂存/恢复单主状态的额外复杂度）。

## 几何提升（档位 1，tile + centeredfloatingmaster）

- `centeredfloatingmaster()` 与 `tile()` 各自处理档位 1 时：`fm = focusclient(m)`、
  `nma = fm ? 1 : m->nmaster`，归属判定 `c == fm`（纯几何，**不改 m->clients 顺序**），
  聚焦窗独占主区，其余窗口等分进 stack 区，**不调 getfacts()**。
- 档位 0 行为与各布局原实现一致。
- 由 dwm.c 暴露的 `focusclient(m)` / `focusmode(m)` 复用。

## 涉及文件

- `dwm.c`：`Pertag` 增加 `const Layout *fmlast[LENGTH(tags)+1]`；createmon 初始化为 NULL；
  新增 `findlayout()`/`setfmmode(mode, arrange)`；
  `togglefocusmaster`→档位1(宿主 CFM)、`togglemaximize`→档位2(宿主 monocle)；`monocle()` 加 outer gap。
- `vanitygaps.c`：`centeredfloatingmaster()` 与 `tile()` 档位 1 几何提升（档位 2 屏外分支已移除）。
- `config.h`：`Mod+z`/`Mod+f` 键位保持（已在工作区）。

## 命令

```
Build: make
Clean: make clean
```

## Testing Strategy

`make` 零新增警告 + 用户 hot-restart（`Mod+Ctrl+q`）实机验证。

## Boundaries

- Always：不真 hide、不改 `m->clients` 顺序、档位仅在 tag 内、不新增依赖。
- Ask first：把 maximize 扩到非 tile 布局、几何提升扩到其它布局、动 pertag 双槽机制。
- Never：破坏普通 centeredfloatingmaster / tile；档位 1 期间布局偏离仍残留提升。

## Success Criteria

1. `make` 零新增警告。
2. `Mod+z` 首次：布局自动切到 centeredfloatingmaster，聚焦窗居中放大；
   `Mod+j/k` 循环切焦点时中央大窗跟随；再次 `Mod+z` 恢复进入前布局。
3. `Mod+f`：布局自动切到 monocle，全部平铺窗带 gap 叠放、聚焦窗置顶（透明可穿透）；
   再次 `Mod+f` 恢复进入前布局。
4. 档位 1 下手动切到 tile 布局：tile 内提升照常生效（tile 与 CFM 共享档位 1），无脏状态。
5. per-tag：A tag 档位不影响 B tag；档位 2 (monocle) 不影响 monocle 原生切换。
6. `Mod+Ctrl+f`(fullscreen) 可用：档位 0 下无 gap 铺满（见宿主布局改动）。

## Open Questions

- （已解决）档位 1 是否绑定 centeredfloatingmaster：否——tile 与 CFM 共享档位 1。
- （已解决）档位 2 实现方式：宿主 monocle（带 outer gap 叠放），非 tile offscreen。
