#!/bin/bash

config_file="$HOME/.config/hypr/modules/screen.conf"

if [ -f "$config_file" ];
then
	mkdir -p "$(dirname "$config_file")"

	while DESC= read -r desc;
	do
		echo "monitor = desc: $desc, preferred, auto, 1" >> "$config_file"
	done < <(hyprctl monitors all | grep "description" | awk '{for(i=2; i<NF; i++) printf "%s%s", $i, (i<NF-1 ? " " : ""); print ""}')

	echo "monitor = , preferred, auto, 1" >> ~/.config/hypr/modules/screen.conf
fi
