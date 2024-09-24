#!/bin/sh

dir=directory_path
path=file_path
str=input_string

if [ $# -eq 2 ]
then
    path=$1
    dir=${path%/*}
    str=$2

    if [ ! -d "$dir" ]
    then
        mkdir -p "$dir"
    fi    
        touch "${path}"
        echo "$str" > "${path}"
        exit 0
else
    echo "Error: arguments missing."
    echo "Required arguments for writer script:"
    echo "1. file with full path"
    echo "2. string to write"
	exit 1
fi