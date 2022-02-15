from dataclasses import dataclass
from gdev.custom.pathlib import Path

@dataclass(frozen=True)
class Mount:
    """
    Class to represent a mount point between the docker container and the host system.
    """
    container_path: Path
    host_path: Path
