#!/bin/bash

function getsha256()
{
	for file in $1/*
	do
		if [ -d $file ]; then
			getsha1 $file
		else
			sha256sum $file
		fi
	done
}

path="$1"
getsha256 $path