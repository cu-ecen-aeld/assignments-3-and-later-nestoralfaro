#!/bin/bash
if [ $# -eq 0 ]
then
	echo "ERROR: Please provide the arguments in order: 1) Directory path 2) text to search."
	exit 1
fi

if [[ ! -d $1 ]]
then
	echo "ERROR: \"$1\" is not a valid directory."
	exit 1
fi

if [[ -z $2 ]]
then
	echo "ERROR: Please provide the text to search."
	exit 1
fi

fileCount=0
matchCount=0

# https://stackoverflow.com/questions/16317961/how-to-process-each-output-line-in-a-loop
while read -r line ; do
	((++fileCount))
	# https://stackoverflow.com/questions/428109/extract-substring-in-bash
	((matchCount=matchCount+${line#*:}))
done < <(grep -rc "$2" $1)

echo "The number of files are $fileCount and the number of matching lines are $matchCount"
