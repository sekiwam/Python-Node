#!/bin/sh

git submodule update --remote

cd nodejs;
git reset --hard 
git clean -f -d
