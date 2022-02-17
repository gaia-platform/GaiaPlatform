#!/usr/bin/env python3.8

# PYTHON_ARGCOMPLETE_OK

import sys
from gdev.main import DockerDev

def main() -> int:
    DockerDev().main()
    return 0

if __name__ == '__main__':
    sys.exit(main())
