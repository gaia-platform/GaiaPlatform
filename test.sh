#! /usr/bin/bash

start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Testing the incubator..."
    fi
}

complete_process() {
    # $1 is the return code to assign to the script
    if [ "$1" -ne 0 ]; then
        echo "Testing of the incubator failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Testing of the incubator succeeded."
        fi
    fi
    if [ "$DID_PUSHD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi
    exit "$1"
}

show_usage() {
    echo "Usage: $(basename "$0") [flags]"
    echo "Flags:"
    echo "  -v,--verbose      Show lots of information while executing the project."
    echo "  -h,--help         Display this help text."
    echo "  -ni,--no-init     Do not initialize the test data before executing the test."
    exit 1
}

# Set up any script variables.
DID_PUSHD=0
TEST_DIRECTORY=/tmp/test_incubator
TEMP_FILE=/tmp/incubator.test.tmp

# Parse any command line values.
VERBOSE_MODE=0
VERY_VERBOSE_MODE=0
NO_INIT_MODE=0
PARAMS=""
while (( "$#" )); do
  case "$1" in
    -h|--help) # unsupported flags
      show_usage
      ;;
    -v|--verbose)
      VERBOSE_MODE=1
      shift
      ;;
    -vv|--very-verbose)
      VERBOSE_MODE=1
      VERY_VERBOSE_MODE=1
      shift
      ;;
    -ni|--no-init)
      NO_INIT_MODE=1
      shift
      ;;
    -*) # unsupported flags
      echo "Error: Unsupported flag $1" >&2
      show_usage
      ;;
    *) # preserve positional arguments
      PARAMS="$PARAMS $1"
      shift
      ;;
  esac
done
eval set -- "$PARAMS"

# Clean entrance into the script.
start_process

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Saving current directory prior to test execution."
fi
if ! pushd . >$TEMP_FILE 2>&1;  then
    cat $TEMP_FILE
    echo "Test script cannot save the current directory before proceeding."
    complete_process 1
fi
DID_PUSHD=1

if [ $NO_INIT_MODE -eq 0 ] ; then
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Removing test directory '$TEST_DIRECTORY' prior to test execution."
    fi
    if ! rm -rf $TEST_DIRECTORY >$TEMP_FILE 2>&1 ; then
        cat $TEMP_FILE
        echo "Test script cannot remove test directory '$TEST_DIRECTORY' prior to test execution."
        complete_process 1
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Installing project into test directory '$TEST_DIRECTORY'."
    fi
    if ! ./install.sh $TEST_DIRECTORY >$TEMP_FILE 2>&1 ; then
        cat $TEMP_FILE
        echo "Test script cannot install the project into directory '$TEST_DIRECTORY'."
        complete_process 1
    fi
    if ! cd $TEST_DIRECTORY; then
        echo "Test script cannot change to the test directory '$TEST_DIRECTORY'."
        complete_process 1
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Building project in test directory '$TEST_DIRECTORY'."
    fi
    if [ "$VERY_VERBOSE_MODE" -ne 0 ]; then
        DID_FAIL=0
        ./build.sh -v -f
        DID_FAIL=$?
    else
        DID_FAIL=0
        ./build.sh -v -f >$TEMP_FILE 2>&1
        DID_FAIL=$?
    fi
    if [ "$DID_FAIL" -ne 0 ]; then
        if [ "$VERY_VERBOSE_MODE" -eq 0 ]; then
            cat $TEMP_FILE
        fi
        echo "Test script cannot build the project in directory '$TEST_DIRECTORY'."
        complete_process 1
    fi
else
    if ! cd $TEST_DIRECTORY; then
        echo "Test script cannot change to the test directory '$TEST_DIRECTORY'."
        complete_process 1
    fi
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Running debug commands through project in test directory '$TEST_DIRECTORY'."
fi
TEST_START_MARK=$(date +%s.%N)
if [ "$VERY_VERBOSE_MODE" -ne 0 ]; then
    DID_FAIL=0
    ./run.sh -v -a debug commands.txt
    DID_FAIL=$?
else
    DID_FAIL=0
    ./run.sh -v -a debug commands.txt >$TEMP_FILE 2>&1
    DID_FAIL=$?
fi
TEST_END_MARK=$(date +%s.%N)
if [ "$DID_FAIL" -ne 0 ]; then
    if [ "$VERY_VERBOSE_MODE" -eq 0 ]; then
        cat $TEMP_FILE
    fi
    echo "Test script cannot run the project in directory '$TEST_DIRECTORY'."
    complete_process 1
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Verifying expected results against actual results."
fi
if ! diff expected_output.json build/output.json >$TEMP_FILE 2>&1 ; then
    echo "Test results were not as expected."
    echo "Differences between expected and actual results located at: $TEMP_FILE."
    complete_process 1
fi

TEST_RUNTIME=$( echo "$TEST_END_MARK - $TEST_START_MARK" | bc -l )
echo "Test executed in $TEST_RUNTIME ms."

# If we get here, we have a clean exit from the script.
complete_process 0 "$1"
