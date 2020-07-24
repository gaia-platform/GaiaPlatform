# PYTHON_ARGCOMPLETE_OK

from __future__ import annotations
from argparse import ArgumentParser, REMAINDER
from asyncio import gather
from dataclasses import dataclass
from importlib import import_module
from importlib.util import find_spec
from inspect import getdoc, isabstract, iscoroutinefunction
import logging
from pkgutil import iter_modules
import sys
from typing import FrozenSet, List, Sequence, Set, Tuple

from gdev.custom.pathlib import Path
from gdev.options import Options, Mount
from gdev.third_party.atools import memoize, memoize_db
from gdev.third_party.argcomplete import autocomplete, FilesCompleter


@dataclass(frozen=True)
class _ParserStructure:
    command_parts: Tuple[str, ...]
    doc: str
    sub_parser_structures: FrozenSet[_ParserStructure] = frozenset()

    def get_command_class(self) -> str:
        return ''.join([
            command_part.capitalize()
            for command_part in self.command_parts
            for command_part in command_part.split('_')
            if command_part
        ])

    def get_command_module(self) -> str:
        return '.'.join(['gdev.cmd', *self.command_parts])

    @classmethod
    def of_command_parts(cls, command_parts: Tuple[str, ...]) -> _ParserStructure:
        module_name = '.'.join(['gdev.cmd', *command_parts])
        spec = find_spec(module_name)
        module = import_module(module_name)
        if spec.submodule_search_locations is None:
            command_class = ''.join([
                command_part.capitalize()
                for command_part in command_parts
                for command_part in command_part.split('_')
                if command_part
            ])
            doc = getdoc(module.__dict__[command_class]) or ''
            parser_structure = _ParserStructure(command_parts=command_parts, doc=doc)
        else:
            doc = getdoc(module)
            sub_parser_structures: Set[_ParserStructure] = set()
            for module in iter_modules(spec.submodule_search_locations):
                if not (sub_command := module.name).startswith('_'):
                    sub_parser_structures.add(
                        cls.of_command_parts(tuple([*command_parts, sub_command]))
                    )
            parser_structure = _ParserStructure(
                command_parts=command_parts,
                doc=doc,
                sub_parser_structures=frozenset(sub_parser_structures)
            )

        return parser_structure


@dataclass(frozen=True)
class Dependency:
    # The gdev CLI uses docstrings in the help output. This is what they'll see if a subclass does
    # not provide a docstring.

    options: Options

    class Exception(Exception):
        pass

    class Abort(Exception):
        def __str__(self) -> str:
            return f'Abort: {super().__str__()}'

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

    @staticmethod
    @memoize_db(keygen=lambda: Path.cwd())
    def get_sticky_mixins() -> List[str]:
        return ['sudo']

    @staticmethod
    @memoize_db(keygen=lambda: Path.cwd())
    def get_sticky_mounts() -> List[str]:
        return ['build.gdev:.']

    @staticmethod
    @memoize_db(size=1)
    def get_parser_structure() -> _ParserStructure:
        return _ParserStructure.of_command_parts(tuple())

    @staticmethod
    def get_parser() -> ArgumentParser:
        def add_flags(parser: ArgumentParser) -> None:
            parser.add_argument(
                'args',
                nargs=REMAINDER,
                help=f'Args to be forwarded on to docker run, if applicable.'
            )
            base_image_default = 'ubuntu:20.04'
            parser.add_argument(
                '--base-image',
                default=base_image_default,
                help=f'Base image for build. Default: "{base_image_default}"'
            )
            log_level_default = 'INFO'
            parser.add_argument(
                '--log-level',
                default=log_level_default,
                choices=[name for _, name in sorted(logging._levelToName.items())],
                help=f'Log level. Default: "{log_level_default}"'
            )
            parser.add_argument(
                '-f', '--force',
                action='store_true',
                help='Force Docker to build with local changes.'
            )

            sticky_mixins = Dependency.get_sticky_mixins()
            parser.add_argument(
                '--sticky-mixins',
                default=sticky_mixins,
                dest='mixins',
                nargs='*',
                choices=sorted([
                    directory.name
                    for directory in Path.mixin().iterdir()
                    if directory.is_dir()
                ]),
                help=(
                    f'This flag is sticky! Any value you provide here will persist for all future'
                    f' invocations of gdev in the current working directory until you explicitly'
                    f' provide a new sticky value. Image mixins to use when creating a container.'
                    f' Mixins provide dev tools and configuration from targets in the'
                    f' "{Path.mixin().relative_to(Path.repo())}" directory. Currently:'
                    f' "{" ".join(sticky_mixins)}"'
                )
            )
            sticky_mounts = Dependency.get_sticky_mounts()
            parser.add_argument(
                '--sticky-mounts',
                default=sticky_mounts,
                dest='mounts',
                nargs='*',
                help=(
                    f'This flag is sticky! Any value you provide here will persist for all future'
                    f' invocations of gdev in the current working directory until you explicitly'
                    f' provide a new sticky value. <host_path>:<container_path> mounts to be'
                    f' created (or if already created, resumed) during `docker run`. Paths may be'
                    f' specified as relative paths. <host_path> relative paths are relative to the'
                    f' host\'s current working directory. <container_path> relative paths are'
                    f' relative to the Docker container\'s WORKDIR (AKA the build dir). Currently:'
                    f' "{" ".join(sticky_mounts)}"'
                )
            )
            platform_default = 'linux/amd64'
            parser.add_argument(
                '--platform',
                default=platform_default,
                choices=['linux/amd64', 'linux/arm64'],
                help=f'Platform to build upon. Default: "{platform_default}"'
            )
            parser.add_argument(
                '--upload',
                action='store_true',
                help='Upload image to Gaia docker image repository after building.'
            )

        def inner(parser: ArgumentParser, parser_structure: _ParserStructure) -> ArgumentParser:
            if not parser_structure.sub_parser_structures:
                add_flags(parser)
                parser.set_defaults(
                    command_class=parser_structure.get_command_class(),
                    command_module=parser_structure.get_command_module(),
                )
            else:
                sub_parsers = parser.add_subparsers()
                for sub_parser_structure in parser_structure.sub_parser_structures:
                    sub_parser = sub_parsers.add_parser(sub_parser_structure.command_parts[-1])
                    inner(sub_parser, sub_parser_structure)
            parser.description = parser_structure.doc

            return parser

        return inner(
            ArgumentParser(prog='gdev'), parser_structure=Dependency.get_parser_structure()
        )

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

        parser = Dependency.get_parser()
        autocomplete(parser, default_completer=FilesCompleter(allowednames='', directories=False))

        parsed_args = parser.parse_args(args).__dict__
        if not parsed_args:
            Dependency.get_parser_structure.memoize.remove()
            parser.parse_args([*args, '--help'])
            import sys
            sys.exit(1)

        Dependency.get_sticky_mixins.memoize.upsert()(parsed_args['mixins'])
        Dependency.get_sticky_mounts.memoize.upsert()(parsed_args['mounts'])

        parsed_args['args'] = ' '.join(parsed_args['args'])
        parsed_args['mixins'] = frozenset(parsed_args['mixins'])

        mounts = []
        for mount in parsed_args['mounts']:
            host_path, container_path = mount.split(':', 1)
            host_path, container_path = Path(host_path), Path(container_path)
            if not host_path.is_absolute():
                host_path = host_path.absolute()
            if not container_path.is_absolute():
                container_path = container_path.absolute().image_build()
            mounts.append(Mount(container_path=container_path, host_path=host_path))
        parsed_args['mounts'] = frozenset(mounts)

        command_class = parsed_args.pop('command_class')
        command_module = parsed_args.pop('command_module')

        options = Options(target=f'{Path.cwd().relative_to(Path.repo())}', **parsed_args)

        dependency = getattr(import_module(command_module), command_class)(options)

        return dependency

    @staticmethod
    def of_sys_argv() -> Dependency:
        """Return Dependency constructed by parsing args from sys.argv."""
        return Dependency.of_args(tuple(sys.argv[1:]))
