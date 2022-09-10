#!/bin/sh

result=0

t=$(mktemp)
o=$(mktemp)
for rom in "$@"
do
	base=$(basename $rom .rom)

	if ! hackem -s0 -t $t $rom >$o
	then
		echo "ERROR: $rom (exit $?)" >/dev/stderr
		result=$((result | 8))
	fi
	if ! diff -u $base.tsv $t
	then
		echo "FAIL: $rom (bad trace)" >/dev/stderr
		result=$((result | 1))
	elif [ -f $base.out ] && ! diff -u $base.out $o
	then
		echo "FAIL: $rom (bad output)" >/dev/stderr
		result=$((result | 2))
	else
		echo ok: $rom
	fi
done
rm -f $t

exit $result
