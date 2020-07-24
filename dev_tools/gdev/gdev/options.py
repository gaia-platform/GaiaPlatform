from __future__ import annotations
from dataclasses import dataclass
import logging
from typing import Tuple

log = logging.getLogger(__name__)


@dataclass(frozen=True)
class Options:
    args: str
    base_image: str
    log_level: str
    mixins: Tuple[str]
    platform: str
    tainted: bool
    target: str
    upload: bool
