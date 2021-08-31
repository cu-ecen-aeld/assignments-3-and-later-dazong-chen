#!/bin/sh
# Script for finder.sh
# Author: Dazong Chen
#reference: 
# https://alligator.io/workflow/command-line-creating-files-directories/
# https://unix.stackexchange.com/questions/305844/how-to-create-a-file-and-parent-directories-in-one-command


# both input cannot be empty
# argument1 is directory path and argument 2 is string
if [ -z "$1" ] || [ -z "$2" ]
then
	echo "ERROR: Both input argument cannot be empty"
	exit 1
fi

# set input arguments to variables
writefile=$1
writestr=$2


#create new directory if it does not exist and write file

if [ ! -d "$writefile" ]
then
	mkdir -p "$(dirname "$writefile")" && touch "$writefile"
fi


# write string to file
echo "$writestr" > "$writefile"

