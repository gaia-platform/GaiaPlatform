"""
Module to represent the entire collection of options available from the command line.
"""
from dataclasses import dataclass
from typing import FrozenSet
from gdev.mount import Mount

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
