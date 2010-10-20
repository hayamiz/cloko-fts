#!/bin/bash

topdir=$(dirname $(readlink -f $0))
datafile=/data/local2/haya/fts/$(hostname -s).txt

grep_num=$(grep "^#" $datafile|wc -l)
fts_output=$(mktemp)
$topdir/fts-node --datafile $datafile --check-docnum > $fts_output

if grep $grep_num $fts_output > /dev/null; then
    echo "$(hostname -s): $grep_num documents parse ok."
else
    echo "$(hostname -s): $grep_num documents parse error."
    exit 1
fi