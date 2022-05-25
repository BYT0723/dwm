#!/bin/bash
cols=80     #width
rows=25     #height
x=+550
y=+150

# add window at right-top
st -i -g ${cols}x${rows}${x}${y} /usr/bin/zsh
