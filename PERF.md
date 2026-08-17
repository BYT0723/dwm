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
