#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to create a Docker container from the image build with `gdev build`.
"""

from abc import ABC, abstractmethod
import os
import shlex
import sys

from gdev.custom.gaia_path import GaiaPath
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from gdev.host import Host
from gdev.cmd.gen._abc.build import GenAbcBuild


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
            "--rm",
            # Use a minimal init system to allow starting services.
            "--init",
            # Usually the default entrypoint, but override it to be certain.
            "--entrypoint /bin/bash",
            f"--hostname {self.build.dockerfile.get_name()}",
            f"--platform linux/{self.options.platform}",
            "--privileged",
            # Mount our current repo as /source/. Modifications to source in the container
            # are reflected on host.
            f" --volume {GaiaPath.repo()}:{GaiaPath.repo().image_source()}",
        ]
        # Handle non-TTY environments as well, e.g. TeamCity continuous integration.
        if sys.stdout.isatty():
            flags_parts.append("-it")
        # Ports to expose between container and host.
        ports = set(self.options.ports)
        if {"clion", "sshd", "vscode"} & self.options.mixins:
            ports.add("22")
            if (
                authorized_keys_path := GaiaPath.home() / ".ssh" / "authorized_keys"
            ).is_file():
                if {"clion", "sudo", "vscode"} & self.options.mixins:
                    flags_parts.append(
                        f"-v {authorized_keys_path.absolute()}:{authorized_keys_path.absolute()}"
                    )
                else:
                    flags_parts.append(
                        f"-v {authorized_keys_path.absolute()}:/root/.ssh/authorized_keys"
                    )
        if ports:
            flags_parts.append("-p " + " ".join(f"{port}:{port}" for port in ports))
        # Additional mounts to bind between container and host.
        for mount in self.options.mounts:
            flags_parts.append(
                f"--mount type=volume"
                f",dst={mount.container_path}"
                f",volume-driver=local"
                f",volume-opt=type=none"
                f",volume-opt=o=bind"
                f",volume-opt=device={mount.host_path}"
            )

        flags = " ".join(flags_parts)

        return flags

    @memoize
    def get_flags(self) -> str:
        """
        Get the set of flags and arguments to apply to a `docker run` command.
        """
        flags = self._get_flags()

        self.log.debug("flags = %s", flags)

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
                    self.log.info(
                        'Binding existing host path "%s" into container.',
                        mount.host_path,
                    )
                else:
                    mount.host_path.mkdir(parents=True)

        # execvpe the `docker run` command. It's drastically simpler than trying to manage it as a
        # Python subprocess.
        command_to_execute = (
            f"docker run {self.get_flags()}"
            f" {self.build.get_tag()}"
            f"""{fr' -c "{self.options.args}"' if self.options.args else ""}"""
        )
        if Host.is_drydock_enabled():
            print(f"[execvpe:{command_to_execute}]")
        else:
            command = shlex.split(command_to_execute)
            self.log.debug("execvpe command=%s", command)
            os.execvpe(command[0], command, os.environ)

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        if not self.options.mixins:
            run = self
        else:
            from gdev.cmd.gen._custom.run import GenCustomRun

            run = GenCustomRun(options=self.options, base_run=self)

        run.run()

    # pylint: enable=import-outside-toplevel
