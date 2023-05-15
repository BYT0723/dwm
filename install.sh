DIR=$(dirname $0)

# 编译dwm
make clean install

# 获取shell脚本
git clone https://github.com/BYT0723/scripts.git ~/.dwm

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
cd dotfile
make install
cd ..

# 安装rofi
if ! [[ -n $(command -v rofi) ]]; then
    pacman -S rofi
fi

# 安装picom-animation-git
# 如果非animations版本，将无法启用动画效果
pacman -S picom-git

pacman -S network-manager-applet udiskie fcitx5-im privoxy trojan

# theme
# git clone https://github.com/vinceliuice/WhiteSur-gtk-theme

# proxy
cp ./scripts/configs/trojan.json /etc/trojan/config.json
cp ./scripts/configs/pac.action /etc/privoxy/pac.action
echo "actionsfile pac.action" >>/etc/privoxy/config
# systemctl start trojan
# systemctl enable trojan
# systemctl start privoxy
# systemctl enable privoxy
