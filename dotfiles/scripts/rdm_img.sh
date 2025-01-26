#!/bin/bash

MAX=$(ls -l $HOME/Background/* | wc -l)
NUMBER=$(shuf -i 1-$MAX -n 1)
((NUMBER--))
WALL="wall${NUMBER}.png"
sleep 2
hyprctl hyprpaper preload "$HOME/Background/${WALL}"
hyprctl hyprpaper wallpaper ",$HOME/Background/${WALL}"
