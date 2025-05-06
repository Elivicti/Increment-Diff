#!/bin/bash

path="$1"


function getsha1()
{
	for file in $1/*
	do
		if [ -d $file ]; then
			getsha1 $file
		else
			sha1sum $file
		fi
	done
}

getsha1 $path