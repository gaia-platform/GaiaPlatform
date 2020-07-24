from abc import ABC, abstractmethod
from dataclasses import replace
import os
import sys
from textwrap import dedent
from typing import Iterable

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize
from .build import GenAbcBuild


class GenAbcRun(Dependency, ABC):
    """Create a Docker container from the image build with `gdev build` and run a command in it."""

    @property
    @abstractmethod
    def build(self) -> GenAbcBuild:
        """The base build that needs to exist for us to run our container."""
        raise NotImplementedError

    @property
    @memoize
    def mixins(self) -> Iterable[GenAbcBuild]:
        """Special [gaia] targets that we will mix into `self.build` before creating a container."""
        from ..run.build import GenRunBuild
        return [
            GenRunBuild(
                replace(
                    self.options,
                    target=f'{Path.mixin().relative_to(Path.repo()) / mixin}',
                    upload=False,
                )
            )
            for mixin in self.options.mixins
        ]

    @property
    @memoize
    def path(self) -> Path:
        """Return path where mixin dockerfile is to be written."""
        path = Path.repo() / '.gdev' / f'{".".join(self.options.mixins)}.dockerfile.gdev'

        self.log.debug(f'{path = }')

        return path

    @memoize
    async def get_name(self) -> str:
        """Return name portion of Docker image tag."""
        name = '__'.join([*self.options.mixins, await self.build.dockerfile.get_name()])
        assert len(name) < 64

        self.log.debug(f'{name = }')

        return name

    @memoize
    async def get_sha(self) -> str:
        """Return the sha256 of the image we need or '' if the image (and sha) don't exist."""
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
        """Return tag portion of Docker image tag."""
        tag = f'{await self.get_name()}:{await self.build.get_git_hash()}'

        self.log.debug(f'{tag = }')

        return tag

    @memoize
    async def get_text(self) -> str:
        """Return text of mixin dockerfile to be written."""
        text_parts = [
            f'FROM {await mixin.get_tag()} as {await mixin.dockerfile.get_name()}'
            for mixin in self.mixins
        ]
        text_parts.append(f'FROM {await self.build.get_tag()}')
        text_parts += [
            f'COPY --from={await mixin.dockerfile.get_name()} / /'
            for mixin in self.mixins
        ]
        uid = os.getuid()
        gid = os.getgid()
        home = Path.home()
        login = os.getlogin()
        text_parts += [
            dedent(fr'''
                RUN groupadd -r -g {gid} {login} || : \
                    && useradd {login} -l -r -u {uid} -g {gid} -G sudo || : \
                    && usermod -u {uid} {login} \
                    && usermod {login} -d {home} \
                    && mkdir -p {home} \
                    && chown {login}:{login} {home} \
                    && echo "{login} ALL=(ALL:ALL) NOPASSWD: ALL" >> /etc/sudoers \
                    && touch {home}/.sudo_as_admin_successful \
                    && chmod -R 777 {Path.repo().image_build()}
            ''').strip(),
            f'{await self.build.dockerfile.get_env_section()}',
            f'{await self.build.dockerfile.get_workdir_section()}',
            f'ENTRYPOINT [ "/bin/bash" ]'
        ]

        text = '\n'.join(text_parts)

        self.log.debug(f'{text = }')

        return text

    @memoize
    async def main(self) -> None:
        # Equivalent to calling `gdev build` before calling `gdev run`. This returns extremely fast
        # if nothing needs to be built, so it is worth always running here.
        await self.build.run()

        # Create all of the mixin images.
        for mixin in self.mixins:
            await mixin.run()

        # See if we actually need to invoke the docker build command. If this is a tainted image, we
        # always need to let docker do the heavy lifting of checking whether anything in our repo
        # changed in a way that requires a rebuild. Otherwise, we just check whether the sha256
        # exists for the image tagged with our git hash. If it does, we can avoid calling docker
        # build and save a few precious seconds.
        if self.options.tainted or (not await self.get_sha()):
            if self.mixins:
                self.log.info(f'Mixing {self.options.mixins} into {await self.build.get_tag()}.')
            self.path.write_text(await self.get_text())
            await Host.execute_shell(
                f'docker build'
                f' --platform {self.options.platform}'
                f' --tag {await self.get_tag()}'
                f' - < {self.path}'
            )

            # The following execvpe can overwrite the last line of build output. This makes it look
            # more natural.
            print()

        # execvpe the `docker run` command. It's drastically simpler than trying to manage it as a
        # Python subprocess.
        command = (
            f'docker run'

            # Remove the container once we exit it.
            f' --rm'

            # The CLI needs to handle non-TTY places as well, e.g. TeamCity continuous integration.
            f'{" -it" if sys.stdout.isatty() else ""}'

            # Use a minimal init system to allow starting services.
            f' --init'

            # Usually the default entrypoint, but override it to be certain.
            f' --entrypoint /bin/bash'

            f' --hostname {await self.get_name()}'
            f' --platform {self.options.platform}'

            # We use shared memory in production. Just assume we'll always need this.
            f' --shm-size 1gb'

            # Be ourselves while inside the container. This way, modifications to files mounted
            # from host don't change the owner to root.
            f' --user {os.getuid()}:{os.getgid()}'

            # Mount our current repo as /source/. Modifications to source in the container
            # are reflected on host.
            f' --volume {Path.repo()}:{Path.repo().image_source()}'

            f' {await self.get_tag()}'
            f'{" -c " + self.options.args if self.options.args else ""}'
        ).split()
        self.log.debug(f'execvpe {command = }')

        os.execvpe(command[0], command, os.environ)
