#!/bin/bash

sudo pacman -Syuu
./install_packages.sh
cd $HOME/.config
git pull
