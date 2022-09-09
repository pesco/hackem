#!/bin/sh

result=0

t=$(mktemp)
for rom in "$@"
do
	if ! hackem -t $t $rom
	then
		echo ERROR: $rom
		result=$((result | 2))
	fi
	if ! diff -u $(basename $rom .rom).tsv $t
	then
		echo FAIL: $rom
		result=$((result | 1))
	else
		echo ok: $rom
	fi
done
rm -f $t

exit $result
