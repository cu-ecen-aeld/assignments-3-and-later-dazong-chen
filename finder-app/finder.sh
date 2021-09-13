#!/bin/sh
# Script for finder.sh
# Author: Dazong Chen
# Reference: 
# 1. check command input: https://stackoverflow.com/questions/6482377/check-existence-of-input-argument-in-a-bash-shell-script
# 2. find directory: https://ryanstutorials.net/bash-scripting-tutorial/bash-if-statements.php


# both input cannot be empty
# argument1 is directory path and argument 2 is string
if [ -z "$1" ] || [ -z "$2" ]
then
	echo "ERROR: Both input argument cannot be empty"
	exit 1
fi

# set input arguments to variables
filesdir=$1
searchstr=$2


# check if the directory is found
if [ ! -d "$filesdir" ]
then	echo "Directory $filesdir NOT exist"
	exit 1
fi


# number of files in the directory and all subdirectories contains the string and the number of matching lines found in respective files.
cd $filesdir
num_files="$(grep -lr "$searchstr" *.* | wc -l)"
#echo "Number of files containing $searchstr : $num_files"
#echo $num_files
num_lines="$(grep -r "$searchstr" *.* | wc -l)"
#echo "Number of lines containing $searchstr : $num_lines"
#echo $num_lines

MATCHSTR="The number of files are ${num_files} and the number of matching lines are ${num_lines}"

echo $MATCHSTR
# | wc -l


