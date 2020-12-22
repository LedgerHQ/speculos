"""setup module."""

import pathlib
from distutils.spawn import find_executable
from setuptools.command.build_py import build_py as _build_py
from setuptools import find_packages, setup
import sys
import shutil


if sys.platform != "linux":
    raise OSError("Unsupported operating system!")


class CMakeBuild(_build_py):
    def run(self):
        if not find_executable("cmake"):
            raise Exception(
                "Can't find cmake! Try: `sudo apt-get install cmake`"
            )

        if not self.dry_run:
            cmake_dir = "bolos-emu-build"
            cwd_path = pathlib.Path().cwd()
            build_lib_path = pathlib.Path(self.build_lib)
            cmake_dir_path = cwd_path / cmake_dir
            cmake_dir_path.mkdir(parents=True, exist_ok=True)

            self.spawn([
                "cmake",
                f"-H.",
                f"-B{cmake_dir}",
                f"-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_TESTING=0",
                "-DWITH_VNC=1"
            ])
            self.spawn(["cmake", "--build", f"{cmake_dir}"])
            shutil.copytree(cmake_dir, build_lib_path / cmake_dir, dirs_exist_ok=True)

        super().run()


setup(
    name="speculos",
    version="1.0.0",
    url="https://github.com/LedgerHQ/speculos",
    python_requires=">=3.8.0",
    description="Ledger Blue and Nano S/X application emulator",
    long_description=pathlib.Path("README.md").read_text(),
    long_description_content_type="text/markdown",
    packages=find_packages(),
    py_modules=["speculos"],
    install_requires=[
        "jsonschema>=3.2.0,<4.0.0",
        "pyelftools>=0.27,<1.0",
        "mnemonic>=0.19,<1.0",
        "construct>=2.10.56,<3.0.0",
        "pyqt5>=5.15.2,<6.0.0"
    ],
    entry_points={
        "console_scripts": [
            "speculos = speculos:main"
        ],
    },
    cmdclass={
        "build_py": CMakeBuild
    }
)
