from __future__ import annotations
from abc import ABC, abstractmethod
import os
from typing import Awaitable, Callable, Iterable, Union

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize
from .cfg import GenAbcCfg
from .dockerfile import GenAbcDockerfile


# We require buildkit support for inline caching of multi-stage dockerfiles. It's also way faster
# and the terminal output is relatively sane.
os.environ['DOCKER_BUILDKIT'] = '1'


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
    async def get_git_hash(self) -> str:
        git_hash = await Host.execute_and_get_line('git rev-parse HEAD')

        self.log.debug(f'{git_hash = }')

        return git_hash

    @memoize
    async def get_image_git_hash(self) -> str:
        if (line := await Host.execute_and_get_line(
                f'docker image inspect'
                ' --format="{{.Config.Labels.GitHash}}"'
                f' {await self.get_tag()}'
        )) == '"<no value>"':
            image_git_hash = ''
        else:
            image_git_hash = line.strip('"')

        self.log.debug(f'{image_git_hash = }')

        return image_git_hash

    @memoize
    async def get_image_sha(self) -> str:
        if lines := await Host.execute_and_get_lines(
            f'docker image ls -q --no-trunc {await self.get_tag()}'
        ):
            image_sha = next(iter(lines))
        else:
            image_sha = ''

        self.log.debug(f'{image_sha = }')

        return image_sha

    @memoize
    async def get_tag(self) -> str:
        name = (
            f'{await self.dockerfile.get_name()}'
            f':{await self.get_git_hash() if self.options.upload else "latest"}'
        )

        self.log.debug(f'{name = }')

        return name

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
    async def main(self) -> None:
        if (
                self.options.force
                or (not await self.get_image_sha())
                or (await self.get_git_hash() != await self.get_image_git_hash())
        ):
            await self.dockerfile.run()

            # TODO query remotely for cached build sources.
            self.log.info(f'Creating image "{await self.get_tag()}"')
            await Host.execute(
                f'docker build'
                f' -f {self.dockerfile.path}'
                f' -t {await self.get_tag()}'

                f' --label GitHash={await self.get_git_hash()}'

                # Keep metadata about layers so that they can be used as a cache source.
                f'{" --build-arg BUILDKIT_INLINE_CACHE=1" if self.options.upload else ""}'

                f' --platform {self.options.platform}'

                # Required to run production.
                f' --shm-size 1gb'

                f' {Path.repo()}'
            )
            await Host.execute(f'docker image prune -f')
