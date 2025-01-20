#!/bin/bash

mkdir ~/.config 2> tmp.txt
mv .fonts ~/.fonts 2> tmp.txt
mv ./* ~/.config 2> tmp.txt
mv ./.* ~/.config 2> tmp.txt

