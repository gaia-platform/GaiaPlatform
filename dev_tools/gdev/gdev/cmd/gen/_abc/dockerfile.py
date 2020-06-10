from __future__ import annotations
from abc import ABC, abstractmethod
from asyncio import gather
from dataclasses import replace
from typing import Iterable, List

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize
from .cfg import GenAbcCfg


class GenAbcDockerfile(Dependency, ABC):

    @property
    @abstractmethod
    def cfg(self) -> GenAbcCfg:
        raise NotImplementedError

    @memoize
    async def get_copy_section(self) -> str:
        copy_section_parts = []
        for input in await self.get_direct_inputs():
            copy_section_parts.append(f'COPY --from={await input.get_name()} / /')
        if await self.get_stage_name() == 'run':
            path = (await self.cfg.get_path()).parent
            copy_section_parts.append(f'COPY {path.context()} {path.image_source()}')

        copy_section = '\n'.join(copy_section_parts)

        self.log.debug(f'{copy_section = }')

        return copy_section

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcDockerfile]:
        direct_inputs = tuple()

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs

    @memoize
    async def get_env_section(self) -> str:
        from ..env.cfg import GenEnvCfg
        from ..gaia.cfg import GenGaiaCfg

        seen_cfgs = set()

        async def inner(cfg: GenEnvCfg) -> List[str]:
            if cfg in seen_cfgs:
                return []
            seen_cfgs.add(cfg)

            env_section_parts = []

            for line in await GenGaiaCfg(cfg.options).get_lines():
                env_section_parts += (
                    await inner(GenEnvCfg(replace(cfg.options, target=line)))
                )
            env_section_parts += await cfg.get_lines()

            return env_section_parts

        lines = await inner(GenEnvCfg(self.options))

        env_section = '\n'.join([f'ENV {line}' for line in lines])

        self.log.debug(f'{env_section = }')

        return env_section

    @memoize
    async def get_filename(self) -> str:
        filename_parts = []
        if stage_name := await self.get_stage_name():
            filename_parts.append(stage_name)
        if self.options.inline:
            filename_parts.append('inline')
        filename_parts += ['dockerfile', 'gdev']

        filename = '.'.join(filename_parts)

        self.log.debug(f'{filename = }')

        return filename

    @memoize
    async def get_from_section(self) -> str:
        from_section_parts = []
        if not self.options.inline:
            # We need to reference external images so we can copy contents.
            for direct_input in await self.get_direct_inputs():
                from_section_parts.append(
                    f'FROM {await direct_input.get_name()}'
                    f':$git_hash as {await direct_input.get_name()}'
                )
        from_section_parts.append(f'FROM $base_image as {await self.get_name()}')

        from_section = '\n'.join(from_section_parts)

        self.log.debug(f'{from_section = }')

        return from_section

    @memoize
    async def get_inputs(self) -> Iterable[GenAbcDockerfile]:
        if not self.options.inline:
            inputs = await self.get_direct_inputs()
        else:
            inputs = [
                *await self.get_transitive_inputs(),
                *await self.get_direct_inputs(),
            ]

        inputs = tuple(inputs)

        self.log.debug(f'{inputs = }')

        return inputs

    @memoize
    async def get_name(self) -> str:
        name = f'gaia_platform/{self.options.target}'
        if stage_name := await self.get_stage_name():
            name = f'{name}/{stage_name}'
        name = name.replace('/', '__').lower()

        self.log.debug(f'{name = }')

        return name

    @memoize
    async def get_path(self) -> Path:
        path = Path.repo() / '.gdev' / self.options.target / await self.get_filename()

        self.log.debug(f'{path = }')

        return path

    @memoize
    async def get_run_section(self) -> str:
        run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

    @memoize
    async def get_stage_name(self) -> str:
        stage_name = await self.cfg.get_section_name()

        self.log.debug(f'{stage_name = }')

        return stage_name

    @memoize
    async def get_sub_text(self) -> str:
        sub_text = '\n'.join(section for section in [
            await self.get_from_section(),
            await self.get_copy_section(),
            await self.get_env_section(),
            await self.get_workdir_section(),
            await self.get_run_section(),
            'ENTRYPOINT [ "/bin/bash" ]',
        ] if section)

        self.log.debug(f'{sub_text = }')

        return sub_text

    @memoize
    async def get_text(self) -> str:
        text_parts = [f'ARG base_image', f'ARG git_hash']
        if self.options.inline:
            for transitive_input in await self.get_transitive_inputs():
                text_parts += [
                    f'\n# {transitive_input}',
                    await transitive_input.get_sub_text(),
                ]
            for direct_input in await self.get_direct_inputs():
                text_parts += [
                    f'\n# {direct_input}',
                    await direct_input.get_sub_text(),
                ]
        text_parts += [
            f'\n# {self}',
            await self.get_sub_text(),
        ]

        text = '\n'.join(text_parts)

        self.log.debug(f'{text = }')

        return text

    @memoize
    async def get_transitive_inputs(self) -> Iterable[GenAbcDockerfile]:
        transitive_inputs = []
        for direct_input in await self.get_direct_inputs():
            transitive_inputs += await direct_input.get_inputs()

        transitive_inputs = tuple(transitive_inputs)

        self.log.debug(f'{transitive_inputs = }')

        return transitive_inputs

    @memoize
    async def get_workdir_section(self) -> str:
        workdir_section = f'WORKDIR {(await self.cfg.get_path()).parent.image_build()}'

        self.log.debug(f'{workdir_section = }')

        return workdir_section

    @memoize
    async def main(self) -> None:
        self.log.info(f'Creating dockerfile {await self.get_path()}')
        Host.write_text(data=await self.get_text(), path=await self.get_path())
