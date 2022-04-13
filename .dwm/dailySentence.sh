#!/bin/bash

sentence=$(fortune)
transTxt=$(echo $sentence | trans en:zh -b)
echo $sentence
echo $transTxt
read
