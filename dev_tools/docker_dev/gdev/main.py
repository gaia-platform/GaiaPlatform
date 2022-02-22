"""
Module to provide a single entry point from the operating system.
"""

import sys
from time import sleep

class DockerDev():
    """
    Class to provide a single entry point from the operating system.
    """
    def main(self):
        """
        Main entry point from the operating system.
        """
        from gdev.dependency import Dependency
        dependency = Dependency.of_sys_argv()

        import logging
        logging.basicConfig(level=dependency.options.log_level)

        try:
            dependency.cli_entrypoint()
        except dependency.Exception as e:
            print(f'\n{e}', file=sys.stderr)
        finally:
            logging.shutdown()

        return 0

if __name__ == "__main__":
    DockerDev().main()
