from __future__ import annotations
from dataclasses import dataclass
import logging
from typing import FrozenSet

from gdev.custom.pathlib import Path

log = logging.getLogger(__name__)


@dataclass(frozen=True)
class Mount:
    container_path: Path
    host_path: Path


@dataclass(frozen=True)
class Options:
    args: str
    base_image: str
    cfg_enables: FrozenSet[str]
    force: bool
    log_level: str
    mixins: FrozenSet[str]
    mounts: FrozenSet[Mount]
    platform: str
    ports: FrozenSet[str]
    registry: str
    target: str
