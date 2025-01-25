#!/bin/bash

killall waybar

waybar -c $HOME/.config/waybar/config.jsonc & -s $HOME/.config/waybar/styles.css
