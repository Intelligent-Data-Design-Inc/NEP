# Copyright 2013-2024 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import os

from spack.package import *


class Nep(CMakePackage):
    """NEP (NetCDF Extension Pack) provides high-performance compression for
    HDF5/NetCDF-4 files with LZ4 and BZIP2 compression algorithms."""

    homepage = "https://github.com/Intelligent-Data-Design-Inc/NEP"
    url = "https://github.com/Intelligent-Data-Design-Inc/NEP/archive/v1.0.0.tar.gz"
    git = "https://github.com/Intelligent-Data-Design-Inc/NEP.git"

    maintainers("edhartnett")

    license("Apache-2.0")

    version("develop", branch="main")
    version("1.3.0", sha256="acf8f8a1360b521cef3f6f57fdeced64d2d78e765e8fd644ce435487fe74a003")
    version("1.0.0", sha256="c996da322c29e184a363ce013e5d44e773525e326a1d8022d69b5df46c032f54")

    variant("docs", default=False, description="Build documentation with Doxygen")
    variant("lz4", default=True, description="Enable LZ4 compression support")
    variant("bzip2", default=True, description="Enable BZIP2 compression support")

    depends_on("netcdf-c@4.9:~mpi", type=("build", "link"))
    depends_on("hdf5@1.12:+hl~mpi", type=("build", "link"))
    depends_on("lz4", when="+lz4", type=("build", "link"))
    depends_on("bzip2", when="+bzip2", type=("build", "link"))
    depends_on("doxygen", when="+docs", type="build")

    def cmake_args(self):
        args = [
            self.define_from_variant("BUILD_DOCUMENTATION", "docs"),
            self.define_from_variant("BUILD_LZ4", "lz4"),
            self.define_from_variant("BUILD_BZIP2", "bzip2"),
            self.define("BUILD_TESTING", self.run_tests),
            self.define("ENABLE_FORTRAN", False),
        ]
        return args

    def check(self):
        """Run install tests to verify libraries are installed."""
        with working_dir(self.build_directory):
            make("test")

    @run_after("install")
    def check_install(self):
        """Verify that plugin libraries are installed."""
        import glob

        # Candidate library directories (handle lib vs lib64 layouts)
        lib_dirs = []
        if os.path.isdir(self.prefix.lib):
            lib_dirs.append(self.prefix.lib)
        if os.path.isdir(self.prefix.lib64):
            lib_dirs.append(self.prefix.lib64)

        tty.info(f"NEP prefix: {self.prefix}")
        tty.info(f"Candidate library dirs for plugins: {lib_dirs}")

        # Locate the plugin directory under one of the lib dirs
        plugin_dir = None
        for lib_dir in lib_dirs:
            candidate = join_path(lib_dir, "plugin")
            if os.path.isdir(candidate):
                plugin_dir = candidate
                break

        if plugin_dir is None:
            # Log useful debugging information before failing
            try:
                prefix_contents = os.listdir(self.prefix)
            except OSError:
                prefix_contents = []
            tty.info(f"Top-level contents of prefix {self.prefix}: {prefix_contents}")

            for lib_dir in lib_dirs:
                try:
                    tty.info(f"Contents of {lib_dir}: {os.listdir(lib_dir)}")
                except OSError:
                    tty.info(f"Could not list contents of {lib_dir}")

            tried_paths = [join_path(d, "plugin") for d in lib_dirs]
            raise InstallError(
                "Plugin directory not found under any library directory. "
                f"Tried: {tried_paths}"
            )

        tty.info(f"Using plugin directory: {plugin_dir}")

        # List what is actually in the plugin directory
        plugins = glob.glob(join_path(plugin_dir, "*.so*"))
        tty.info(f"All plugin .so files found: {plugins}")

        def _require_plugin(patterns, desc):
            """Require that at least one file matching any pattern exists."""
            for pattern in patterns:
                matches = glob.glob(join_path(plugin_dir, pattern))
                if matches:
                    tty.info(f"{desc} plugin files matching '{pattern}': {matches}")
                    return
            raise InstallError(f"{desc} plugin not found in {plugin_dir}")

        # Verify expected plugins are present based on variants
        if self.spec.satisfies("+lz4"):
            _require_plugin(["libh5lz4.so", "libh5lz4.so.*"], "LZ4")

        if self.spec.satisfies("+bzip2"):
            _require_plugin(["libh5bzip2.so", "libh5bzip2.so.*"], "BZIP2")
