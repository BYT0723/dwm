# Performance Log

## drawstatusbar: malloc → stack buffer (2026-08-12)

| Idea | Baseline → Result | Verdict | Why |
|------|-------------------|---------|-----|
| malloc/free → char[1024] stack buf | drawstatusbar 1188µs → 962µs avg | **kept** | -19.0%, far exceeds run-to-run noise. drawbar also dropped 14.2% as side effect. |

### Baseline (437 calls)
- drawstatusbar: 519.2ms total, 1188µs avg
- drawbar: 647.3ms total, 1321µs avg

### After (454 calls)
- drawstatusbar: 436.8ms total, 962µs avg
- drawbar: 577.1ms total, 1134µs avg

### Other measurements (not actionable)
- getfacts: 57µs avg — double-traversal concern was unfounded, negligible
- manage XInternAtom: not worth caching, infrequent calls
- drw_map XSync: expected cost, no room for improvement

## drw_clr_create: 缓存 XftColor (2026-08-17)

| Idea | Baseline → Result | Verdict | Why |
|------|-------------------|---------|-----|
| hex 色名+alpha → 64 项静态缓存 | drw_clr_create 18.4% → 0%（XftColorAllocName 采样 0 次，两轮确认） | **kept** | 状态栏 `^c#xxxxxx^` 颜色码固定，命中率≈100%；彻底消除每帧 XftColorAllocName 的 X 往返 |

### 方法
`perf record -p <dwm> -i -F 99 --call-graph dwarf` 采样 3-5min（`-i` 排除 dwm 子进程，否则被 fork 的终端稀释统计）。

### Baseline (2026-08-17 12:09-12:12, 92 samples)
- drw_clr_create: 18.4% children（XftColorAllocName，每次状态刷新对每个颜色码一次 X 往返）
- drawbar: 37.7%, drawstatusbar: 29.8%, gettextprop: 28.2%

### After (12:22-12:37, 96+97 samples, 两轮)
- drw_clr_create / XftColorAllocName: 采样 **0 次**

### 实现
drw.c `drw_clr_create`：静态 64 项缓存（name[8] + alpha + Clr），命中直接拷贝 `XftColor` 返回；仅缓存 `strlen < 8` 的 hex 色名，命名色走原路径。

### 备注（未采取）
- 状态栏整串文本宽度缓存：状态文本每次刷新都变，命中率≈0，放弃
- 逐字符字形宽度缓存：命中率高但仅能省 ~5%（drw_font_getexts→XftGlyphExtents），fallback 链复杂，不值

## drw_text: 单字节字符宽度缓存 (2026-08-17)

| Idea | Baseline → Result | Verdict | Why |
|------|-------------------|---------|-----|
| 缓存单字节字符在主字体上的宽度 | 字体测量 self ~54% → ~5%，drw_fontset_getwidth 60.8% → 9.9% children | **kept** | drw_text 是逐字符测量（render=0 测量 + render=1 绘制各一遍），每字符 XftCharExists+XftTextExtentsUtf8（内部 FcCharSetHasChar）。状态栏字符集有限，主字体渲染的 ASCII 字符第二帧起全命中，跳过 Xft 调用 |

### 方法
同前（`perf record -p <dwm> -i -F 99 --call-graph dwarf` 5min）。

### Baseline (dwm4, 2026-08-17 12:22-12:29, 96 samples)
- drw_fontset_getwidth: 60.79% children / drw_font_getexts 13.05%
- FcCharSetHasChar ~36% + XftCharExists 7.7% + XftGlyphExtents 10.3% (self)

### After (dwm6, 13:05-13:10, 108 samples)
- drw_fontset_getwidth: 9.88% children (self 0.00%) / drw_font_getexts 4.20%
- fontconfig self ~5.1% (0xeae0 2.5% 等) + XftCharExists 0.02%；XftGlyphExtents 无 self

### 实现
drw.c `drw_text` 逐字符循环：`charwidth[256]/charvalid[256]` 静态缓存，仅当 `usedfont==drw->fonts`（纯主字体、无 fallback 切换）且单字节字符时读写；命中分支复刻原 `curfont==usedfont` 分支语义（ellipsis/overflow/截断一致）。

### 备注
- 剩余 ~5% 字体调用为非 ASCII/fallback 字符路径，不在缓存范围
- 另发现绘制期二次测量（TEXTW 布局 + drw_text 内部）已被本缓存吸收，无需方案 2
- 方案 2（绘制期跳过整段测量 / 全 ASCII 整段快速路径）评估后**放弃**：方案 1 后 drw_fontset_getwidth self 0.00%，二次测量已零 Xft 成本；且 fonts 配置 3 字体（Symbols/CaskaydiaCove/Noto CJK）混排，快速路径前提受限，剩余收益（纯 C 循环）<2%，在采样噪声内
