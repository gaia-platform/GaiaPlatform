from __future__ import annotations
from abc import ABC, abstractmethod
import os
from typing import Awaitable, Callable, Iterable, Mapping, Union

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize
from .cfg import GenAbcCfg
from .dockerfile import GenAbcDockerfile


# We require buildkit support for inline caching of multi-stage dockerfiles. It's also way faster
# and the terminal output is relatively sane.
os.environ['DOCKER_BUILDKIT'] = '1'
os.environ['DOCKER_CLI_EXPERIMENTAL'] = 'enabled'


class GenAbcBuild(Dependency, ABC):
    """Build a Docker image from the rules in the `gdev.cfg` file in the current working directory.
    """

    @property
    def cfg(self) -> GenAbcCfg:
        return self.dockerfile.cfg

    @property
    @abstractmethod
    def dockerfile(self) -> GenAbcDockerfile:
        raise NotImplementedError

    async def _execute(
            self,
            *,
            command: str,
            err_ok: bool,
            fn: Callable[..., Awaitable[Union[None, Iterable[str], str]]]
    ) -> Union[None, Iterable[str], str]:
        return await fn(
            command=(
                f'docker run --rm'
                f' -v {Path.repo()}:{Path.repo().image_source()}'
                f' {await self.get_tag()}'
                f' {command}'
            ),
            err_ok=err_ok,
        )

    async def execute(self, command: str, *, err_ok=False) -> None:
        await self._execute(command=command, err_ok=err_ok, fn=Host.execute)

    async def execute_and_get_lines(self, command: str, *, err_ok=False) -> Iterable[str]:
        return await self._execute(command=command, err_ok=err_ok, fn=Host.execute_and_get_lines)

    async def execute_and_get_line(self, command: str, *, err_ok=False) -> str:
        return await self._execute(command=command, err_ok=err_ok, fn=Host.execute_and_get_line)

    @memoize
    async def get_actual_git_hash(self) -> str:
        actual_git_hash = await self._get_actual_label_value('GitHash')

        self.log.debug(f'{actual_git_hash = }')

        return actual_git_hash

    @memoize
    async def _get_actual_label_value(self, name: str) -> str:
        if (line := await Host.execute_and_get_line(
                f'docker image inspect'
                f' --format="{{{{.Config.Labels.{name}}}}}"'
                f' {await self.get_tag()}'
        )) == '"<no value>"':
            value = ''
        else:
            value = line.strip('"')

        return value

    @memoize
    async def _get_actual_label_value_by_name(self) -> Mapping[str, str]:
        return {'GitHash': await self._get_actual_label_value(name='GitHash')}

    @memoize
    async def get_actual_label_value_by_name(self) -> Mapping[str, str]:
        actual_label_value_by_name = await self._get_actual_label_value_by_name()

        self.log.debug(f'{actual_label_value_by_name = }')

        return actual_label_value_by_name

    @memoize
    async def get_base_build_names(self) -> Iterable[str]:
        seen_dockerfiles = set()

        async def inner(dockerfile: GenAbcDockerfile) -> Iterable[str]:
            build_names = []
            if dockerfile not in seen_dockerfiles:
                seen_dockerfiles.add(dockerfile)
                for input_dockerfile in await dockerfile.get_input_dockerfiles():
                    build_names += await inner(input_dockerfile)
                if await dockerfile.get_run_section() or dockerfile is self.dockerfile:
                    build_names.append(await dockerfile.get_name())
            return build_names

        base_build_names = tuple(await inner(self.dockerfile))

        self.log.debug(f'{base_build_names = }')

        return base_build_names

    @memoize
    async def get_sha(self) -> str:
        if lines := await Host.execute_and_get_lines(
                f'docker image ls -q --no-trunc {await self.get_tag()}'
        ):
            sha = next(iter(lines))
        else:
            sha = ''

        self.log.debug(f'{sha = }')

        return sha

    @memoize
    async def get_tag(self) -> str:
        tag = f'{await self.dockerfile.get_name()}:latest'

        self.log.debug(f'{tag = }')

        return tag

    @memoize
    async def get_uncommitted(self) -> str:
        seen_dockerfiles = set()

        async def inner(dockerfile: GenAbcDockerfile) -> Iterable[Path]:
            paths = set()
            if dockerfile not in seen_dockerfiles:
                seen_dockerfiles.add(dockerfile)
                paths.add( Path.repo() / Path(dockerfile.options.target))
                for input_dockerfile in await dockerfile.get_input_dockerfiles():
                    paths |= await inner(input_dockerfile)
            return paths

        uncommitted = '\n'.join(
            await Host.execute_and_get_lines(
                f'git status --short ' + ' '.join(
                    f'{path.parent}' for path in await inner(self.dockerfile)
                )
            )
        )

        self.log.debug(f'{uncommitted = }')

        return uncommitted

    @memoize
    async def get_wanted_git_hash(self) -> str:
        wanted_git_hash = await Host.execute_and_get_line('git rev-parse HEAD')

        self.log.debug(f'{wanted_git_hash = }')

        return wanted_git_hash

    async def _get_wanted_label_value_by_name(self) -> Mapping[str, str]:
        return {'GitHash': await self.get_wanted_git_hash()}

    @memoize
    async def get_wanted_label_value_by_name(self) -> Mapping[str, str]:
        wanted_label_value_by_name = await self._get_wanted_label_value_by_name()

        self.log.debug(f'{wanted_label_value_by_name = }')

        return wanted_label_value_by_name

    @memoize
    async def main(self) -> None:
        await self.dockerfile.run()

        # TODO query remotely for cached build sources.
        self.log.info(f'Creating image "{await self.get_tag()}"')
        await Host.execute(
            f'docker buildx build'
            f' -f {self.dockerfile.path}'
            f' -t {await self.get_tag()}'

            f'''{''.join([
                f' --label {name}="{value}"'
                for name, value
                in (await self.get_wanted_label_value_by_name()).items()
            ])}'''

            # Keep metadata about layers so that they can be used as a cache source.
            f' --build-arg BUILDKIT_INLINE_CACHE=1'

            f' --platform linux/{self.options.platform}'

            # Required to run production.
            f' --shm-size 1gb'

            # Allow cloning repos with ssh.
            f' --ssh default'

            f''' --cache-from {','.join([
                f'{self.options.registry}/{base_build_name}:latest'
                for base_build_name in await self.get_base_build_names()
            ])}'''

            f' {Path.repo()}'
        )
        await Host.execute(f'docker image prune -f')

    @memoize
    async def cli_entrypoint(self) -> None:
        if not self.options.mixins:
            build = self
        else:
            from .._custom.build import GenCustomBuild
            build = GenCustomBuild(options=self.options, base_build=self)

        await build.run()
