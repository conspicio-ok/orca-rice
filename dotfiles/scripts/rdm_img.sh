#!/bin/bash

NUMBER=$(ls -l $HOME/Background | wc -l)
((NUMBER--))
WALL="wall${NUMBER}.png"
sleep 2
hyprctl hyprpaper preload="$HOME/Background/${WALL}"
hyprctl hyprpaper wallpaper="$HOME/Background/${WALL}"
