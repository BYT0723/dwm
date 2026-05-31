#!/bin/bash
#
# dwm 一键安装脚本
# 用途: 在新安装的 Arch Linux 上配置完整的 dwm 桌面环境
# 用法: ./install.sh（以普通用户运行，需要 sudo 权限）
#
set -euo pipefail

# ============================================================
# 配置
# ============================================================
CACHE_DIR="/tmp/walter-dwm-install"
DOTFILE_REPO="https://github.com/BYT0723/dotfile.git"
DWM_REPO="https://github.com/BYT0723/dwm.git"
SCRIPTS_REPO="https://github.com/BYT0723/scripts.git"
CN_MIRROR="https://repo.archlinuxcn.org/\$arch"

# ============================================================
# 工具函数
# ============================================================
info() { echo -e "\033[32m========== $* ==========\033[0m"; }
step() { echo -e "\033[32m----- $*\033[0m"; }
warn() { echo -e "\033[33mWARNING: $*\033[0m"; }
die() {
	echo -e "\033[31mERROR: $*\033[0m" >&2
	exit 1
}
need() { command -v "$1" &>/dev/null || die "请先安装: $1"; }

clone_or_pull() {
	local repo="$1" dest="$2"
	if [ -d "$dest/.git" ]; then
		git -C "$dest" pull --ff-only
	else
		git clone "$repo" "$dest"
	fi
}

# ============================================================
# 前置检查
# ============================================================
[ "$(id -u)" -ne 0 ] || die "请以普通用户运行，需要提权时脚本会自动调用 sudo"
need sudo
need git
need curl
command -v pacman &>/dev/null || die "此脚本仅支持 Arch Linux"

sudo -v # 缓存 sudo 凭证，避免中途输密码

mkdir -p "$CACHE_DIR"

# ============================================================
# Phase 1: 系统组件 — 字体与基础库
# ============================================================
info "Phase 1: 系统组件"

step "启用 archlinuxcn 源"
if ! grep -q '\[archlinuxcn\]' /etc/pacman.conf; then
	echo -e "\n[archlinuxcn]\nServer = $CN_MIRROR" | sudo tee -a /etc/pacman.conf >/dev/null
fi
sudo pacman -Sy --noconfirm archlinuxcn-keyring
sudo pacman -S --noconfirm archlinuxcn-mirrorlist-git 2>/dev/null || warn "archlinuxcn-mirrorlist-git 未安装（可能需要手动配置镜像）"

step "安装字体"
sudo pacman -S --noconfirm \
	noto-fonts noto-fonts-cjk noto-fonts-emoji \
	noto-fonts-cjk-fontconfig \
	nerd-fonts-complete

# ============================================================
# Phase 2: Shell 环境
# ============================================================
info "Phase 2: Shell 环境"

step "安装 zsh 和 starship"
sudo pacman -S --noconfirm zsh starship

step "安装 Zimfw"
curl -fsSL https://raw.githubusercontent.com/zimfw/install/master/install.zsh | zsh

step "部署 dotfiles"
clone_or_pull "$DOTFILE_REPO" "$CACHE_DIR/dotfile"
cp -r "$CACHE_DIR/dotfile/"* "$HOME/"

step "更新 zim 插件"
zimfw update

# ============================================================
# Phase 3: 窗口管理器
# ============================================================
info "Phase 3: dwm 窗口管理器"

step "编译安装 dwm"
clone_or_pull "$DWM_REPO" "$CACHE_DIR/dwm"
make -C "$CACHE_DIR/dwm" clean install

# ============================================================
# Phase 4: 脚本与工具
# ============================================================
info "Phase 4: dwm 脚本"

step "克隆脚本仓库到 ~/.dwm"
clone_or_pull "$SCRIPTS_REPO" "$HOME/.dwm"

# ============================================================
# Phase 5: 应用组件
# ============================================================
info "Phase 5: 应用组件"

step "蓝牙"
sudo pacman -S --noconfirm bluez bluez-utils

step "网络管理"
sudo pacman -S --noconfirm networkmanager network-manager-applet

step "桌面通知"
sudo pacman -S --noconfirm libnotify dunst

step "鉴权"
sudo pacman -S --noconfirm lxqt-policykit

step "输入法"
sudo pacman -S --noconfirm fcitx5-im fcitx5-chinese-addons
if ! grep -q "GTK_IM_MODULE=fcitx" /etc/environment 2>/dev/null; then
	sudo tee -a /etc/environment >/dev/null <<'EOF'
GTK_IM_MODULE=fcitx
QT_IM_MODULE=fcitx
XMODIFIERS=@im=fcitx
SDL_IM_MODULE=fcitx
GLFW_IM_MODULE=ibus
EOF
fi

step "壁纸"
sudo pacman -S --noconfirm feh xwinwrap-git mpv archlinux-wallpaper

# ============================================================
# Phase 6: 可选组件
# ============================================================
info "Phase 6: 可选组件"

step "浏览器"
sudo pacman -S --noconfirm firefox surf

step "代理"
sudo pacman -S --noconfirm trojan
if [ -f "$HOME/.dwm/configs/trojan.json" ]; then
	sudo cp "$HOME/.dwm/configs/trojan.json" /etc/trojan/config.json
	sudo systemctl enable --now trojan
else
	warn "trojan 配置未找到，跳过代理服务设置"
fi

step "工具"
sudo pacman -S --noconfirm yazi xclip
ya pack -i
ya pack -u

step "音乐"
sudo pacman -S --noconfirm mpd mpc rmpc

# ============================================================
info "安装完成！请重启或手动 startx 启动 dwm"
# ============================================================
