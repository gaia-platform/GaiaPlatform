from .cfg import GenEnvCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenEnvDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenEnvCfg:
        return GenEnvCfg(self.options)
