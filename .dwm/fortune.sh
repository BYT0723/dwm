#!/bin/bash

fortune > ~/.fortune
cat < ~/.fortune | trans -b >> ~/.fortune
