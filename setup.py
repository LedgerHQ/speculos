#!/usr/bin/env python3
"""Install Speculos"""
import pathlib
from distutils.spawn import find_executable
from setuptools.command.build_py import build_py as _build_py
from setuptools import find_packages, setup
import sys
import tempfile


class BuildSpeculos(_build_py):
    """
    Extend "setup.py build_py" to build Speculos launcher and VNC server using cmake.

    This command requires some system dependencies (ARM compiler, libvncserver
    headers...) which are documented on https://speculos.ledger.com/installation/build.html

    distutils documentation about extending the build command:
    https://docs.python.org/3.8/distutils/extending.html#integrating-new-commands
    """

    def run(self):
        if not find_executable("cmake"):
            raise RuntimeError("cmake is not found and is required to build Speculos")

        if not self.dry_run:
            pathlib.Path(self.build_lib).mkdir(parents=True, exist_ok=True)
            with tempfile.TemporaryDirectory(prefix="build-", dir=self.build_lib) as build_dir:
                self.spawn(
                    [
                        "cmake",
                        "-H.",
                        "-B" + build_dir,
                        "-DCMAKE_BUILD_TYPE=Release",
                        "-DBUILD_TESTING=0",
                        "-DWITH_VNC=1",
                    ]
                )
                self.spawn(["cmake", "--build", build_dir])

        super().run()


setup(
    name="speculos",
    author="Ledger",
    author_email="hello@ledger.fr",
    version="0.1.0",
    url="https://github.com/LedgerHQ/speculos",
    python_requires=">=3.6.0",
    description="Ledger Blue and Nano S/X application emulator",
    long_description=pathlib.Path("README.md").read_text(),
    long_description_content_type="text/markdown",
    packages=find_packages(),
    install_requires=[
        "construct>=2.10.56,<3.0.0",
        "flask>=2.0.0,<3.0.0",
        "flask-restful>=0.3.8,<1.0",
        "jsonschema>=3.2.0,<4.0.0",
        "mnemonic>=0.19,<1.0",
        "pillow>=8.0.0,<10.0.0",
        "pyelftools>=0.27,<1.0",
        "pyqt5>=5.15.2,<6.0.0",
        "requests>=2.25.1,<3.0.0",
    ]
    + (["dataclasses>=0.8,<0.9"] if sys.version_info <= (3, 6) else []),
    extras_require={
        'dev': [
            'pytest',
            'pytest-cov'
        ]},
    setup_requires=["wheel"],
    entry_points={
        "console_scripts": [
            "speculos = speculos.main:main",
        ],
    },
    cmdclass={
        "build_py": BuildSpeculos,
    },
    include_package_data=True,
)
