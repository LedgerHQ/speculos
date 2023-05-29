from . import api  # noqa: F401
from . import client  # noqa: F401
from . import mcu  # noqa: F401

try:
    from speculos.__version__ import __version__  # noqa
except ImportError:
    __version__ = "unknown version"  # noqa
