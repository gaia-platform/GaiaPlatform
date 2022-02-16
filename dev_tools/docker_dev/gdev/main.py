"""
Module to
"""

import sys
from time import sleep

class DockerDev():
    def main(self):
        """
        Main entrance point.
        """
        from gdev.dependency import Dependency
        dependency = Dependency.of_sys_argv()

        import logging
        logging.basicConfig(level=dependency.options.log_level)

        import asyncio
        try:
            asyncio.run(dependency.cli_entrypoint())
        except dependency.Exception as e:
            print(f'\n{e}', file=sys.stderr)
        finally:
            logging.shutdown()

        return 0

if __name__ == "__main__":
    DockerDev().main()
