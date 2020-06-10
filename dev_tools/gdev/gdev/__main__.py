#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK


def main() -> int:
    from gdev.dependency import Dependency
    dependency = Dependency.of_sys_argv()

    import logging
    logging.basicConfig(level=dependency.options.log_level)

    import asyncio
    asyncio.run(dependency.run())

    return 0


if __name__ == '__main__':
    import sys
    sys.exit(main())
