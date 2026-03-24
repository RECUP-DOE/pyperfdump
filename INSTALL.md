## Install pyperfdump
There are 3 methods available to install pyperfdump:
1) CMake
2) Meson
3) Spack
___
### Requirements

Required dependencies:
- Python 3+
- PAPI

Optional dependencies affect library usage and available output formats:
- MPI and mpi4py
  - Allows counter collection in distributed applications
  - When built with MPI support, Python scripts must use mpi4py
- HDF5
  - Allows counter output in HDF5 format
  - When combined with MPI, HDF5 must be parallel HDF5
___
### CMake

Installation steps:
```bash
$ mkdir build && cd build/
$ cmake ../
$ make
$ make install
```

The `make install` command will attempt to use an active virtual environment's path.
Outside of a virtual environment, `make install` will install to Python root site-packages.

Library specific variables and default values:
```
USE_MPI:BOOL=OFF
ENABLE_HDF5:BOOL=OFF
// Silence warnings for out-of-order usage
SILENCE_WARNINGS:BOOL=OFF
```

Example variables to explicitly set paths and options:
```bash
cmake \
  -DUSE_MPI:BOOL=ON \
  -DENABLE_HDF5:BOOL=ON \
  -DCMAKE_INSTALL_PREFIX:PATH="$VIRTUAL_ENV" \
  -DPAPI_PREFIX:PATH="$PAPI_DIR" \
  -DHDF5_ROOT:PATH="$PHDF5_DIR" \
  -DPython_ROOT_DIR:PATH="$PYTHON_DIR" \
  -DMPI_HOME:PATH="$MPI_ROOT" \
  -DMPIEXEC_EXECUTABLE:PATH="$(command -v mpiexec)" \
  ../
```
___
### Meson

Installation steps:
```bash
$ meson setup build && cd build/
$ meson compile
$ meson install
```

The `meson install` command will prefer a venv, similar to CMake.

Options can be passed to meson during the setup phase, e.g.,
```bash
$ meson setup -Duse_mpi=true -Denable_hdf5=true build
```
___
### Spack

Spack provides an [installation guide](https://spack-tutorial.readthedocs.io/en/latest/tutorial_basics.html).

Basic spack installation directions:
```bash
$ git clone --depth=2 --branch=releases/v1.1 https://github.com/spack/spack.git
$ cd spack/
$ . spack/share/spack/setup-env.sh
$ spack env create pyperfdump
$ spack env activate pyperfdump
$ spack add py-perfdump+mpi+hdf5
$ spack concretize
$ spack install
```

With the 2 variant options, there are 4 combinations:
- Using MPI with HDF5 enabled: `py-perfdump+mpi+hdf5`
- Using MPI without HDF5: `py-perfdump+mpi~hdf5`
- No MPI with HDF5 enabled: `py-perfdump~mpi+hdf5`
- No MPI without HDF5: `py-perfdump~mpi~hdf5` __(default)__
___
