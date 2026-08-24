# Spec: dwm Tag 双向滑动动画（KDE 式 unmap + picom show/hide）

## Objective
让 dwm 的 tag 切换拥有方向一致的双向滑动动画（旧窗口滑出 + 新窗口滑入），
且动画不经过其他 monitor。机制：dwm 在切换 tag 时对旧窗口 unmap、新窗口 map，
作为 picom show/hide 动画的触发器；滑动效果由 picom 合成层视觉偏移实现，
窗口真实位置不动。方向语义：per-monitor 的滑动方向由 prevtag→curtag 决定。

Success（验收标准）:
- 切 tag 时旧窗口被 unmap（且不会被 unmanage 移出管理）
- 新 tag 窗口被 map
- picom 对 tag 切换窗口触发 show/hide 滑动动画，方向跟随 prevtag→curtag
- 动画为纯视觉偏移，窗口真实位置不跨 monitor
- hidewin()/HIDDEN/tagpreview/restack 等现有逻辑不受影响
- 快速连续切换 tag 不崩溃

## Tech Stack
- C（dwm fork，suckless 风格），X11/Xlib，Xinerama
- 外部依赖：picom（合成器，提供 show/hide 动画引擎）

## Commands
```
Build: make
Install: make install (或 ./install.sh)
Check: make 后手动验证（见 Testing Strategy）
```

## Project Structure
```
dwm.c    → 主逻辑：showhide()/view()/hidewin()/unmapnotify() 等
config.h → 开关（ANIMATE_TAGSLIDE）与常量
picom.conf（~/.config/dwm/picom.conf，仓库外）→ show/hide 滑动规则
docs/ideas/tag-slide-animation.md → 本 spec
tasks/plan.md、tasks/todo.md → 计划与任务
```

## Code Style
dwm fork 惯例：Allman 大括号、2 空格缩进、snake_case、函数无 static 前缀（本 fork）。
示例（本次核心改动）：
```c
/* 临时屏蔽 UnmapNotify，避免 unmapnotify() 把窗口 unmanage */
static void unmapwindow_quiet(Client *c) {
  Window w = c->win;
  XWindowChanges ...; /* 事件掩码读取/恢复 */
  XUnmapWindow(dpy, w);
}
```

## Testing Strategy
dwm 无单测框架。验证分三层：
1. 编译：`make` 通过（含 -Wall 无新告警）
2. 静态核对：diff review，确认 unmap 路径不触发 unmanage
3. 手动行为验证（清单见 Success Criteria，逐项实测）

## Boundaries
- Always：修改前先读相关函数上下文；编译通过后才算完成
- Ask first：改 unmapnotify() 事件处理逻辑（涉及窗口生命周期，风险最高）
- Never：不触碰 hidewin() 的 IconicState 语义；不提交未授权的调试改动

## Open Questions
（均已确认）方向=prevtag→curtag；直接替换移屏外逻辑；新分支 feat/tag-slide-animation
