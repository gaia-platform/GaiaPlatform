from abc import ABC, abstractmethod
import os
import shlex
import sys
from typing import Iterable

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .build import GenAbcBuild


class GenAbcRun(Dependency, ABC):
    """Create a Docker container from the image build with `gdev build` and run a command in it."""

    @property
    @abstractmethod
    def build(self) -> GenAbcBuild:
        """The base build that needs to exist for us to run our container."""
        raise NotImplementedError

    @memoize
    async def _get_flags(self) -> str:
        flags_parts = [
            # Remove the container once we exit it.
            f'--rm',

            # Use a minimal init system to allow starting services.
            f'--init',

            # Usually the default entrypoint, but override it to be certain.
            f'--entrypoint /bin/bash',

            f'--hostname {await self.build.dockerfile.get_name()}',
            f'--platform {self.options.platform}',

            # We use shared memory in production. Just assume we'll always need this.
            f'--shm-size 1gb',

            # Mount our current repo as /source/. Modifications to source in the container
            # are reflected on host.
            f' --volume {Path.repo()}:{Path.repo().image_source()}'
        ]
        # Handle non-TTY environments as well, e.g. TeamCity continuous integration.
        if sys.stdout.isatty():
            flags_parts.append('-it')
        # Additional mounts to bind between container and host.
        for mount in self.options.mounts:
            flags_parts.append(
                f'--mount type=volume'
                f',dst={mount.container_path}'
                f',volume-driver=local'
                f',volume-opt=type=none'
                f',volume-opt=o=bind'
                f',volume-opt=device={mount.host_path}'
            )

        flags = ' '.join(flags_parts)

        return flags

    @memoize
    async def get_flags(self) -> str:
        flags = await self._get_flags()

        self.log.debug(f'{flags = }')

        return flags

    @memoize
    async def main(self) -> None:
        # Equivalent to calling `gdev build` before calling `gdev run`. This returns extremely fast
        # if nothing needs to be built, so it is worth always running here.
        await self.build.run()

        if self.options.mounts:
            for mount in self.options.mounts:
                if mount.host_path.exists():
                    self.log.info(f'Binding existing host path "{mount.host_path}" into container.')
                else:
                    mount.host_path.mkdir(parents=True)

        # execvpe the `docker run` command. It's drastically simpler than trying to manage it as a
        # Python subprocess.
        command = shlex.split(
            f'docker run {await self.get_flags()}'
            f' {await self.build.get_tag()}'
            f'''{
                fr' -c "{self.options.args}"' if self.options.args else ""
            }'''
        )
        self.log.debug(f'execvpe {command = }')

        os.execvpe(command[0], command, os.environ)
