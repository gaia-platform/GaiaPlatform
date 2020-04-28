#!/bin/bash
input=$1
output=$2
# treat the rest of the arguments as include directories
include_args="${@:3}"
for path in $include_args
do
  includes="$includes -I $path"
done
./rule_subscriber $input -output=$output -- $includes
