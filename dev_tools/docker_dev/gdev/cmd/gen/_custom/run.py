"""
Module to satisfy the run requirements to for a RUN section that includes MIXINs.
"""
from dataclasses import dataclass
import os

from gdev.third_party.atools import memoize
from .build import GenCustomBuild
from .._abc.run import GenAbcRun


@dataclass(frozen=True, repr=False)
class GenCustomRun(GenAbcRun):
    """
    Class to satisfy the run requirements to for a RUN section that includes MIXINs.
    """
    base_run: GenAbcRun

    @property
    def build(self) -> GenCustomBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenCustomBuild(options=self.options, base_build=self.base_run.build)

    @memoize
    def _get_flags(self) -> str:
        """
        Get the set of flags and arguments to apply to a `docker run` command.
        """

        flags_parts = [super()._get_flags()]
        # gdb needs ptrace enabled in order to attach to a process. Additionally,
        # seccomp=unconfined is recommended for gdb, and we don't run in a hostile environment.
        if {'clion', 'gdb', 'vscode'} & self.options.mixins:
            flags_parts += [
                '--cap-add=SYS_PTRACE',
                '--security-opt seccomp=unconfined'
            ]
        if {'clion', 'sudo', 'vscode'} & self.options.mixins:
            flags_parts.append(f'--user {os.getuid()}:{os.getgid()}')

        flags = ' '.join(flags_parts)

        return flags
