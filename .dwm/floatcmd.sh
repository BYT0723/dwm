#!/bin/bash
cols=80     #width
rows=25     #height
font="Source Code Pro:style=Medium Italic:size=14"
x=+550
y=+150

# add window at right-top
st -i -g ${cols}x${rows}${x}${y} -f "${font}" /usr/bin/zsh
