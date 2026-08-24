# Tasks: dwm Tag 双向滑动动画

## Task 1: showhide() 改为 unmap/map

**Description:** 重构 showhide()，隐藏分支用临时屏蔽 UnmapNotify 的 unmap（新 helper
unmapwindow_quiet），显示分支补 XMapRaised。不设 IconicState。保留 updateicon 与递归结构。

**Acceptance criteria:**
- [ ] showhide() 隐藏分支对非 ISVISIBLE 窗口执行 XUnmapWindow（而非移屏外）
- [ ] showhide() 显示分支对 ISVISIBLE 窗口执行 XMapRaised
- [ ] unmapwindow_quiet 临时屏蔽 root SubstructureNotifyMask + 窗口 StructureNotifyMask
- [ ] 不调用 setclientstate(IconicState)，HIDDEN(c) 语义不变

**Verification:**
- [ ] make 编译通过（无新 -Wall 告警）
- [ ] 手动：切 tag 后窗口仍在管理（bar 上 tag 占位正常，切回窗口还在）

**Dependencies:** None

**Files likely touched:** `dwm.c`

**Estimated scope:** Small

## Task 2: view() 计算方向并写 _DWM_SLIDE_DIR 属性

**Description:** view() 在 arrange 前遍历 selmon 客户端，按 prevtag→curtag 计算方向，
对将隐藏/将显示窗口写属性 1/2，其余写 0。setup 中 XInternAtom 初始化属性原子。

**Acceptance criteria:**
- [ ] view() 中 dir = (curtag > prevtag) ? 1 : 2
- [ ] 将隐藏窗口（tags & old && !(tags & new)）写 dir，将显示窗口写 3-dir，其余写 0
- [ ] 仅处理 selmon 客户端

**Verification:**
- [ ] make 编译通过
- [ ] 用 xprop 观察切换后窗口属性值符合预期

**Dependencies:** Task 1

**Files likely touched:** `dwm.c`

**Estimated scope:** Small

## Task 3: picom.conf 新增 _DWM_SLIDE_DIR 滑动规则

**Description:** 在 rules 加一条 match `_DWM_SLIDE_DIR@` 的动画规则，值 1 与 2 分别定义
hide/show 的左右 offset-x 滑动（用 window-monitor-width 变量，保证多分辨率通用）。

**Acceptance criteria:**
- [ ] 规则仅匹配属性值为 1/2 的窗口（值 0 走全局 appear/disappear）
- [ ] hide/show 动画 offset 方向与 dir 语义一致

**Verification:**
- [ ] `picom --test-config` 或 picom 启动无配置报错
- [ ] 手动：切 tag 可见双向滑动

**Dependencies:** Task 2

**Files likely touched:** `~/.config/dwm/picom.conf`

**Estimated scope:** Small

## Task 4: 综合回归验证

**Description:** 全场景手动验证。

**Acceptance criteria:**
- [ ] 单 tag 双向切换滑动，方向跟随 prevtag→curtag
- [ ] 多 tag 组合切换（交集窗口不动画）
- [ ] 快速连续切换不崩溃、不丢窗口
- [ ] hidewin（手动隐藏）与 tag 切换互不干扰
- [ ] tagpreview、bar、systray 正常
- [ ] 多 monitor 不穿屏
- [ ] picom 停止时退化为标准 unmap/map

**Verification:**
- [ ] 逐项实测

**Dependencies:** Task 1, 2, 3

**Files likely touched:** 无（验证为主）

**Estimated scope:** Small
