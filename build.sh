#! /usr/bin/bash

start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo Building the project...
    fi
}

complete_process() {
    # $1 is the return code to assign to the script
    if [ "$1" -ne 0 ]; then
        echo "Build of the project failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Build of the project succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi

    exit "$1"
}

show_usage() {
    echo "Usage: $(basename "$0") [flags]"
    echo "Flags:"
    echo "  -v,--verbose      Show lots of information while building the project."
    echo "  -f,--force        Force a complete build of the project."
    echo "  -m,--make-only    Build only the make part of the project."
    echo "  -nc,--no-cache    (Experimental) Build the project with no CMake cache."
    echo "  -r,--refresh-ddl  (Experimental) Build the project with refreshed DDL."
    echo "  -h,--help         Display this help text."
    echo " "
    echo "Note: The --refresh-ddl flag explicitly drops the database to ensure that the"
    echo "      DDL file is regenerated from scratch.  Please use accordingly."
    exit 1
}

# Set up any script variables.
TEMP_FILE=/tmp/incubator.build.tmp

# Parse any command line values.
VERBOSE_MODE=0
NO_CACHE=0
REFRESH_DDL=0
FORCE_BUILD=0
MAKE_MODE=0
PARAMS=""
while (( "$#" )); do
  case "$1" in
    -nc|--no-cache)
      NO_CACHE=1
      shift
      ;;
    -r|--refresh-ddl)
      REFRESH_DDL=1
      shift
      ;;
    -f|--force)
      FORCE_BUILD=1
      shift
      ;;
    -m|--make-only)
      MAKE_MODE=1
      shift
      ;;
    -v|--verbose)
      VERBOSE_MODE=1
      shift
      ;;
    -h|--help) # unsupported flags
      show_usage
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

# Make sure that competing command line flags are called out.
if [ $FORCE_BUILD -eq 1 ] && [ $MAKE_MODE -eq 1 ]; then
    echo "Force flag and the Make-Only flag are not compatible with each other."
    complete_process 1
fi

if [ $REFRESH_DDL -eq 1 ] && [ $MAKE_MODE -eq 1 ]; then
    echo "Refresh DDL flag and the Make-Only flag are not compatible with each other."
    complete_process 1
fi

if [ $NO_CACHE -eq 1 ] && [ $MAKE_MODE -eq 1 ]; then
    echo "No (CMake) Cache flag and the Make-Only flag are not compatible with each other."
    complete_process 1
fi


# Clean entrance into the script.
start_process

# Force a complete rebuild of everything.
if [ $FORCE_BUILD -eq 1 ]; then
    if [ $VERBOSE_MODE -ne 0 ]; then
        echo "Force option selected. Forcing rebuild of all build artifacts."
    fi
    rm -rf build
fi

# If the build directory does not already exist, then create it.
if [ ! -d "build" ]; then
    # Note that at this point, even if the flag has not been set, we
    #   treat the build as a forced build to ensure everything is set
    #   properly.
    FORCE_BUILD=1

    if ! mkdir build; then
        echo "Creation of the build directory failed."
        complete_process 1
    fi
fi

# If the flag is set to not use the cached CMake data, then remove it first.
if [ $NO_CACHE -eq 1 ]; then
    if [ $VERBOSE_MODE -ne 0 ]; then
        echo "No-cache option selected.  Removing CMake cache file."
    fi
    rm build/CMakeCache.txt
fi

# If the flag is set to refresh the DDL data, then do so.
if [ $REFRESH_DDL -eq 1 ]; then
    if [ $VERBOSE_MODE -ne 0 ]; then
        echo "Refresh-ddl option selected.  Removing database 'incubator'."
    fi
    echo "drop database incubator" > $TEMP_FILE
    echo "exit" >> $TEMP_FILE
    gaiac -i < $TEMP_FILE > /dev/null
fi

# Generate the Makefile
if [ $VERBOSE_MODE -ne 0 ]; then
    echo "Building the makefile..."
    cmake -B build
    DID_FAIL=$?
else
    cmake -B build >$TEMP_FILE 2>&1
    DID_FAIL=$?
fi
if [ $DID_FAIL -ne 0 ]; then
    if [ $VERBOSE_MODE -eq 0 ]; then
       cat $TEMP_FILE
    fi
    echo "Build of the makefile failed."
    complete_process 1
fi

# Build the actual executable file.
if [ $VERBOSE_MODE -ne 0 ]; then
    echo "Building the executable file..."
fi
if ! make -C build --silent; then
    echo "Build of the executable file failed."
    complete_process 1
fi


# If we get here, we have a clean exit from the script.
complete_process 0
