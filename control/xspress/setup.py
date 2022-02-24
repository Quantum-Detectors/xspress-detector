import os
import sys
from setuptools import find_packages, setup

# Place the directory containing _version_git on the path
for path, _, filenames in os.walk(os.path.dirname(os.path.abspath(__file__))):
    if "_version_git.py" in filenames:
        sys.path.append(path)
        break

from _version_git import __version__, get_cmdclass  # noqa


setup(
    cmdclass=get_cmdclass(),
    version=__version__,
)

