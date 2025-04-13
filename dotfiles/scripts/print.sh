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
		string=$string'=';
	}
	string=$string$1$string

	echo $string;
}

