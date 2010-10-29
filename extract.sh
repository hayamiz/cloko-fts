#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: $0 FILE URL"
    exit 1
fi

datafile=$1
url=$2

EOD_LINE=$(cat $datafile | grep -A 1000 $url | grep -n EOD | head -1 | sed -e 's/:.*//g')
# echo "EOD_LINE = $EOD_LINE" > /dev/stderr
cat $datafile | grep -A 1000 $url | head -${EOD_LINE}

