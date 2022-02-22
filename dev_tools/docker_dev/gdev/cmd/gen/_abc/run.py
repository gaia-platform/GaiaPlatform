from abc import ABC, abstractmethod
import os
import shlex
import sys

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .build import GenAbcBuild
from gdev.host import Host

class GenAbcRun(Dependency, ABC):
    """
    Create a Docker container from the image build with `gdev build` and run a command in it.
    """

    @property
    @abstractmethod
    def build(self) -> GenAbcBuild:
        """
        The base build that needs to exist for us to run our container.
        """
        raise NotImplementedError

    @memoize
    def _get_flags(self) -> str:
        """
        Get the set of flags and arguments to apply to a `docker run` command.
        """
        flags_parts = [
            # Remove the container once we exit it.
            f'--rm',

            # Use a minimal init system to allow starting services.
            f'--init',

            # Usually the default entrypoint, but override it to be certain.
            f'--entrypoint /bin/bash',

            f'--hostname {self.build.dockerfile.get_name()}',
            f'--platform linux/{self.options.platform}',
            f'--privileged',

            # Mount our current repo as /source/. Modifications to source in the container
            # are reflected on host.
            f' --volume {Path.repo()}:{Path.repo().image_source()}'
        ]
        # Handle non-TTY environments as well, e.g. TeamCity continuous integration.
        if sys.stdout.isatty():
            flags_parts.append('-it')
        # Ports to expose between container and host.
        ports = set(self.options.ports)
        if {'clion', 'sshd', 'vscode'} & self.options.mixins:
            ports.add('22')
            if (authorized_keys_path := Path.home() / '.ssh' / 'authorized_keys').is_file():
                if {'clion', 'sudo', 'vscode'} & self.options.mixins:
                    flags_parts.append(
                        f'-v {authorized_keys_path.absolute()}:{authorized_keys_path.absolute()}'
                    )
                else:
                    flags_parts.append(
                        f'-v {authorized_keys_path.absolute()}:/root/.ssh/authorized_keys'
                    )
        if ports:
            flags_parts.append('-p ' + ' '.join(f'{port}:{port}' for port in ports))
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
    def get_flags(self) -> str:
        """
        Get the set of flags and arguments to apply to a `docker run` command.
        """
        flags = self._get_flags()

        self.log.debug(f'{flags = }')

        return flags

    @memoize
    def main(self) -> None:
        """
        Main action to invoke for this class.
        """
        if (
                self.options.force
                or (not self.build.get_sha())
                or (
                    self.build.get_wanted_label_value_by_name()
                    != self.build.get_actual_label_value_by_name()
                )
        ):
            self.build.run()

        if self.options.mounts:
            for mount in self.options.mounts:
                if mount.host_path.exists():
                    self.log.info(f'Binding existing host path "{mount.host_path}" into container.')
                else:
                    mount.host_path.mkdir(parents=True)

        # execvpe the `docker run` command. It's drastically simpler than trying to manage it as a
        # Python subprocess.
        command_to_execute = (f'docker run {self.get_flags()}'
            f' {self.build.get_tag()}'
            f'''{fr' -c "{self.options.args}"' if self.options.args else ""}''')
        if Host.is_drydock_enabled():
            print(f"[execvpe:{command_to_execute}]")
        else:
            command = shlex.split(command_to_execute)
            self.log.debug(f'execvpe {command = }')
            os.execvpe(command[0], command, os.environ)

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        if not self.options.mixins:
            run = self
        else:
            from .._custom.run import GenCustomRun
            run = GenCustomRun(options=self.options, base_run=self)

        run.run()
