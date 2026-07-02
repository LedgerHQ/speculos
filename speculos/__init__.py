from . import (
    api,  # noqa: F401
    client,  # noqa: F401
    mcu,  # noqa: F401
)

try:
    from speculos.__version__ import __version__
except ImportError:
    __version__ = "unknown version"
