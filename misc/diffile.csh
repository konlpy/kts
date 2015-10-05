#!/bin/csh -f
#
#
#
diff $1 $2 | ediff > $1.ddd
awk ' { print $1 } ' $1.ddd > $2.diff
/bin/rm $1.ddd


