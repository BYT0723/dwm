DIR=$(pwd)

# 编译dwm
make clean install

# 创建脚本软链接
if [[ -d ~/.dwm ]]; then
    mv ~/.dwm ~/.dwm-old
fi
ln -s ${DIR}/scripts/ ~/.dwm

# 安装st
if ! [[ -n $(command -v st) ]]; then
    read -p "install st ? (y/n): " flag
    if [[ $flag == "y" ]]; then
        git clone https://github.com/BYT0723/st.git
        cd st
        make clean install
        cd ..
        rm -rf st
    else
        echo "Warning: To use a customized terminal, you need to modify the term.sh before start dwm"
    fi
fi

# 安装Alacritty
if ! [[ -n $(command -v alacritty) ]]; then
    read -p "install alacritty ? (y/n): " flag
    if [[ $flag == "y" ]]; then
        sudo pacman -S alacritty
    else
        echo "Warning: To use a customized terminal, you need to modify the term.sh before start dwm"
    fi
fi

# clone configuration
git clone https://github.com/BYT0723/dotfile.git
mv -r ./dotfile/.config/* ~/.config/

# 安装rofi
if ! [[ -n $(command -v rofi) ]]; then
    pacman -S rofi
fi

# 安装picom-animation
# 如果非animations版本，将无法启用动画效果
yay -S picom-animations-git

pacman -S mate-power-manager network-manager-applet volumeicon udiskie fcitx5-im privoxy trojan

# theme
# https://github.com/vinceliuice/WhiteSur-gtk-theme

# proxy
cp ./scripts/configs/trojan.json /etc/trojan/config.json
systemctl start trojan
systemctl enable trojan

cp ./scripts/configs/pac.action /etc/privoxy/pac.action
echo "actionsfile pac.action" >>/etc/privoxy/config
systemctl start privoxy
systemctl enable privoxy
