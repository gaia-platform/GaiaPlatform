from .build import GenEnvBuild
from .._abc.run import GenAbcRun


class GenPreBuildRun(GenAbcRun):

    @property
    def build(self) -> GenEnvBuild:
        return GenEnvBuild(self.options)
