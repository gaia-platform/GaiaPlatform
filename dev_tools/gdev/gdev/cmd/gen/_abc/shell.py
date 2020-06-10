from abc import ABC, abstractmethod
from asyncio import gather
from dataclasses import replace
import os
from textwrap import dedent

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize
from .image import GenAbcImage


class GenAbcShell(Dependency, ABC):

    @property
    @abstractmethod
    def image(self) -> GenAbcImage:
        raise NotImplementedError

    @property
    def mixin(self) -> GenAbcImage:
        from ..image import GenImage
        return GenImage(
            replace(
                self.options, target=f'{Path.mixin().relative_to(Path.repo()) / self.options.mixin}'
            )
        )

    @memoize
    async def get_name(self) -> str:
        name = f'{self.options.mixin}__{await self.image.dockerfile.get_name()}'

        self.log.debug(f'{name = }')

        return name

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
        tag = f'{await self.image.dockerfile.get_name()}:{await self.image.get_git_hash()}'

        self.log.debug(f'{tag = }')

        return tag

    @memoize
    async def main(self) -> None:
        await gather(*[self.image.run(), self.mixin.run()])

        if not await self.get_sha():
            await Host.execute_shell(
                dedent(fr"""
                    echo "
                    FROM --platform={self.options.platform} {await self.mixin.get_tag()} as mixin
                    FROM --platform={self.options.platform} {await self.image.get_tag()}
                    COPY --from=mixin / /
                    {await self.image.dockerfile.get_env_section()}
                    {await self.image.dockerfile.get_workdir_section()}
                    ENTRYPOINT [ "/bin/bash" ]
                    " | docker build --tag {await self.get_tag()} -
                """).strip()
            )

        os.execvpe(
            'docker',
            (
                f'docker run -it'

                # Usually the default entrypoint, but override it to be certain.
                f' --entrypoint /bin/bash'

                f' --hostname {await self.get_name()}'

                # We use shared memory in production. Just assume we'll always need this.
                f' --shm-size 1gb'

                # Be ourselves while inside the container. This way, modifications to files mounted
                # from host don't change the owner to root.
                #f' --user {os.getuid()}:{os.getgid()}'
                f' --volume /etc/group:/etc/group'
                f' --volume /etc/passwd:/etc/passwd'
                f' --volume /etc/shadow:/etc/shadow'

                # Mount our current repo as /source/. Modifications to source in the container
                # are reflected on host.
                f' --volume {Path.repo()}:{Path.repo().image_source()}'

                f' {await self.get_tag()}'
            ).split(),
            os.environ
        )
