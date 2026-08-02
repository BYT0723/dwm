#!/bin/bash
# ╔══════════════════════════════════════════════════════════════╗
# ║            walter-arch-bootstrap                            ║
# ║            在裸 Arch Linux 上重建完整 dwm 桌面环境           ║
# ╚══════════════════════════════════════════════════════════════╝
#
# 用法:
#   ./install.sh              # 交互式选择安装阶段 (dialog UI)
#   ./install.sh --all        # 全自动安装所有阶段
#   ./install.sh --cn         # 启用国内镜像 (GitHub/archlinuxcn 加速)
#   ./install.sh --all --cn   # 全自动 + 国内镜像
#   ./install.sh --dry-run    # 预览模式，不实际执行
#   ./install.sh --help       # 显示帮助
#
# 设计原则:
#   1. 按依赖层次组织 — 先编译依赖，后运行时，最后编译 dwm
#   2. 每个包都标注消费者 — # → ~/.dwm/tools/wallpaper.sh
#   3. 幂等 — pacman/paru 原生支持重复安装
#   4. 失败即停 — set -euo pipefail，哪步错报哪步
#   5. 先 dialog 后选阶段 — 安装 dialog 后展示 checklist UI
#
# 依赖层次:
#   Phase 0  → 前置检查 + paru + dialog + sing-box
#   Phase 1  → dwm 编译依赖 (X11 libs / imlib2 / fontconfig)
#   Phase 2  → X11 Server + 图形工具 (xrandr/xdotool/xclip/...)
#   Phase 3  → 字体 (Noto / Nerd Font / CJK)
#   Phase 4  → Shell 环境 (zsh + starship + zim)
#   Phase 5  → 终端与 CLI 工具 (kitty/tmux/jq/fzf/...)
#   Phase 6  → 桌面组件 (picom/dunst/rofi/锁屏/通知)
#   Phase 7  → 应用与工具 (音频/媒体/输入法/网络/开发)
#   Phase 8  → 编译安装 dwm
#   Phase 9  → 部署配置 (克隆仓库 → dotfile make install)
#   Phase 10 → 验证 (关键命令可执行性检查)

set -euo pipefail

# ═══════════════════════════════════════════════════════════
# 配置 — 如需定制请修改此处
# ═══════════════════════════════════════════════════════════
readonly DOTFILE_REPO="https://github.com/BYT0723/dotfile.git"
readonly DWM_REPO="https://github.com/BYT0723/dwm.git"
readonly SCRIPTS_REPO="https://github.com/BYT0723/scripts.git"
readonly CN_MIRROR="https://repo.archlinuxcn.org/\$arch"
readonly WORKDIR="/tmp/walter-bootstrap"
# GitHub 加速镜像 — 由 --cn 参数自动设置
# 国内用户可使用: https://ghfast.top/ 或 https://ghproxy.com/
GH_PROXY=""

# ANSI
readonly R='\033[0;31m' G='\033[0;32m' Y='\033[1;33m' C='\033[0;36m' B='\033[1m' N='\033[0m'

# ═══════════════════════════════════════════════════════════
# 工具函数
# ═══════════════════════════════════════════════════════════
say() { echo -e "${G}==>${N} $*"; }
warn() { echo -e "${Y}⚠${N}  $*"; }
die() {
	echo -e "${R}✗${N}  $*" >&2
	exit 1
}
phase() { echo -e "\n${C}══════${N} $* ${C}══════${N}"; }

clone_or_pull() {
	local repo="$1" dest="$2"
	# GitHub 加速代理 → 解决国内 github.com 不可达
	if [ -n "$GH_PROXY" ] && [[ "$repo" == https://github.com/* ]]; then
		repo="${GH_PROXY}${repo}"
	fi
	if [ -d "$dest/.git" ]; then
		say "更新: $dest"
		git -C "$dest" pull --ff-only
	else
		say "克隆: $repo → $dest"
		git clone "$repo" "$dest"
	fi
}

# pacman 包装 (官方仓库)
pkg() {
	if $DRY; then
		echo "  [DRY] sudo pacman -S --noconfirm --needed $*"
	else
		sudo pacman -S --noconfirm --needed "$@"
	fi
}

# paru 包装 (官方 + AUR)
aur() {
	if $DRY; then
		echo "  [DRY] paru -S --noconfirm --needed $*"
	else
		paru -S --noconfirm --needed "$@"
	fi
}

# ═══════════════════════════════════════════════════════════
# CLI 参数解析
# ═══════════════════════════════════════════════════════════
ALL=false DRY=false CN=false

for arg in "$@"; do
	case "$arg" in
	--all | -a) ALL=true ;;
	--dry-run | -n) DRY=true ;;
	--cn) CN=true ;;
	--help | -h)
		sed -n '2,21p' "$0"
		exit 0
		;;
	*) die "未知参数: $arg (--all | --dry-run | --cn | --help)" ;;
	esac
done

$CN && GH_PROXY="https://ghfast.top/" && say "启用国内镜像 (GitHub 加速: ${GH_PROXY})"
$DRY && warn "DRY-RUN 模式 — 仅打印操作，不修改系统"

# ═══════════════════════════════════════════════════════════
# Phase 0: 前置检查 + paru + dialog
# ═══════════════════════════════════════════════════════════
phase0() {
	phase "Phase 0: 前置检查与基础工具"

	$DRY || [ "$(id -u)" -ne 0 ] || die "请以普通用户运行，提权时脚本自动调用 sudo"
	command -v curl &>/dev/null || pkg curl
	command -v git &>/dev/null || pkg git
	$DRY || command -v pacman &>/dev/null || die "仅支持 Arch Linux"

	$DRY || sudo -v
	mkdir -p "$WORKDIR"

	# dialog — 提供交互式 checklist UI
	if ! command -v dialog &>/dev/null && ! $DRY; then
		say "安装 dialog (交互式界面)"
		sudo pacman -S --noconfirm --needed dialog
	fi

	# archlinuxcn — 预编译 AUR 包 (paru, nerd-fonts, 等)
	if ! grep -q '\[archlinuxcn\]' /etc/pacman.conf 2>/dev/null; then
		say "启用 archlinuxcn 源"
		$DRY || echo -e "\n[archlinuxcn]\nServer = ${CN_MIRROR}" |
			sudo tee -a /etc/pacman.conf >/dev/null
	fi
	pkg archlinuxcn-keyring

	# paru — AUR helper
	if ! command -v paru &>/dev/null; then
		say "安装 paru (AUR helper)"
		if $DRY; then
			echo "  [DRY] 安装 paru"
		elif sudo pacman -S --noconfirm paru 2>/dev/null; then
			say "paru 安装完成 (archlinuxcn 预编译)"
		else
			warn "archlinuxcn 未提供 paru，从 AUR 构建..."
			pkg base-devel
			git clone https://aur.archlinux.org/paru.git "$WORKDIR/paru"
			(cd "$WORKDIR/paru" && makepkg -si --noconfirm)
			command -v paru &>/dev/null || die "paru 安装失败，请手动安装"
		fi
	fi

	# sing-box — 代理工具，提前安装以便后续 GitHub 操作走代理
	pkg sing-box
	#   → 包已安装，配置由 Phase 9 dotfile 部署
	#   → 如需立即使用代理，先手动写入 /etc/sing-box/config.json
	#     然后: sudo systemctl start sing-box
	#     再设置: export https_proxy=http://127.0.0.1:1080
	#   → 或依靠 GH_PROXY 镜像拉取 GitHub 仓库
}

# ═══════════════════════════════════════════════════════════
# 阶段选择 (dialog checklist)
# ═══════════════════════════════════════════════════════════
declare -A PHASE_NAMES=(
	[1]="编译依赖       (dwm 构建所需的 X11/imlib2/fontconfig 库)"
	[2]="X11 图形基础    (X Server + xrandr/xdotool/xclip 等)"
	[3]="字体            (Noto/Nerd Font/CJK)"
	[4]="Shell 环境      (zsh + starship + zim)"
	[5]="终端与CLI工具   (kitty/alacritty/tmux/jq/fzf/...)"
	[6]="桌面组件        (picom/dunst/rofi/i3lock/...) "
	[7]="应用与工具      (音频/媒体/输入法/网络/主题/...)"
	[8]="编译安装 dwm    (make clean install)"
	[9]="部署配置        (克隆仓库 → dotfile make install)"
	[10]="验证           (关键命令可执行性检查)"
)

select_phases() {
	if $ALL; then
		PHASES="1 2 3 4 5 6 7 8 9 10"
		return
	fi

	if ! command -v dialog &>/dev/null; then
		warn "dialog 不可用，默认全量安装"
		PHASES="1 2 3 4 5 6 7 8 9 10"
		return
	fi

	# 构建 dialog checklist 参数
	local args=()
	for i in {1..10}; do
		args+=("$i" "${PHASE_NAMES[$i]}" "on")
	done

	local choice
	exec 3>&1
	choice=$(dialog --title " walter-arch-bootstrap " \
		--checklist "SPACE 选择/取消   ENTER 确认\n
按依赖层次组织的安装阶段，首次安装建议全选\n
取消(ESC) = 默认全量安装:" \
		0 0 0 "${args[@]}" 2>&1 1>&3) || true
	exec 3>&-

	if [ -z "$choice" ]; then
		say "未选择任何阶段，默认全量安装"
		PHASES="1 2 3 4 5 6 7 8 9 10"
	else
		PHASES=$(echo "$choice" | tr -d '"')
		say "已选择阶段: $PHASES"
	fi
}

# ═══════════════════════════════════════════════════════════
# Phase 1: dwm 编译依赖
# ═══════════════════════════════════════════════════════════
phase1() {
	phase "Phase 1: dwm 编译依赖"

	# ── 构建工具链 ──
	pkg base-devel
	#   → make, gcc, pkgconf  (dwm/Makefile)

	# ── X11 库 ──
	pkg libx11
	#   → 核心 Xlib, dwm.c 链接 -lX11
	pkg libxinerama
	#   → 多显示器支持, config.mk -DXINERAMA -lXinerama
	pkg libxft
	#   → FreeType 字体渲染, drw.c 链接 -lXft
	pkg libxrender
	#   → X Render 扩展 (alpha/透明/圆角), drw.c 链接 -lXrender

	# ── 图像 ──
	pkg imlib2
	#   → 窗口图标加载, drw.c/dwm.c 链接 -lImlib2

	# ── 字体 ──
	pkg fontconfig
	#   → 字体发现与匹配, drw.c 链接 -lfontconfig
	pkg freetype2
	#   → 字体光栅化, libxft 底层依赖
}

# ═══════════════════════════════════════════════════════════
# Phase 2: X11 Server + 图形工具
# ═══════════════════════════════════════════════════════════
phase2() {
	phase "Phase 2: X11 与图形工具"

	# ── X Server ──
	pkg xorg-server xorg-xinit
	#   → X11 显示服务 + startx

	# ── 显示器管理 ──
	pkg xorg-xrandr
	#   → ~/.dwm/tools/monitor-conf.sh
	#   → ~/.dwm/tools/wallpaper.sh (get_monitor_info)
	pkg autorandr
	#   → ~/.dwm/autostart.sh (显示器布局自动检测)

	# ── 键盘/输入 ──
	pkg xorg-setxkbmap xorg-xset
	#   → ~/.dwm/autostart.sh (Caps→Esc, 按键速率)
	#   → ~/.dwm/tools/keyboard.sh

	# ── 剪贴板/选区 ──
	pkg xclip
	#   → ~/.dwm/tools/screenshot.sh (截图到剪贴板)
	pkg slop
	#   → ~/.dwm/tools/screencast.sh (区域选择)

	# ── 自动化/锁屏辅助 ──
	pkg xdotool
	#   → ~/.dwm/tools/lock.sh (锁屏前操作)
	#   → ~/.dwm/tools/screenshot.sh (窗口截图)
	#   → ~/.dwm/tools/theme.sh (主题切换)
	pkg xautolock xprintidle
	#   → ~/.dwm/tools/screen.sh (定时锁屏 + 空闲检测)

	# ── Xresources ──
	pkg xorg-xrdb
	#   → ~/.Xresources (DWM 动态配色)
	#   → ~/.dwm/tools/theme.sh (light/dark 切换)
}

# ═══════════════════════════════════════════════════════════
# Phase 3: 字体
# ═══════════════════════════════════════════════════════════
phase3() {
	phase "Phase 3: 字体"

	pkg noto-fonts noto-fonts-cjk noto-fonts-emoji
	#   → 系统默认无衬线/衬线/等宽字体
	#   → 中文/日文/韩文 (CJK) 显示
	#   → GTK 应用 (→ .config/gtk-*/settings.ini)
	aur noto-fonts-cjk-fontconfig
	#   → CJK 字体 fontconfig 预设规则

	pkg nerd-fonts-complete
	#   → dwm 标签栏图标 (config.h Nerd Font icons)
	#   → waybar 图标 (→ .config/waybar/)
	#   → kitty/alacritty 终端字体 (→ .config/kitty/kitty.conf)
	#   → conky 系统监控 (→ .config/conky/)
}

# ═══════════════════════════════════════════════════════════
# Phase 4: Shell 环境
# ═══════════════════════════════════════════════════════════
phase4() {
	phase "Phase 4: Shell 环境"

	pkg zsh
	#   → 交互式 Shell (→ .zshrc)
	pkg starship
	#   → 跨 Shell 提示符 (→ .config/starship.toml)
}

# ═══════════════════════════════════════════════════════════
# Phase 5: 终端与 CLI 工具
# ═══════════════════════════════════════════════════════════
phase5() {
	phase "Phase 5: 终端与 CLI 工具"

	# ── 终端模拟器 ──
	pkg kitty
	#   → ~/.dwm/dwm-launcher.sh (默认终端)
	#   → ~/.dwm/dwm-statuscmd.sh (浮动终端)
	#   → .config/kitty/kitty.conf
	pkg alacritty
	#   → 备选终端 (→ .config/alacritty/alacritty.toml)

	# ── 终端复用器 ──
	pkg tmux
	#   → 会话管理 (→ .config/tmux/tmux.conf)

	# ── JSON/HTTP/文件传输 ──
	pkg jq
	#   → ~/.dwm/tools/wallpaper.sh (配置解析)
	#   → ~/.dwm/tools/screencast.sh (音频设备检测)
	#   → ~/.dwm/utils/dwm-status-tools.sh (天气/邮件 API)
	pkg curl wget rsync
	#   → ~/.dwm/tools/update-ruleset.sh (规则集下载)
	#   → ~/.dwm/utils/dwm-status-tools.sh (天气/邮件 API)
	#   → dotfile Makefile (rsync 配置同步)

	# ── 现代 CLI 替换 (→ .aliases) ──
	pkg ripgrep fd fzf bat eza zoxide
	#   → rg  (替代 grep)、fd  (替代 find)
	#   → fzf (模糊搜索)、bat (替代 cat)
	#   → eza (替代 ls)、 zoxide (替代 cd)
	pkg git-delta
	#   → git diff 语法高亮 (→ .gitconfig)
}

# ═══════════════════════════════════════════════════════════
# Phase 6: 桌面组件
# ═══════════════════════════════════════════════════════════
phase6() {
	phase "Phase 6: 桌面组件"

	# ── 合成器 ──
	pkg picom
	#   → ~/.dwm/autostart.sh (窗口透明/模糊/圆角)
	#   → .config/dwm/picom.conf

	# ── 通知 ──
	pkg dunst libnotify
	#   → ~/.dwm/autostart.sh (通知守护进程)
	#   → ~/.dwm/tools/brightness.sh (notify-send)
	#   → ~/.dwm/tools/volume.sh
	#   → .config/dunst/dunstrc

	# ── 启动器/菜单 ──
	pkg rofi rofi-calc
	#   → ~/.dwm/dwm-launcher.sh (应用启动/电源菜单/模块)
	#   → ~/.dwm/rofi/ 全部脚本
	#   → rofi-calc: 计算器插件
	aur rofi-emoji
	#   → ~/.dwm/rofi/scripts/emoji.sh (emoji 选择器)

	# ── 锁屏 ──
	aur i3lock-color
	#   → ~/.dwm/tools/lock.sh (美化锁屏)

	# ── 硬件控制 ──
	pkg brightnessctl
	#   → ~/.dwm/tools/brightness.sh (屏幕亮度)
	#   → ~/.dwm/dwm-status-print.sh (亮度状态)

	# ── 音频 ──
	pkg pipewire wireplumber pipewire-audio pipewire-pulse pipewire-alsa
	#   → 系统音频服务 (替代 PulseAudio)
	#   → .config/pipewire/  .config/wireplumber/
	pkg pavucontrol
	#   → GUI 音量控制
	pkg alsa-utils
	#   → ~/.dwm/tools/volume.sh (amixer 音量控制)
	#   → ~/.dwm/dwm-status-print.sh (音量状态)

	# ── 系统服务 ──
	pkg networkmanager network-manager-applet
	#   → ~/.dwm/autostart.sh (nm-applet 托盘)
	#   → 网络管理
	pkg bluez bluez-utils
	#   → 蓝牙支持
	pkg lxqt-policykit
	#   → ~/.dwm/autostart.sh (鉴权代理)
	pkg udiskie
	#   → ~/.dwm/autostart.sh (USB 自动挂载)
}

# ═══════════════════════════════════════════════════════════
# Phase 7: 应用与工具
# ═══════════════════════════════════════════════════════════
phase7() {
	phase "Phase 7: 应用与工具"

	# ── 音频/音乐 ──────────────────────────────────────
	pkg mpd mpc
	#   → MPD 音乐播放守护进程 + 命令行控制
	#   → ~/.dwm/dwm-status-tools.sh (MPD 状态采集)
	#   → ~/.dwm/rofi/scripts/mpd.sh
	#   → .config/mpd/mpd.conf
	aur rmpc
	#   → MPD TUI 客户端 (→ .config/rmpc/config.ron)
	aur ncpamixer
	#   → ~/.dwm/dwm-statuscmd.sh (音量混音器 TUI)
	aur easyeffects
	#   → 音频效果器 (EQ/混响/...)
	#   → .local/share/easyeffects/

	# ── 媒体/壁纸 ──────────────────────────────────────
	pkg feh mpv ffmpeg yt-dlp
	#   → feh:   ~/.dwm/tools/wallpaper.sh (图片壁纸)
	#   → mpv:   ~/.dwm/tools/wallpaper.sh (视频壁纸)
	#   → ffmpeg:~/.dwm/tools/screencast.sh (屏幕录制)
	#   → ffprobe: ~/.dwm/tools/wallpaper-render.sh
	#   → yt-dlp: ~/.dwm/tools/youtube/yt.sh
	aur xwinwrap-git
	#   → ~/.dwm/tools/wallpaper-render.sh
	#     (视频/网页壁纸桌面渲染层)
	aur archlinux-wallpaper
	#   → ~/.dwm/tools/lock.sh (锁屏壁纸源)

	# ── 截图 ────────────────────────────────────────────
	pkg flameshot maim
	#   → flameshot: GUI 截图 (~/.dwm/tools/screenshot.sh)
	#   → maim:      CLI 窗口截图
	pkg nsxiv
	#   → ~/.dwm/tools/screenshot.sh (截图预览)

	# ── 输入法 ──────────────────────────────────────────
	pkg fcitx5 fcitx5-chinese-addons fcitx5-configtool
	#   → ~/.dwm/autostart.sh (fcitx5 启动)
	#   → .config/fcitx5/ (配置)
	#   → 中文输入 (拼音)
	aur fcitx5-rime
	#   → Rime 输入引擎

	# ── 系统监控 ────────────────────────────────────────
	pkg conky
	#   → ~/.dwm/autostart.sh (系统监控小部件)
	#   → .config/conky/
	pkg btop
	#   → ~/.dwm/dwm-statuscmd.sh (CPU 详情 → btop)

	# ── 电池/电量 ───────────────────────────────────────
	pkg acpi
	#   → ~/.dwm/dwm-status-print.sh (电池状态)

	# ── 文件管理 ────────────────────────────────────────
	pkg yazi
	#   → 终端文件管理器 (→ .config/yazi/)
	pkg pcmanfm
	#   → GUI 文件管理器 (→ .config/pcmanfm/)

	# ── 终端应用 ────────────────────────────────────────
	pkg newsboat
	#   → RSS 阅读器 (→ .config/newsboat/)
	#   → ~/.dwm/dwm-status-tools.sh (RSS 未读数)
	pkg aerc
	#   → 终端邮件客户端 (→ .config/aerc/)
	pkg zathura zathura-pdf-mupdf
	#   → PDF 阅读器 (→ .config/zathura/zathurarc)
	pkg speedtest-cli
	#   → ~/.dwm/dwm-statuscmd.sh (网速测试)

	# ── 开发工具 ────────────────────────────────────────
	pkg github-cli lazygit
	#   → gh:      GitHub CLI (→ .config/gh/)
	#   → lazygit: Git TUI (→ .config/lazygit/)

	# ── 主题/外观 (GTK + Qt) ────────────────────────────
	aur orchis-theme-git
	#   → GTK 主题 Orchis-Dark
	#   → .config/gtk-*/settings.ini
	aur tela-icon-theme-git
	#   → 图标主题 Tela-manjaro-dark
	aur bibata-cursor-theme-bin
	#   → 光标主题 Bibata-Modern-Ice
	#   → .Xresources (Xcursor.theme)
	pkg kvantum qt6ct
	#   → kvantum: Qt 主题引擎
	#   → .config/Kvantum/  .config/qt6ct/
	aur kvantum-theme-orchis-git
	#   → Kvantum Orchis 主题 (Qt 应用统一外观)

	# ── 按键显示 ────────────────────────────────────────
	pkg screenkey
	#   → 屏幕按键回显 (→ .config/screenkey.json)

	# ── 显示管理器 (可选) ────────────────────────────────
	pkg sddm
	#   → 登录管理器
	#   → ~/.dwm/tools/sddm.sh (主题管理)
}

# ═══════════════════════════════════════════════════════════
# Phase 8: 编译安装 dwm
# ═══════════════════════════════════════════════════════════
phase8() {
	phase "Phase 8: 编译安装 dwm"

	local dwm_src
	if [ -f "config.mk" ] && [ -f "dwm.c" ] && [ -f "config.h" ]; then
		# 从 dwm 源码目录执行 install.sh，直接使用本地源码
		dwm_src="$PWD"
		say "使用当前目录 dwm 源码: $dwm_src"
	else
		clone_or_pull "$DWM_REPO" "$WORKDIR/dwm"
		dwm_src="$WORKDIR/dwm"
	fi

	if $DRY; then
		echo "  [DRY] make -C $dwm_src clean install"
	else
		say "编译 dwm..."
		make -C "$dwm_src" clean install
		say "dwm 安装完成 → $(command -v dwm)"
	fi
}

# ═══════════════════════════════════════════════════════════
# Phase 9: 部署配置
# ═══════════════════════════════════════════════════════════
phase9() {
	phase "Phase 9: 部署配置"

	# ── dwm 脚本 (~/.dwm/) ──
	clone_or_pull "$SCRIPTS_REPO" "$HOME/.dwm"

	# ── dotfile 配置 ──
	clone_or_pull "$DOTFILE_REPO" "$WORKDIR/dotfile"

	if $DRY; then
		echo "  [DRY] make -C $WORKDIR/dotfile install"
		echo "  [DRY] zimfw install"
	else
		say "部署 dotfile 配置 (rsync → \$HOME)..."
		make -C "$WORKDIR/dotfile" install

		# Zim 插件
		if command -v zimfw &>/dev/null; then
			say "安装/更新 Zim 插件..."
			zimfw install
		fi

		# Shell 切换
		if [ "$SHELL" != "$(command -v zsh)" ]; then
			say "设置默认 Shell → zsh"
			chsh -s "$(command -v zsh)"
		fi
	fi
}

# ═══════════════════════════════════════════════════════════
# Phase 10: 验证
# ═══════════════════════════════════════════════════════════
phase10() {
	phase "Phase 10: 验证"

	$DRY && {
		echo "  [DRY] 跳过验证"
		return
	}

	local ok=0 fail=0

	check() {
		if command -v "$1" &>/dev/null; then
			printf "  ${G}✓${N} %s\n" "$1"
			((ok++)) || true
		else
			printf "  ${R}✗${N} %-30s %s\n" "$1" "${2:-}"
			((fail++)) || true
		fi
	}

	check dwm "dwm 未安装 → Phase 8 可能失败"
	check kitty "→ ~/.dwm/dwm-launcher.sh"
	check rofi "→ ~/.dwm/dwm-launcher.sh"
	check picom "→ ~/.dwm/autostart.sh"
	check dunst "→ ~/.dwm/autostart.sh"
	check paru "→ AUR helper"
	check jq "→ 被多个脚本依赖"
	check feh "→ ~/.dwm/tools/wallpaper.sh"
	check mpv "→ ~/.dwm/tools/wallpaper.sh"
	check ffmpeg "→ ~/.dwm/tools/screencast.sh"
	check fcitx5 "→ ~/.dwm/autostart.sh"
	check zsh "→ Shell"
	check starship "→ Shell 提示符"
	check tmux "→ 终端复用器"

	echo ""
	echo "  通过: $ok   失败: $fail"
	[ "$fail" -eq 0 ] || warn "有 $fail 项未通过，请查看上方 ✗ 标记"
}

# ═══════════════════════════════════════════════════════════
# 主流程
# ═══════════════════════════════════════════════════════════
main() {
	echo -e "${B}"
	echo "  ╔══════════════════════════════════╗"
	echo "  ║   walter-arch-bootstrap         ║"
	echo "  ║   裸 Arch → 完整 dwm 桌面环境    ║"
	echo "  ╚══════════════════════════════════╝"
	echo -e "${N}"

	phase0 # 前置检查 + 安装 paru/dialog
	select_phases

	local phases_arr=($PHASES)
	for p in "${phases_arr[@]}"; do
		"phase${p}"
	done

	echo ""
	echo -e "${G}══════════════════════════════════════${N}"
	echo -e "${G}  完成!${N}"
	echo ""
	echo "  下一步:"
	echo "    • 确认 ~/.dwm/autostart.sh 中的启动项"
	echo "    • startx 启动 dwm"
	echo "    • 或配置 SDDM: systemctl enable sddm"
	echo ""
}

main
