#!/bin/bash

EOD_LINE=$(cat /data/local2/haya/fts/cloko001.txt | grep -A 1000 $1 | grep -n EOD | head -1 | sed -e 's/:.*//g')
cat /data/local2/haya/fts/cloko001.txt | grep -A 1000 $1 | head -${EOD_LINE}

