from .dockerfile import GenGitDockerfile
from .._abc.build import GenAbcBuild


class GenGitBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenGitDockerfile:
        return GenGitDockerfile(self.options)
