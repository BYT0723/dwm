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
  else
    echo "Warning: To use a customized terminal, you need to modify the term.sh before start dwm"
  fi
fi

# 安装rofi
if ! [[ -n $(command -v rofi) ]]; then
  read -p "install rofi ? (y/n): " flag
  if [[ $flag == "y" ]]; then
    pacman -S rofi
  fi
fi
