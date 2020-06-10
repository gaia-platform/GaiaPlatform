from __future__ import annotations
from dataclasses import dataclass
import logging

log = logging.getLogger(__name__)


@dataclass(frozen=True)
class Options:
    base_image: str
    inline: bool
    log_level: str
    mixin: str
    platform: str
    tainted: bool
    target: str
