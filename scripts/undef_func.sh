#!/bin/bash
# undef_func.sh -- print the undefined functions
# Usage:
#   $ make 2>typescript or 'script; make; exit'
#   $ undef_func.sh typescript
#
input=$1

[ -z "$input" -a ! -f typescript ] && echo "Usage: %0 input_file" && exit -1

[ -z "$input" -a -f typescript ] && input=typescript

grep undefined $input | cut -d"\`" -f2 | tr -d "'" | sort | uniq -c | sort -g
