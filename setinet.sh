#!/bin/bash

# exit if no argument is given
if [ -z "$1" ] || [ -z "$2" ]
  then
    echo "Arguments not supplied"
    exit 1
fi

# set INET_ROOT env variable in ~/.*rc
if grep -q "INET_ROOT" ~/.$2rc
then
    sed -in "s#INET_ROOT=.*#INET_ROOT=$1#g" ~/.$2rc
else
    echo "export INET_ROOT=$1" >> ~/.$2rc
fi

