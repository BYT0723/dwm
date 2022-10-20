#!/bin/bash

# 查看cache中超过180天未访问的文件
find ~/.cache/ -depth -type f -atime +180

# 删除cache中超过180天未访问的文件
find ~/.cache/ -type f -atime +365 -delete
