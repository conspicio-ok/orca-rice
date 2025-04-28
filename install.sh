#!/bin/bash

function print_section {
	col=$((80));
	spaces=$((0));
	col=$(($col-$spaces*4-${#1}));
	string='';

	for ((i=0; i<$spaces; i++)) {
		string=$string' ';
	}
	for ((i=0; i<$(($col/2)); i++)) {
		string=$string'-';
	}
	string=$string$1$string

	echo $string;
}

cd;

orca='orca-rice';
if [ ! -d $orca ];
then
	echo "$orca is not found, please move it at your home !";
else
	print_section  "CREATE '.config' FOLDER";
	mkdir $HOME/.config 2> tmp.txt
	echo ;
	echo ;
	
	print_section "MOVE CURRENTS FILES IN THE .CONFIG FOLDER"
	mv $HOME/orca-rice/* $HOME/.config > tmp.txt
	mv $HOME/orca-rice/.* $HOME/.config > tmp.txt
	echo ;
	echo ;
	
	print_section "PACKAGES INSTALL";
	bash $HOME/.config/dotfiles/scripts/install_packages.sh;
	echo ;
	echo ;

	print_section "CREATE BASHRC";
	if [ -f $HOME/.bashrc ];
		then	rm $HOME/.bashrc;
	fi
	ln -s $HOME/.config/bash/bashrc $HOME/.bashrc
	if [ -f /root/.bashrc ];
		then	rm /root/.bashrc;
	fi
	ln -s $HOME/.config/bash/bashrc /root/.bashrc
fi


