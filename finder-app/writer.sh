#!/bin/bash
if [ $# -eq 0 ]
then
	echo "ERROR: Please provide the arguments in order: 1) File path 2) text to write."
	exit 1
fi

if [[ -z $2 ]]
then
	echo "ERROR: Please provide the text to write."
	exit 1
fi

# https://stackoverflow.com/questions/9022383/bash-redirect-create-folder
mkdir -p "${1%/*}" && echo "$2" > "$1"

if [[ ! -e $1 ]]
then
	echo "ERROR: Unable to write \"$2\" to file \"$1\"."
	exit 1
fi
