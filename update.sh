#!/bin/bash

echo '';
echo "#####################system update####################";
sudo pacman -Syu;
cd ~/.config;
echo '';
echo "###################orca-rice update###################";
git pull;
echo '';

