#!/bin/sh

export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/.."

if test -z "$NO_MAKE"; then
    make -C $top_dir > /dev/null || exit 1
fi

if test -z "$CUTTER"; then
    CUTTER="`make -s -C $BASE_DIR echo-cutter`"
fi

CUTTER_ARGS=
CUTTER_WRAPPER=
if test x"$CUTTER_DEBUG" = x"yes"; then
    CUTTER_WRAPPER="$top_dir/libtool --mode=execute gdb --args"
    CUTTER_ARGS="--keep-opening-modules"
elif test x"$CUTTER_CHECK_LEAK" = x"yes"; then
    export G_SLICE=always-malloc
    export G_DEBUG=gc-friendly
    VALGRIND_LOG=$(mktemp)
    CUTTER_WRAPPER="$top_dir/libtool --mode=execute valgrind "
    CUTTER_WRAPPER="$CUTTER_WRAPPER --leak-check=full --show-reachable=yes --log-file=${VALGRIND_LOG} -v"
    CUTTER_ARGS="--keep-opening-modules"
fi

CUTTER_ARGS="${CUTTER_ARGS} -s $BASE_DIR"

LC_ALL=C
export LC_ALL
echo $CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BASE_DIR
$CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BASE_DIR

if test "$?" -eq "0" && test x"$CUTTER_CHECK_LEAK" = x"yes"; then
    PAGER=${PAGER}
    if test -z "${PAGER}"; then
	PAGER="less"
    fi
    $PAGER $VALGRIND_LOG
    rm $VALGRIND_LOG
fi
