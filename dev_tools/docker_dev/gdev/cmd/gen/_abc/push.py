from abc import ABC, abstractmethod

from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize
from .build import GenAbcBuild


class GenAbcPush(Dependency, ABC):

    @property
    @abstractmethod
    def build(self) -> GenAbcBuild:
        raise NotImplemented

    @memoize
    async def main(self) -> None:
        await self.build.main()

        tag = await self.build.get_tag()
        await Host.execute(f'docker tag {tag} {self.options.registry}/{tag}')
        await Host.execute(f'docker push {self.options.registry}/{tag}')
