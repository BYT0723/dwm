# Implementation Plan: dwm Tag 双向滑动动画

## Overview
将 dwm 的 tag 切换改为 KDE 式 unmap/map 模型：切换时对旧 tag 窗口 unmap、新 tag 窗口
map，作为 picom show/hide 动画的触发器；滑动效果由 picom 合成层视觉偏移实现，
窗口真实位置不动，因此不经过其他 monitor。方向由 view() 按 prevtag→curtag 计算，
写入窗口属性 `_DWM_SLIDE_DIR`（1/2），picom 规则按属性值应用左右滑动。

## Architecture Decisions
- **直接替换 showhide 的移屏外逻辑为 unmap/map**（用户决定，不保留旧逻辑）
- **不设 IconicState**：tag 切换的 unmap 保持 NormalState，避免污染 HIDDEN(c)/hidewin 语义
- **屏蔽 UnmapNotify**：unmap 前临时移除 root 的 SubstructureNotifyMask 与窗口的
  StructureNotifyMask（仿 hidewin），阻止 unmapnotify() 把窗口 unmanage
- **属性值 0/1/2**：每次 view 切换对 selmon 全部窗口写属性（0=不涉及、1/2=方向），
  值 0 不匹配 picom 规则，天然与普通窗口开关的 appear/disappear 隔离
- **不保留开关**（用户决定直接替换）

## Task List

### Phase 1: dwm 侧窗口生命周期改造
- [ ] Task 1: showhide() 改为 unmap/map（含 unmapwindow_quiet helper）

### Phase 2: 方向属性
- [ ] Task 2: view() 计算方向并写 _DWM_SLIDE_DIR 属性

### Checkpoint: dwm 编译 + 无 unmanage 回归
- [ ] make 通过，无 -Wall 新告警
- [ ] 切 tag 窗口不消失（不被 unmanage），hidewin/tagpreview 正常

### Phase 3: picom 动画规则
- [ ] Task 3: picom.conf 新增 _DWM_SLIDE_DIR 滑动规则

### Phase 4: 综合验证
- [ ] Task 4: 手动回归（双向滑动、多 tag、快速连续切换、多 monitor、picom 停止）

### Checkpoint: Complete
- [ ] 所有验收标准满足
- [ ] 交给用户实机验证视觉效果

## Risks and Mitigations
| Risk | Impact | Mitigation |
|------|--------|------------|
| unmap 触发 unmanage 丢窗口 | High | 屏蔽 UnmapNotify（hidewin 同款），Task 1 后立即验证 |
| picom 动画 match 语法/变量名错误 | Med | 参考讨论 #1364 与现有配置；先用固定方向验证再上方向 |
| 快速连续切换动画叠加异常 | Low | 边缘情况，人工验证 |
| 窗口 map 后栈序错乱 | Med | restack() 已统一处理，map 后用 XMapRaised |

## Open Questions
无（方向、替换策略均已确认）
