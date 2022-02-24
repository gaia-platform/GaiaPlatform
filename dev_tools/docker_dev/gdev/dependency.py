#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide a base class for use by all the subcommand modules.
"""

# PYTHON_ARGCOMPLETE_OK

from __future__ import annotations
from argparse import ArgumentParser, REMAINDER
from dataclasses import dataclass
from importlib import import_module
from inspect import isabstract
import logging
import platform
import sys
from typing import Sequence
import argparse

from gdev.custom.gaia_path import GaiaPath
from gdev.options import Options
from gdev.mount import Mount
from gdev.third_party.atools import memoize, memoize_db
from gdev.third_party.argcomplete import autocomplete, FilesCompleter
from gdev.parser_structure import ParserStructure
from gdev.host import Host


@dataclass(frozen=True)
class Dependency:
    """
    Bob.Dependency
    """

    # The gdev CLI uses docstrings in the help output. This is what they'll see if a
    # subclass does not provide a docstring.

    options: Options

    __LOG_LEVELS = ["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG"]

    # These two classes are only present to handle the Abort exception,
    # and early exit in some cases.
    class Exception(Exception):
        """
        Useless exception to be refactored out.
        """

    class Abort(Exception):
        """
        Exception to know we aborted.
        """

        def __str__(self) -> str:
            return f"Abort: {super().__str__()}"

    def __hash__(self) -> int:
        return hash((type(self), self.options))

    def __repr__(self) -> str:
        return f"{type(self).__name__}({self.options.target})"

    def __str__(self) -> str:
        return repr(self)

    @property
    @memoize
    def log(self) -> logging.Logger:
        """
        Set up a logging instance for the current module.

        Note that the @memoize decorator means that this will only be setup
        once for any given instance of a class.
        """
        log = logging.getLogger(f"{self.__module__} ({self.options.target})")
        log.setLevel(self.options.log_level)
        log.propagate = False

        handler = logging.StreamHandler(sys.stderr)
        handler.setLevel(self.options.log_level)
        if handler.level > logging.DEBUG:
            formatter = logging.Formatter(f"({self.options.target}) %(message)s")
        else:
            formatter = logging.Formatter("%(levelname)s:%(name)s %(message)s")
        handler.setFormatter(formatter)
        log.addHandler(handler)
        return log

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        self.run()

    @staticmethod
    @memoize_db(size=1)
    def get_parser_structure() -> ParserStructure:
        """
        Create a parser structure out of the command parts.
        """
        return ParserStructure.of_command_parts(tuple())

    @staticmethod
    def get_parser() -> ArgumentParser:
        """
        Calculate the arguments to show for the given subcommand.
        """

        def add_flags(parser: ArgumentParser) -> None:

            parser.add_argument(
                "--dry-dock",
                action="store_true",
                default=False,
                help=argparse.SUPPRESS,
            )

            # Dockerfile, and above
            base_image_default = "ubuntu:20.04"
            parser.add_argument(
                "--base-image",
                default=base_image_default,
                help=f'Base image for build. Default: "{base_image_default}"',
            )

            # Cfg, and above.
            # For Cfg, will add `# enable by setting` comments when the cfg output is displayed
            #  indicating what the cfg file will look like with that cfg-enable applied.
            #  NOTE: Due to nargs='*', this will swallow all remaining arguments.
            #  NOTE: Cfg does not warn if provided --cfg-enable is not present in any gdev.cfg files
            cfg_enables_default = []
            parser.add_argument(
                "--cfg-enables",
                default=cfg_enables_default,
                nargs="*",
                help=(
                    f"Enable lines in gdev.cfg files gated by `enable_if`, `enable_if_any`, and"
                    f' `enable_if_all` functions. Default: "{cfg_enables_default}"'
                ),
            )

            # Only used when GDev is being set up.
            log_level_default = "INFO"
            parser.add_argument(
                "--log-level",
                default=log_level_default,
                choices=Dependency.__LOG_LEVELS,
                help=f'Log level. Default: "{log_level_default}"',
            )

            # Only used as part of "run" to force a build.
            parser.add_argument(
                "-f",
                "--force",
                action="store_true",
                help="Force Docker to build with local changes.",
            )

            # Dockerfile, and above
            mixins_default = []
            parser.add_argument(
                "--mixins",
                default=mixins_default,
                nargs="*",
                choices=sorted(
                    [
                        directory.name
                        for directory in GaiaPath.mixin().iterdir()
                        if directory.is_dir()
                    ]
                ),
                help=(
                    f"Image mixins to use when creating a container. Mixins provide dev tools and"
                    f" configuration from targets in "
                    f'the "{GaiaPath.mixin().relative_to(GaiaPath.repo())}"'
                    f' directory. Default: "{mixins_default}"'
                ),
            )
            mounts_default = []
            parser.add_argument(
                "--mounts",
                default=[],
                nargs=1,
                help=(
                    f"<host_path>:<container_path> mounts to be created (or if already created,"
                    f" resumed) during `docker run`. Paths may be specified as relative paths."
                    f" <host_path> relative paths are relative to the host's current working"
                    f" directory. <container_path> relative paths are relative to the Docker"
                    f" container's WORKDIR (AKA the build dir). Default:"
                    f' "{" ".join(mounts_default)}"'
                ),
            )

            # Build +
            platform_default = {
                "x86_64": "amd64",
                "aarch64": "arm64",
            }[platform.machine()]
            parser.add_argument(
                "--platform",
                default=platform_default,
                choices=["amd64", "arm64"],
                help=f'Platform to build upon. Default: "{platform_default}"',
            )
            ports_default = []
            parser.add_argument(
                "-p",
                "--ports",
                default=ports_default,
                nargs="*",
                type=int,
                help=f'Ports to expose in underlying docker container. Default: "{ports_default}"',
            )

            # Build +
            registry_default = None
            parser.add_argument(
                "--registry",
                default=registry_default,
                help=(
                    "Registry to push images and query cached build stages."
                    f" Default: {registry_default}"
                ),
            )
            parser.add_argument(
                "args",
                nargs=REMAINDER,
                help="Args to be forwarded on to docker run, if applicable.",
            )

        def inner(
            parser: ArgumentParser, parser_structure: ParserStructure
        ) -> ArgumentParser:
            if not parser_structure.sub_parser_structures:
                add_flags(parser)
                parser.set_defaults(
                    command_class=parser_structure.get_command_class(),
                    command_module=parser_structure.get_command_module(),
                )
            else:
                sub_parsers = parser.add_subparsers()
                sub_parser_map = {}
                for sub_parser_structure in parser_structure.sub_parser_structures:
                    sub_parser_map[
                        sub_parser_structure.command_parts[-1]
                    ] = sub_parser_structure
                for next_map_key in sorted(sub_parser_map.keys()):
                    sub_parser = sub_parsers.add_parser(next_map_key)
                    inner(sub_parser, sub_parser_map[next_map_key])

            # NOTE: This is where the description for the parser element in the help
            # is arrived
            parser.description = parser_structure.doc

            return parser

        return inner(
            ArgumentParser(prog="gdev"),
            parser_structure=Dependency.get_parser_structure(),
        )

    @memoize
    def run(self) -> None:
        """
        Run the main action that is the focus of this class.
        """

        if isabstract(self):
            return

        if hasattr(self, "main"):
            self.log.debug("Starting %s.main", str(type(self).__name__))
            self.main()
            self.log.debug("Finished %s.main", str(type(self).__name__))

        # if self.options.log_level == 'DEBUG':
        #     getters = []
        #     for cls in reversed(type(self).mro()):
        #         for k, v in cls.__dict__.items():
        #             if k.startswith('get_') and iscoroutinefunction(v) and hasattr(v, 'memoize'):
        #                 getters.append(getattr(self, k)())

        #     await gather(*[getter for getter in getters])

    @staticmethod
    def of_args(args: Sequence[str]) -> Dependency:
        """
        Return Dependency constructed by parsing args as if from sys.argv.
        """

        parser = Dependency.get_parser()
        autocomplete(
            parser, default_completer=FilesCompleter(allowednames="", directories=False)
        )

        parsed_args = parser.parse_args(args).__dict__
        if not parsed_args:
            Dependency.get_parser_structure.memoize.remove()
            parser.parse_args([*args, "--help"])
            sys.exit(1)

        if "dry_dock" in parsed_args:
            Host.set_drydock(parsed_args["dry_dock"])
            del parsed_args["dry_dock"]

        if parsed_args["args"] and parsed_args["args"][0] == "--":
            parsed_args["args"] = parsed_args["args"][1:]
        parsed_args["args"] = " ".join(parsed_args["args"])
        parsed_args["cfg_enables"] = frozenset(
            [
                parsed_args["base_image"],
                *parsed_args["cfg_enables"],
                *parsed_args["mixins"],
            ]
        )
        parsed_args["mixins"] = frozenset(parsed_args["mixins"])
        parsed_args["ports"] = frozenset(str(port) for port in parsed_args["ports"])

        mounts = []
        for mount in parsed_args["mounts"]:
            host_path, container_path = mount.split(":", 1)
            host_path, container_path = GaiaPath(host_path), GaiaPath(container_path)
            if not host_path.is_absolute():
                host_path = host_path.absolute()
            if not container_path.is_absolute():
                container_path = container_path.absolute().image_build()
            mounts.append(Mount(container_path=container_path, host_path=host_path))
        parsed_args["mounts"] = frozenset(mounts)

        command_class = parsed_args.pop("command_class")
        command_module = parsed_args.pop("command_module")

        options = Options(
            target=f"{GaiaPath.cwd().relative_to(GaiaPath.repo())}", **parsed_args
        )

        dependency = getattr(import_module(command_module), command_class)(options)

        return dependency

    @staticmethod
    def of_sys_argv() -> Dependency:
        """
        Return Dependency constructed by parsing args from sys.argv.
        """
        return Dependency.of_args(tuple(sys.argv[1:]))
