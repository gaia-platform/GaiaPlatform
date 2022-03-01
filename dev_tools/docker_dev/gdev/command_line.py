#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for all interactions with the command line.
"""

from argparse import ArgumentParser, REMAINDER, SUPPRESS
import platform
from gdev.custom.gaia_path import GaiaPath
from gdev.parser_structure import ParserStructure
from gdev.mount import Mount
from gdev.host import Host


class CommandLine:
    """
    Class to provide for all interactions with the command line.
    """

    __LOG_LEVELS = ["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG"]

    __PLATFORM_CHOICE_MAP = {
        "x86_64": "amd64",
        "aarch64": "arm64",
    }

    __SUBCOMMAND_ID_CFG = 0
    __SUBCOMMAND_ID_DOCKERFILE = 1
    __SUBCOMMAND_ID_BUILD = 2
    __SUBCOMMAND_ID_RUN = 3
    __SUBCOMMAND_ID_PUSH = 4
    __SUBCOMMAND_TO_ID_MAP = {
        "cfg": __SUBCOMMAND_ID_CFG,
        "dockerfile": __SUBCOMMAND_ID_DOCKERFILE,
        "build": __SUBCOMMAND_ID_BUILD,
        "run": __SUBCOMMAND_ID_RUN,
        "push": __SUBCOMMAND_ID_PUSH,
    }

    DOUBLE_DASH_ARGUMENT = "--"

    __DEFAULT_LOG_LEVEL = "INFO"
    __DEFAULT_BASE_IMAGE = "ubuntu:20.04"
    __DEFAULT_PORTS = []
    __DEFAULT_MOUNTS = []
    __DEFAULT_REGISTRY = None
    __DEFAULT_PLATFORM = __PLATFORM_CHOICE_MAP[platform.machine()]
    __DEFAULT_MIXINS = []
    __DEFAULT_CFG_ENABLES = []

    __is_backward_compatible_enabled = False

    @staticmethod
    def set_backward_mode(value):
        """
        Set the backward compatible mode to allow the interface to look exactly
        like it did in the old GDev.
        """
        CommandLine.__is_backward_compatible_enabled = value

    @staticmethod
    def is_backward_compatibility_mode_enabled():
        """
        Determine if backward compatible mode is enabled.
        """
        return CommandLine.__is_backward_compatible_enabled

    @staticmethod
    def __add_base_image(parser):

        parser.add_argument(
            "--base-image",
            default=CommandLine.__DEFAULT_BASE_IMAGE,
            help=f'Base image for build. Default: "{CommandLine.__DEFAULT_BASE_IMAGE}"',
        )

    @staticmethod
    def __add_cfg_enables(parser):

        #  NOTE: Due to nargs='*', this will swallow all remaining arguments.
        #  NOTE: Cfg does not warn if provided --cfg-enable is not present in any gdev.cfg files
        parser.add_argument(
            "--cfg-enables",
            default=CommandLine.__DEFAULT_CFG_ENABLES,
            nargs="*",
            help=(
                f"Enable lines in gdev.cfg files gated by `enable_if`, `enable_if_any`, and"
                f' `enable_if_all` functions. Default: "{CommandLine.__DEFAULT_CFG_ENABLES}"'
            ),
        )

    @staticmethod
    def __add_log_level(parser):
        parser.add_argument(
            "--log-level",
            default=CommandLine.__DEFAULT_LOG_LEVEL,
            choices=CommandLine.__LOG_LEVELS,
            help=f'Log level. Default: "{CommandLine.__DEFAULT_LOG_LEVEL}"',
        )

    @staticmethod
    def __add_force_build(parser):
        parser.add_argument(
            "-f",
            "--force",
            action="store_true",
            help="Force Docker to build with local changes.",
        )

    @staticmethod
    def __add_mixins(parser):
        parser.add_argument(
            "--mixins",
            default=CommandLine.__DEFAULT_MIXINS,
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
                f' directory. Default: "{CommandLine.__DEFAULT_MIXINS}"'
            ),
        )

    @staticmethod
    def __add_mounts(parser):
        parser.add_argument(
            "--mounts",
            default=CommandLine.__DEFAULT_MOUNTS,
            nargs=1,
            help=(
                f"<host_path>:<container_path> mounts to be created (or if already created,"
                f" resumed) during `docker run`. Paths may be specified as relative paths."
                f" <host_path> relative paths are relative to the host's current working"
                f" directory. <container_path> relative paths are relative to the Docker"
                f" container's WORKDIR (AKA the build dir). Default:"
                f' "{" ".join(CommandLine.__DEFAULT_MOUNTS)}"'
            ),
        )

    @staticmethod
    def __add_platform(parser):
        parser.add_argument(
            "--platform",
            default=CommandLine.__DEFAULT_PLATFORM,
            choices=sorted(CommandLine.__PLATFORM_CHOICE_MAP.values()),
            help=f'Platform to build upon. Default: "{CommandLine.__DEFAULT_PLATFORM}"',
        )

    @staticmethod
    def __add_ports(parser):
        parser.add_argument(
            "-p",
            "--ports",
            default=CommandLine.__DEFAULT_PORTS,
            nargs="*",
            type=int,
            help="Ports to expose in underlying docker container. "
            + f'Default: "{CommandLine.__DEFAULT_PORTS}"',
        )

    @staticmethod
    def __add_registry(parser):
        parser.add_argument(
            "--registry",
            default=CommandLine.__DEFAULT_REGISTRY,
            help=(
                "Registry to push images and query cached build stages."
                f" Default: {CommandLine.__DEFAULT_REGISTRY}"
            ),
        )

    @staticmethod
    def __add_docker_run_arguments(parser, is_backward_compatible_guess):

        if is_backward_compatible_guess:
            help_text = "Args to be forwarded on to docker run, if applicable."
        else:
            help_text = (
                "Zero or more arguments to be forwarded on to docker run. "
                + "If one or more arguments are provided, the first argument "
                + f"must be `{CommandLine.DOUBLE_DASH_ARGUMENT}`."
            )

        parser.add_argument(
            "args",
            nargs=REMAINDER,
            help=help_text,
        )

    @staticmethod
    def __add_flags(
        parser: ArgumentParser, parser_name: str, is_backward_compatible_guess: bool
    ) -> None:

        assert parser_name in CommandLine.__SUBCOMMAND_TO_ID_MAP
        subcommand_id = CommandLine.__SUBCOMMAND_TO_ID_MAP[parser_name]

        _ = parser_name
        parser.add_argument(
            "--dry-dock",
            action="store_true",
            default=False,
            help=SUPPRESS,
        )
        parser.add_argument(
            "--backward",
            action="store_true",
            default=False,
            help=SUPPRESS,
        )

        if is_backward_compatible_guess:
            CommandLine.__add_base_image(parser)
            CommandLine.__add_cfg_enables(parser)
            CommandLine.__add_log_level(parser)
            CommandLine.__add_force_build(parser)
            CommandLine.__add_mixins(parser)
            CommandLine.__add_mounts(parser)
            CommandLine.__add_platform(parser)
            CommandLine.__add_ports(parser)
            CommandLine.__add_registry(parser)
            CommandLine.__add_docker_run_arguments(parser, is_backward_compatible_guess)
        else:
            CommandLine.__add_log_level(parser)
            CommandLine.__add_cfg_enables(parser)
            if subcommand_id >= CommandLine.__SUBCOMMAND_ID_DOCKERFILE:
                CommandLine.__add_base_image(parser)
                CommandLine.__add_mixins(parser)
            if subcommand_id >= CommandLine.__SUBCOMMAND_ID_BUILD:
                CommandLine.__add_platform(parser)
                CommandLine.__add_registry(parser)
            if subcommand_id == CommandLine.__SUBCOMMAND_ID_RUN:
                CommandLine.__add_force_build(parser)
                CommandLine.__add_mounts(parser)
                CommandLine.__add_ports(parser)
                CommandLine.__add_docker_run_arguments(
                    parser, is_backward_compatible_guess
                )

    @staticmethod
    def get_parser(parser_structure, is_backward_compatible_guess) -> ArgumentParser:
        """
        Calculate the arguments to show for the given subcommand.
        """

        def inner(
            parser: ArgumentParser, parser_structure: ParserStructure, parser_name: str
        ) -> ArgumentParser:
            if not parser_structure.sub_parser_structures:
                CommandLine.__add_flags(
                    parser, parser_name, is_backward_compatible_guess
                )
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
                    inner(sub_parser, sub_parser_map[next_map_key], next_map_key)

            if is_backward_compatible_guess:
                parser.description = parser_structure.doc
            else:
                parser.description = parser_structure.alt_doc

            return parser

        return inner(
            ArgumentParser(prog="gdev", allow_abbrev=False),
            parser_structure=parser_structure,
            parser_name=None,
        )

    @staticmethod
    def __get_argument_with_default(parsed_args, entry_name, default_value):
        if entry_name not in parsed_args:
            parsed_args[entry_name] = default_value
        return parsed_args[entry_name]

    @staticmethod
    def interpret_arguments(parsed_args):
        """
        Interpret the arguments that were supplied, inline to the parsed_args dictionary.
        """

        if "dry_dock" in parsed_args:
            Host.set_drydock(parsed_args["dry_dock"])
            del parsed_args["dry_dock"]
        if "backward" in parsed_args:
            CommandLine.set_backward_mode(parsed_args["backward"])
            del parsed_args["backward"]

        # Note: https://stackoverflow.com/questions/22850332/
        #       getting-the-remaining-arguments-in-argparse
        if "args" in parsed_args:
            if CommandLine.is_backward_compatibility_mode_enabled():
                if (
                    parsed_args["args"]
                    and parsed_args["args"][0] == CommandLine.DOUBLE_DASH_ARGUMENT
                ):
                    parsed_args["args"] = parsed_args["args"][1:]
                parsed_args["args"] = " ".join(parsed_args["args"])
            else:
                if (
                    parsed_args["args"]
                    and parsed_args["args"][0] != CommandLine.DOUBLE_DASH_ARGUMENT
                ):
                    raise ValueError(
                        "arguments to pass to docker run must be prefaced "
                        + f"with `{CommandLine.DOUBLE_DASH_ARGUMENT}`"
                    )
                parsed_args["args"] = " ".join(parsed_args["args"][1:])
        else:
            parsed_args["args"] = ""

        base_image = CommandLine.__get_argument_with_default(
            parsed_args, "base_image", CommandLine.__DEFAULT_BASE_IMAGE
        )
        mixins = CommandLine.__get_argument_with_default(
            parsed_args, "mixins", CommandLine.__DEFAULT_MIXINS
        )
        parsed_args["mixins"] = frozenset(mixins)

        parsed_args["cfg_enables"] = frozenset(
            [
                base_image,
                *parsed_args["cfg_enables"],
                *mixins,
            ]
        )

        CommandLine.__get_argument_with_default(parsed_args, "force", False)
        CommandLine.__get_argument_with_default(
            parsed_args, "platform", CommandLine.__DEFAULT_PLATFORM
        )
        CommandLine.__get_argument_with_default(
            parsed_args, "registry", CommandLine.__DEFAULT_REGISTRY
        )

        ports = (
            [str(port) for port in parsed_args["ports"]]
            if "ports" in parsed_args
            else CommandLine.__DEFAULT_PORTS
        )
        parsed_args["ports"] = frozenset(ports)

        mounts = CommandLine.__DEFAULT_MOUNTS
        if "mounts" in parsed_args:
            for mount in parsed_args["mounts"]:
                host_path, container_path = mount.split(":", 1)
                host_path, container_path = GaiaPath(host_path), GaiaPath(
                    container_path
                )
                if not host_path.is_absolute():
                    host_path = host_path.absolute()
                if not container_path.is_absolute():
                    container_path = container_path.absolute().image_build()
                mounts.append(Mount(container_path=container_path, host_path=host_path))
        parsed_args["mounts"] = frozenset(mounts)
