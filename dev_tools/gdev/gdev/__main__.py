#!/usr/bin/env python3.8
# PYTHON_ARGCOMPLETE_OK


def main() -> int:
    from gdev.dependency import Dependency
    dependency = Dependency.of_sys_argv()

    import logging
    logging.basicConfig(level=dependency.options.log_level)

    import asyncio
    try:
        asyncio.run(dependency.cli_entrypoint())
    except dependency.Exception as e:
        print(f'\n{e}', file=sys.stderr)

    return 0


if __name__ == '__main__':
    import sys
    sys.exit(main())
