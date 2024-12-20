from spack.package import *

class PyPerfdump(CMakePackage):
    """An MPI- and HDF5- enabled Python module to create PAPI dumps"""

    homepage = "https://github.com/RECUP-DOE/pyperfdump/"
    git = "https://github.com/RECUP-DOE/pyperfdump.git"
    url = "https://github.com/RECUP-DOE/pyperfdump/archive/refs/tags/v1.0.tar.gz"

    maintainers("chaseleif")

    license("GPL-3.0-or-later", checked_by="chaseleif")

    version(
        "1.0",
        sha256="ff7e27546d1e03fb932ed5489829878366f3f0b44ad32136fded94f07096dfe6",
    )

    variant("mpi", default=True, description="Use MPI")
    variant("hdf5", default=True, description="Enable HDF5 output")

    depends_on("cmake@3.15:", type="build")
    depends_on("cxx", type="build")
    depends_on("papi", type=("build", "link", "run"))
    depends_on("python@3:", type=("build", "link", "run"))

    depends_on("mpi", type=("build", "link", "run"), when="+mpi")
    depends_on("py-mpi4py", type="run", when="+mpi")

    depends_on("hdf5+mpi", type=("build", "link", "run"), when="+mpi+hdf5")
    depends_on("hdf5~mpi", type=("build", "link", "run"), when="~mpi+hdf5")

    def cmake_args(self):
        spec = self.spec
        args = [
            "-DCMAKE_INSTALL_RPATH={0}".format(spec.prefix.lib),
            "-DUSE_MPI:BOOL={0}".format("ON" if "+mpi" in spec else "OFF"),
            "-DENABLE_HDF5:BOOL={0}".format("ON" if "+hdf5" in spec else "OFF"),
            "-DPython_ROOT_DIR={0}".format(spec["python"].prefix),
            "-DPAPI_PREFIX={0}".format(spec["papi"].prefix),
        ]
        if "+mpi" in spec:
            args.append("-DMPI_HOME={0}".format(spec["mpi"].prefix))
        if "+hdf5" in spec:
            args.append("-DHDF5_ROOT={0}".format(spec["hdf5"].prefix))
        return args
