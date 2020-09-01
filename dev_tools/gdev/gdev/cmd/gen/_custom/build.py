from typing import Mapping

from gdev.third_party.atools import memoize
from .dockerfile import GenCustomDockerfile
from .._abc.build import GenAbcBuild


class GenCustomBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenCustomDockerfile:
        return GenCustomDockerfile(self.options)

    @memoize
    async def _get_wanted_label_value_by_name(self) -> Mapping[str, str]:
        return {
            **await super()._get_wanted_label_value_by_name(),
            'Mixins': f'{sorted(self.options.mixins)}'.replace(' ', '')
        }

    @memoize
    async def _get_actual_label_value_by_name(self) -> Mapping[str, str]:
        return {
            **await super()._get_actual_label_value_by_name(),
            'Mixins': await super()._get_actual_label_value('Mixins')
        }
