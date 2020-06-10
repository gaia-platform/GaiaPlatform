from __future__ import annotations
from abc import ABC, abstractmethod
from asyncio import gather
import os
from typing import Awaitable, Callable, Iterable, Union

from gdev.dependency import Dependency
from gdev.custom.pathlib import Path
from gdev.host import Host
from gdev.third_party.atools import memoize
from .cfg import GenAbcCfg
from .dockerfile import GenAbcDockerfile


os.environ['DOCKER_BUILDKIT'] = '1'
os.environ['BUILDKIT_PROGRESS'] = 'plain'


class GenAbcImage(Dependency, ABC):

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
    async def get_direct_inputs(self) -> Iterable[GenAbcImage]:
        inputs = tuple()

        self.log.debug(f'{inputs = }')

        return inputs

    @staticmethod
    @memoize
    async def _get_git_hash() -> str:
        return await Host.execute_and_get_line('git rev-parse HEAD')

    @memoize
    async def get_git_hash(self) -> str:
        git_hash = f'{"tainted" if self.options.tainted else await self._get_git_hash()}'

        self.log.debug(f'{git_hash = }')

        return git_hash

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

    @staticmethod
    @memoize
    async def _get_prev_hashes() -> Iterable[str]:
        # FIXME this will probably break if origin/master isn't a direct ancestor of HEAD
        return await Host.execute_and_get_lines('git rev-parse origin/master^..HEAD')

    @memoize
    async def get_prev_hashes(self) -> Iterable[str]:
        prev_hashes = await self._get_prev_hashes()

        self.log.debug(f'{prev_hashes = }')

        return prev_hashes

    @memoize
    async def get_tag(self) -> str:
        name = f'{await self.dockerfile.get_name()}:{await self.get_git_hash()}'

        self.log.debug(f'{name = }')

        return name

    @memoize
    async def get_uncommitted(self) -> str:
        uncommitted = '\n'.join(
            await Host.execute_and_get_lines(
                f'git status --short {(await self.cfg.get_path()).parent}'
            )
        )

        self.log.debug(f'{uncommitted = }')

        return uncommitted

    @memoize
    async def main(self) -> None:
        if not self.options.inline:
            await gather(*[direct_input.run() for direct_input in await self.get_direct_inputs()])

        if tainted := self.options.tainted or (not await self.get_sha()):
            if (uncommitted := await self.get_uncommitted()) and (not tainted):
                raise Exception(
                    f'Cannot build an untainted image with uncommitted files. Use "--tainted" if '
                    f'you would like to build a tainted image. {uncommitted = }'
                )

            await self.dockerfile.run()

            # TODO query remotely for candidate cache source images.
            self.log.info(f'Creating image "{await self.get_tag()}"')
            await Host.execute(
                f'docker build'
                f' -f {await self.dockerfile.get_path()}'
                f' -t {await self.get_tag()}'

                # Special args needed by generated dockerfiles.
                f' --build-arg base_image={self.options.base_image}'
                f' --build-arg git_hash={await self.get_git_hash()}'

                # Keep metadata about layers so that they can be used as a cache source.
                f' --build-arg BUILDKIT_INLINE_CACHE=1'

                f' --platform {self.options.platform}'

                # Required to run production.
                f' --shm-size 1gb'

                f' {Path.repo()}'
            )
