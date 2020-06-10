from typing import Iterable

from gdev.third_party.atools import memoize
from .dockerfile import GenGitDockerfile
from .._abc.image import GenAbcImage


class GenGitImage(GenAbcImage):

    @property
    def dockerfile(self) -> GenGitDockerfile:
        return GenGitDockerfile(self.options)

    @memoize
    async def get_children(self) -> Iterable[GenAbcImage]:

        children = tuple()

        self.log.debug(f'{children = }')

        return children
