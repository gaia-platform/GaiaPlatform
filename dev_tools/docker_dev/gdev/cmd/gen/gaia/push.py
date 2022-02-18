from .build import GenGaiaBuild
from .._abc.push import GenAbcPush


class GenGaiaPush(GenAbcPush):

    @property
    def build(self) -> GenGaiaBuild:
        return GenGaiaBuild(self.options)
