#!/bin/sh

files_dir=directory
search_str=string
num_of_files=0
num_of_lines=0

if [ $# -eq 2 ]
then
    files_dir=$1
    search_str="$2"
    if [ ! -d "$files_dir" ]
    then
        echo "Error: non existent directory"
        exit 1
    else
        num_of_files=$(grep -lr "${search_str}" "${files_dir}" | wc -l)
        num_of_lines=$(grep -r "${search_str}" "${files_dir}" | wc -l)
        echo "The number of files are $num_of_files and the number of matching lines are $num_of_lines"
        exit 0
    fi    
else
    echo "Error: arguments missing."
    echo "Required arguments for finder script:"
    echo "1. directory path"
    echo "2. string to search for"
	exit 1
fi