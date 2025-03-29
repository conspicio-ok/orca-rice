#!/bin/bash

sudo pacman -Syuu --noconfirm
./install_packages.sh
cd $HOME/.config
git pull
