# PYTHON_ARGCOMPLETE_OK

from __future__ import annotations
from argparse import ArgumentParser, RawTextHelpFormatter, REMAINDER
from asyncio import gather
from dataclasses import dataclass
from importlib import import_module
from importlib.util import find_spec
from inspect import getdoc, isabstract, iscoroutinefunction
import logging
from pkgutil import iter_modules
import sys
from typing import Sequence

from gdev.custom.pathlib import Path
from gdev.options import Options
from gdev.third_party.atools import memoize
from gdev.third_party.argcomplete import autocomplete


@dataclass(frozen=True)
class Dependency:
    # The gdev CLI uses docstrings in the help output. This is what they'll see if a subclass does
    # not provide a docstring.

    options: Options

    def __hash__(self) -> int:
        return hash((type(self), self.options))

    def __repr__(self) -> str:
        return f'{type(self).__name__}({self.options.target})'

    def __str__(self) -> str:
        return repr(self)

    @property
    @memoize
    def log(self) -> logging.Logger:
        log = logging.getLogger(f'{self.__module__} ({self.options.target})')
        log.setLevel(self.options.log_level)
        log.propagate = False

        handler = logging.StreamHandler(sys.stderr)
        handler.setLevel(self.options.log_level)
        if handler.level > logging.DEBUG:
            formatter = logging.Formatter(f'({self.options.target}) %(message)s')
        else:
            formatter = logging.Formatter(f'%(levelname)s:%(name)s %(message)s')
        handler.setFormatter(formatter)
        log.addHandler(handler)

        return log

    @memoize
    async def cli_entrypoint(self) -> None:
        await self.run()

    @memoize
    async def run(self) -> None:
        if isabstract(self):
            return

        if hasattr(self, 'main'):
            self.log.debug(f'Starting {type(self).__name__}.main')
            await self.main()
            self.log.debug(f'Finished {type(self).__name__}.main')

        if self.options.log_level == 'DEBUG':
            getters = []
            for cls in reversed(type(self).mro()):
                for k, v in cls.__dict__.items():
                    if k.startswith('get_') and iscoroutinefunction(v) and hasattr(v, 'memoize'):
                        getters.append(getattr(self, k)())

            await gather(*[getter for getter in getters])

    @staticmethod
    def of_args(args: Sequence[str]) -> Dependency:
        """Return Dependency constructed by parsing args as if from sys.argv."""

        # RawTextHelpFormatter preserves likely-intentional formatting like numbered lists.
        parser = ArgumentParser(prog='gdev', formatter_class=RawTextHelpFormatter)
        cmd_parts = []
        sub_parser = parser
        for arg in args:
            if arg.startswith('-'):
                break
            else:
                cmd_parts.append(arg)

        while cmd_parts:
            module_name = '.'.join(['gdev', 'cmd', *cmd_parts])
            try:
                spec = find_spec(module_name)
            except ModuleNotFoundError:
                spec = None

            if spec is None:
                cmd_parts = cmd_parts[:-1]
            else:
                break
        else:
            spec = find_spec('gdev.cmd')

        for cmd_part in cmd_parts:
            sub_parsers = sub_parser.add_subparsers()
            sub_parser = sub_parsers.add_parser(cmd_part, formatter_class=RawTextHelpFormatter)

        cmd_module = '.'.join(['gdev', 'cmd', *cmd_parts])
        module = import_module(cmd_module)
        cmd_class = ''.join([
            part.capitalize()
            for cmd_part in cmd_parts
            for part in cmd_part.split('_')
            if part
        ])

        if spec.submodule_search_locations is not None:
            # This is a folder, so find all the subcommands.
            sub_parsers = sub_parser.add_subparsers()
            sub_parser.description = getdoc(module) or ''
            for module_info in iter_modules(spec.submodule_search_locations):
                if not module_info.name.startswith('_'):
                    sub_parsers.add_parser(module_info.name)
        else:
            # This is a file, so it is a final subcommand.
            sub_parser.description = getdoc(module.__dict__[cmd_class]) or ''
            sub_parser.add_argument(
                'args',
                nargs=REMAINDER,
                help=f'Args to be forwarded on to docker run, if applicable.'
            )
            base_image_default = 'ubuntu:20.04'
            sub_parser.add_argument(
                '--base-image',
                default=base_image_default,
                help=f'Base image for build. Default: "{base_image_default}"'
            )
            log_level_default = 'INFO'
            sub_parser.add_argument(
                '--log-level',
                default=log_level_default,
                choices=[name for _, name in sorted(logging._levelToName.items())],
                help=f'Log level. Default: "{log_level_default}"'
            )
            sub_parser.add_argument(
                '--mixins',
                default=[],
                nargs='+',
                choices=sorted([
                    directory.name
                    for directory in Path.mixin().iterdir()
                    if directory.is_dir()
                ]),
                help=(
                    f'Image mixins to use when creating a container. Mixins provide dev tools and'
                    f' configuration from targets in the "{Path.mixin().relative_to(Path.repo())}"'
                    f' directory.'
                )
            )
            platform_default = 'linux/amd64'
            sub_parser.add_argument(
                '--platform',
                default=platform_default,
                choices=['linux/amd64', 'linux/arm64'],
                help=f'Platform to build upon. Default: "{platform_default}"'
            )
            exclusive_args = sub_parser.add_mutually_exclusive_group()
            exclusive_args.add_argument(
                '--tainted',
                action='store_true',
                help=(
                    'Allow uncommitted changes while building images. Images built this way are'
                    ' not reproducible and are thus tainted. Tainted images can not be uploaded to'
                    ' docker image repository.'
                )
            )
            exclusive_args.add_argument(
                '--upload',
                action='store_true',
                help='Upload image to Gaia docker image repository after building.'
            )

        autocomplete(parser)

        parsed_args = parser.parse_args(args).__dict__
        if not parsed_args:
            sub_parser.print_help()
            import sys
            sys.exit(1)

        parsed_args['args'] = ' '.join(parsed_args['args'])
        parsed_args['mixins'] = tuple(sorted(set(parsed_args['mixins'])))

        options = Options(
            target=f'{Path.cwd().relative_to(Path.repo())}',
            **parsed_args
        )

        dependency = getattr(import_module(cmd_module), cmd_class)(options)

        return dependency

    @staticmethod
    def of_sys_argv() -> Dependency:
        """Return Dependency constructed by parsing args from sys.argv."""
        return Dependency.of_args(tuple(sys.argv[1:]))
