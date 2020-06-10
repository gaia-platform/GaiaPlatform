# PYTHON_ARGCOMPLETE_OK

from __future__ import annotations
from argparse import ArgumentParser
from asyncio import gather, ensure_future
from dataclasses import dataclass
from importlib import import_module
from importlib.util import find_spec
from inspect import isabstract, iscoroutinefunction
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
        log = logging.getLogger(self.__module__)
        log.setLevel(self.options.log_level)
        log.propagate = False

        handler = logging.StreamHandler(sys.stdout)
        handler.setLevel(self.options.log_level)
        if handler.level > logging.DEBUG:
            formatter = logging.Formatter(f'({self.options.target}) %(message)s')
        else:
            formatter = logging.Formatter(
                f'%(levelname)s:%(name)s ({self.options.target}) %(message)s'
            )
        handler.setFormatter(formatter)
        log.addHandler(handler)

        return log

    @memoize
    async def run(self) -> None:
        if isabstract(self):
            return

        if hasattr(self, 'main'):
            self.log.debug(f'Starting {type(self).__name__}.main')
            await self.main()
            self.log.debug(f'Finished {type(self).__name__}.main')

        await self._post_main()

    async def _post_main(self) -> None:
        """Find all memoized getters and start them."""
        if self.options.log_level == 'DEBUG':
            getters = []
            for cls in reversed(type(self).mro()):
                for k, v in cls.__dict__.items():
                    if k.startswith('get_') and iscoroutinefunction(v) and hasattr(v, 'memoize'):
                        getters.append(getattr(self, k)())

            await ensure_future(gather(*[getter for getter in getters]))

    @staticmethod
    def of_args(args: Sequence[str]) -> Dependency:
        parser = ArgumentParser(prog='gdev')
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
            sub_parser = sub_parsers.add_parser(cmd_part)

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
            sub_parser.description = module.__doc__ or ''
            for module_info in iter_modules(spec.submodule_search_locations):
                if not module_info.name.startswith('_'):
                    sub_parsers.add_parser(module_info.name)
        else:
            # This is a file, so it is a final subcommand.
            sub_parser.description = module.__dict__[cmd_class].__doc__ or ''
            target_default = '.'
            sub_parser.add_argument(
                'target',
                nargs='?',
                default=target_default,
                help=f'Path relative to repo root to be built. Default: "{target_default}"'
            )
            base_image_default = 'ubuntu:latest'
            sub_parser.add_argument(
                '--base-image',
                default=base_image_default,
                help=f'Base image for build. Default: "{base_image_default}"'
            )
            sub_parser.add_argument(
                '--inline',
                action='store_true',
                help='Inline input dockerfile stages in dockerfile.'
            )
            log_level_default = 'INFO'
            sub_parser.add_argument(
                '--log-level',
                default=log_level_default,
                choices=[name for _, name in sorted(logging._levelToName.items())],
                help=f'Log level. Default: "{log_level_default}"'
            )
            mixin_default = 'base'
            sub_parser.add_argument(
                '--mixin',
                default=mixin_default,
                choices=sorted([directory.name for directory in Path.mixin().iterdir()]),
                help=(
                    f'Image mixin to use when creating a container. Mixins provide dev tools and'
                    f' configuration from targets in "{Path.mixin().relative_to(Path.repo())}".'
                    f' Default: "{mixin_default}"'
                )
            )
            platform_default = 'linux/amd64'
            sub_parser.add_argument(
                '--platform',
                default=platform_default,
                choices=['linux/amd64', 'linux/arm64'],
                help=f'Platform to build upon. Default: "{platform_default}"'
            )
            sub_parser.add_argument(
                '--no-gdev-cache',
                action='store_true',
                help=f'Reset gdev cache.'
            )
            sub_parser.add_argument(
                '--tainted',
                action='store_true',
                help=(
                    'Allow uncommitted changes while building images. Images built this way are'
                    ' not reproducible and are thus tainted.'
                )
            )

        autocomplete(parser)

        parsed_args = parser.parse_args(args).__dict__
        if not parsed_args:
            sub_parser.print_help()
            import sys
            sys.exit(1)

        if parsed_args['target'] == '.':
            parsed_args['target'] = f'{Path.cwd().relative_to(Path.repo())}'

        no_gdev_cache = parsed_args.pop('no_gdev_cache')

        options = Options(**parsed_args)

        dependency = getattr(import_module(cmd_module), cmd_class)(options)

        if no_gdev_cache:
            from gdev.third_party.atools import memoize
            memoize.reset_all()

        return dependency

    @staticmethod
    def of_sys_argv() -> Dependency:
        return Dependency.of_args(tuple(sys.argv[1:]))
