#!/bin/bash

mkdir ~/.config 2> tmp.txt
mv ./* ~/.config >> tmp.txt
mv ./.* ~/.config >> tmp.txt

