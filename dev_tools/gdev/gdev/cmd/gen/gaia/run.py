from .build import GenGaiaBuild
from .._abc.run import GenAbcRun


class GenGaiaRun(GenAbcRun):

    @property
    def build(self) -> GenGaiaBuild:
        return GenGaiaBuild(self.options)
