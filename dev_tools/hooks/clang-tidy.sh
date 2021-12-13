#! /usr/bin/bash

# This is an approximation of running clang-tidy through Cmake, without having to
# go through the 20+ minutes to build the entire tree.
#
# The "weird" stuff in the processing of this is to get around header files not
# being found.
TEMP_FILE=$(mktemp /tmp/clang-tidy.XXXXXXXXX)

find "$GAIA_REPO/production" -name '*.h' -o -name '*.cpp' | \
    grep -v tests | \
    xargs clang-tidy | \
    grep -v 'file not found' | \
    grep -v '#include' > "$TEMP_FILE"
cat "$TEMP_FILE"

if grep -q "error:" "$TEMP_FILE"; then
  echo "One or more clang-tidy errors occurred when scanning the project."
  exit 1
elif grep -q "error:" "$TEMP_FILE"; then
  echo "One or more clang-tidy warnings occurred when scanning the project."
  exit 2
else
  echo "No clang-tidy errors or warnings occurred when scanning the project."
fi
exit 0
