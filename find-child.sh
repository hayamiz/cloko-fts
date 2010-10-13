#!/bin/bash

INFO_FILE=$(dirname $0)/connect-info.txt
MYID=$(hostname -s|sed -e 's/[a-z]//g')

if grep ^$MYID $INFO_FILE > /dev/null; then
    grep ^$MYID $INFO_FILE | sed -e 's/^[0-9]* //g'
else
    echo none
fi
