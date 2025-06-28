#!/bin/bash

sudo pacman -Syuu --noconfirm
$HOME/.config/dotfiles/scripts/install_packages.sh
cd $HOME/.config
git pull
