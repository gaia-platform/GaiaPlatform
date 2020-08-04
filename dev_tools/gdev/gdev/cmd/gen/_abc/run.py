from abc import ABC, abstractmethod
from dataclasses import replace
import os
import shlex
import sys
from textwrap import dedent
from typing import FrozenSet, Iterable

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
    def mixin_builds(self) -> Iterable[GenAbcBuild]:
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

    @memoize
    async def get_image_mixins(self) -> FrozenSet[str]:
        if (line := await Host.execute_and_get_line(
                f'docker image inspect'
                ' --format="{{.Config.Labels.Mixins}}"'
                f' {await self.build.get_tag()}'
        )) == '"<no value>"':
            image_mixins = frozenset()
        else:
            image_mixins = eval(line.strip('"'))

        self.log.debug(f'{image_mixins = }')

        return image_mixins

    @memoize
    async def get_text(self) -> str:
        """Return text of mixin dockerfile to be written."""
        text_parts = [
            f'FROM {await mixin_build.get_tag()} as {await mixin_build.dockerfile.get_name()}'
            for mixin_build in self.mixin_builds
        ]
        text_parts.append(f'FROM {await self.build.get_tag()}')
        text_parts += [
            f'COPY --from={await mixin_build.dockerfile.get_name()} / /'
            for mixin_build in self.mixin_builds
        ]
        if 'sudo' in self.options.mixins:
            uid = os.getuid()
            gid = os.getgid()
            home = Path.home()
            login = os.getlogin()
            text_parts.append(dedent(fr'''
                RUN groupadd -r -o -g {gid} {login} \
                    && useradd {login} -l -r -o -u {uid} -g {gid} -G sudo \
                    && mkdir -p {home} \
                    && chown {login}:{login} {home} \
                    && echo "{login} ALL=(ALL:ALL) NOPASSWD: ALL" >> /etc/sudoers \
                    && touch {home}/.sudo_as_admin_successful \
                    && chown -R {login}:{login} {Path.repo().image_build()}
            ''').strip())
        text_parts += [
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
        for mixin_build in self.mixin_builds:
            await mixin_build.run()

        # See if we actually need to invoke the docker build command. If this is a tainted image, we
        # always need to let docker do the heavy lifting of checking whether anything in our repo
        # changed in a way that requires a rebuild. Otherwise, we just check whether the sha256
        # exists for the image tagged with our git hash. If it does, we can avoid calling docker
        # build and save a few precious seconds.
        if (
                self.options.force
                or (self.options.mixins != await self.get_image_mixins())
                or (await self.build.get_git_hash() != await self.build.get_image_git_hash())
        ):
            self.log.info(
                f'Mixing {sorted(self.options.mixins)} into {await self.build.get_tag()}.'
            )
            self.build.dockerfile.path.write_text(await self.get_text())
            await Host.execute_shell(
                f'docker build'
                f' --platform {self.options.platform}'
                f' --label GitHash={await self.build.get_git_hash()}'
                f' --label Mixins="{self.options.mixins}"'
                f' --tag {await self.build.get_tag()}'
                f' - < {self.build.dockerfile.path}'
            )
            await Host.execute(f'docker image prune -f')

            # The following execvpe can overwrite the last line of build output. This makes it look
            # more natural.
            print()

        uid, gid = os.getuid(), os.getgid()

        if self.options.mounts:
            for mount in self.options.mounts:
                if mount.host_path.exists():
                    self.log.info(f'Binding existing host path "{mount.host_path}" into container.')
                else:
                    mount.host_path.mkdir(parents=True)

        # execvpe the `docker run` command. It's drastically simpler than trying to manage it as a
        # Python subprocess.
        command = shlex.split(
            f'docker run'

            # Remove the container once we exit it.
            f' --rm'

            # The CLI needs to handle non-TTY places as well, e.g. TeamCity continuous integration.
            f"{' -it' if sys.stdout.isatty() else ''}"

            # Use a minimal init system to allow starting services.
            f' --init'

            # Usually the default entrypoint, but override it to be certain.
            f' --entrypoint /bin/bash'

            f' --hostname {await self.build.dockerfile.get_name()}'
            f' --platform {self.options.platform}'

            # We use shared memory in production. Just assume we'll always need this.
            f' --shm-size 1gb'

            # gdb needs ptrace enabled in order to attach to a process. Additionally,
            # seccomp=unconfined is recommended for gdb, and we don't run in a hostile environment.
            f'''{
                ' --cap-add=SYS_PTRACE --security-opt seccomp=unconfined'
                if "gdb" in self.options.mixins else ''
            }'''

            # Be ourselves while inside the container if sudo is available. This way, modifications
            # to files mounted from host don't change the owner to root.
            f"{f' --user {uid}:{gid}' if 'sudo' in self.options.mixins else ''}"

            # Additional mounts to bind between container and host.
            f'''{
                ''.join([
                    f' --mount type=volume'
                    f',dst={mount.container_path}'
                    f',volume-driver=local'
                    f',volume-opt=type=none'
                    f',volume-opt=o=bind'
                    f',volume-opt=device={mount.host_path}'
                    for mount in self.options.mounts])
            }'''

            # Mount our current repo as /source/. Modifications to source in the container
            # are reflected on host.
            f' --volume {Path.repo()}:{Path.repo().image_source()}'

            f' {await self.build.get_tag()}'
            f'''{
                fr' -c "{self.options.args}"' if self.options.args else ""
            }'''
        )
        self.log.debug(f'execvpe {command = }')

        os.execvpe(command[0], command, os.environ)
