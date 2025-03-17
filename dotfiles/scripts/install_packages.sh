#!/bin/bash
#if [ -n "$x" ]

# Packages list files
file="package.txt"

# Check if packages file exist
if [ ! -f "$file" ];	then
	echo "File doesn't found"
	exit 1
fi

# Lire le fichier ligne par ligne
while read -r ligne
do
	number=$(echo $ligne | awk '{print substr($0,1,1)}')
	package=$(echo $ligne | awk '{print substr($0,2)}')
	if [ $number -ne 0 ]; then
		if pacman -Qs $package > /dev/null;	then
  			echo "$package is already installed"
		else
			echo "$package is not installed"
			echo "Installation of $package..."
			sudo pacman -S $(package)
			echo "$package installed !"
			if [ $number -eq 2 ]; then
				echo "Activation of $package..."
				sudo systemctl enable $(package)
				echo "$package activated"
				echo "Start $package"
				sudo systemctl start $(package)
				echo "$package is started"
				echo "Please restart system for active the update !!!"
			fi
		fi
	fi
done < "$file"
