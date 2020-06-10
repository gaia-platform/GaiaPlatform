#!usr/bin/env python3
from __future__ import annotations

from argparse import ArgumentParser as _ArgumentParser
from dataclasses import dataclass
from typing import Iterable, Optional
import sys


@dataclass(frozen=True)
class Options:

    @staticmethod
    def of_argv(argv: Iterable[str] = tuple(sys.argv)) -> Options:
        return Options()


class ArgumentParser(_ArgumentParser):

    @staticmethod
    def of_argv(argv: Iterable[str] = tuple(sys.argv)) -> ArgumentParser:
        argument_parser = ArgumentParser()

        return argument_parser


def main(argv: Optional[Iterable[str]] = None) -> int:
    options = Options.of_argv()

    return 0


if __name__ == '__main__':
    import sys
    sys.exit(main())
