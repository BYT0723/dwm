# Dwm

## Preview

![preview](https://i.imgur.com/WZxCz6j.jpg)

> Tips: 配合 Dwm 运行的大量 shell 被存储在[scripts](https://github.com/BYT0723/scripts)中，建议完整阅读文档并安装依赖，在安装本库 Dwm 配合使用

## Install

```shell
git clone https://github.com/BYT0723/dwm.git

cd dwm
chmod +x install.sh

./install.sh

```

## Required

- picom
- xautolock
- nm-applet
- fcitx5
- udiskie
- imlib (for status bar tab icon)

  Archlinux:

  ```shell
  sudo pacman -S imlib2
  ```

  Debian:

  ```shell
  sudo apt install libimlib2-dev
  ```

## Tests

```shell
make test    # unit tests for the status block/pill and tab-cell geometry (no X needed)
make smoke   # end-to-end bar test on a private Xvfb display
```

`make smoke` additionally needs `Xvfb`, `xterm`, `xdotool`, `xwd` and
ImageMagick (`convert`/`compare`). It starts dwm on a throwaway `HOME` and
screenshots the bar, so run it on a machine where starting a headless X server
is acceptable.
