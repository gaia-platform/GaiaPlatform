from dataclasses import replace
import os
from textwrap import dedent
from typing import Iterable

from gdev.custom.pathlib import Path
from gdev.third_party.atools import memoize
from .cfg import GenCustomCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenCustomDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenCustomCfg:
        return GenCustomCfg(self.options)

    @memoize
    async def get_env_section(self) -> str:
        from ..pre_run.dockerfile import GenPreRunDockerfile

        env_section = await GenPreRunDockerfile(self.options).get_env_section()

        self.log.debug(f'{env_section = }')

        return env_section

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..run.dockerfile import GenRunDockerfile

        input_dockerfiles = [GenRunDockerfile(self.options)]
        for line in await self.cfg.get_lines():
            input_dockerfiles.append(GenRunDockerfile(replace(self.options, target=line)))
        input_dockerfiles = tuple(input_dockerfiles)

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles

    @memoize
    async def get_text(self) -> str:
        text_parts = [await super().get_text()]

        if {'clion', 'sudo', 'vscode'} & self.options.mixins:
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

        text = '\n'.join(text_parts)

        self.log.debug(f'{text = }')

        return text
