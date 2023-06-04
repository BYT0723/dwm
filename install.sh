#!/bin/bash

#
# 本脚本未对xorg进行配置
# 你需要自行安装你的xorg和驱动程序，并配置完善
# 否则可能会出现黑屏，键鼠失效等情况
#
#
# 脚本中大量使用了Github的资源，建议使用代理运行
# 无代理可使用gitee.com替换基本中的github.com
# 但zimfw的安装可能会失败

CACHE_DIR=$HOME"/.cache/walter-dwm"

if ![ -d "$CACHE_DIR" ]; then
    mkdir $CACHE_DIR
fi

title() {
    echo -e "\033[32m----------"$*"----------\033[0m"
}

subtitle() {
    echo -e "\033[32m-----"$*"\033[0m"
}

title "安装字体"

#
# 不建议使用extra源中的nerdfont字体，没有complelte导致部分symbols无法显示
# 若连接archlinuxcn源困难，可选择extra中的字体包
# pacman -S ttf-cascadia-code-nerd ttf-jetbrains-mono-nerd ttf-iosevka-nerd wqy-zenhei wqy-microhei

echo "[archlinuxcn]
Server = https://repo.archlinuxcn.org/$arch" >>/etc/pacman.conf
pacman -S archlinuxcn-keyring archlinuxcn-mirrorlist-git
# 建议手动修改/etc/pacman.d/archlinuxcn-mirrorlist, 选择距离更近速度更快的源
pacman -S nerd-fonts-complete ttf-lxgw-wenkai ttf-lxgw-wenkai-mono ttf-yozai-font ttf-myuppy-gb

title "初始化zsh环境"

title "安装zsh和starship"
pacman -S zsh starship

title "zimfw"
curl -fsSL https://raw.githubusercontent.com/zimfw/install/master/install.zsh | zsh

title "克隆dotfile仓库"
git clone https://github.com/BYT0723/dotfile.git $CACHE_DIR"/dotfile"

title "复制配置文件"
cp $CACHE_DIR"/dotfile/*" ~/
title "更新zim以及插件"
zimfw update

title "安装dwm"
git clone https://github.com/BYT0723/dwm.git $CACHE_DIR"/dwm"
cd $CACHE_DIR"/dwm" && make clean install
title "将 dwm 写入用户的xinitrc"
if ! [ -f ~/.xinitrc ]; then
    touch ~/.xinitrc
fi
echo "exec dwm >> .dwm.log" >>~/.xinitrc

title "安装st"
git clone https://github.com/BYT0723/dwm.git $CACHE_DIR"/st"
cd $CACHE_DIR"/st" && make clean install

title "安装shell脚本文件"
git clone https://github.com/BYT0723/scripts ~/.dwm

title "安装脚本所需组件"
pacman -S rofi picom-git alsa-utils light acpi

subtitle "蓝牙"
pacman -S bluez bluez-utils

subtitle "网络"
pacman -S networkmanager network-manager-applet

subtitle "通知"
pacman -S libnotify dunst

subtitle "鉴权"
pacman -S lxqt-policykit

subtitle "输入法"
pacman -S fcitx5-im fcitx5-chinese-addons
# 配置输入法环境变量
echo "GTK_IM_MODULE=fcitx
QT_IM_MODULE=fcitx
XMODIFIERS=@im=fcitx
SDL_IM_MODULE=fcitx
GLFW_IM_MODULE=ibus" >>/etc/environment

subtitle "壁纸"
pacman -S feh xwinwrap-git mpv archlinux-wallpaper

#---------------------------------------------------------------------------#

title "其他组件"

subtitle "浏览器"
pacman -S firefox surf

subtitle "代理"
pacman -S trojan
cp ~/.dwm/configs/trojan.json /etc/trojan/config.json
systemctl start trojan
systemctl enable trojan

subtitle "工具"
pacman -S ranger xclip

subtitle "音乐"
pacman -S mpd mpc ncmpcpp
