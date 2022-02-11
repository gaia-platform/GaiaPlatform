from .build import GenWebBuild
from .._abc.run import GenAbcRun


class GenWebRun(GenAbcRun):

    @property
    def build(self) -> GenWebBuild:
        return GenWebBuild(self.options)
