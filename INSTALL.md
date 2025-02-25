## Install pyperfdump
There are 2 methods available to install pyperfdump:
1) CMake
2) Spack
___
### CMake
This installation method requires external dependencies to be installed separately.

Required dependencies:
- CMake 3.15.0+
- Python 3+
- PAPI

Optional dependencies affect library usage and available output formats:
- MPI and mpi4py
  - Allows counter collection in distributed applications
  - When built with MPI support, Python scripts must use mpi4py
- HDF5
  - Allows counter output in HDF5 format
  - When combined with MPI, HDF5 must be parallel HDF5

Installation steps:
```bash
$ mkdir build && cd build/
$ cmake ../
$ make
$ make install
```

The `make install` command will use an active virtual environment's path.
Outside of a virtual environment, `make install` will install to Python root site-packages.
The installation prefix can alternatively be set, e.g., `-DCMAKE_INSTALL_PREFIX:PATH=/usr/lib`.

Library specific variables and default values:
```
USE_MPI:BOOL=OFF
ENABLE_HDF5:BOOL=OFF
// Silence warnings for out-of-order usage
SILENCE_WARNINGS:BOOL=OFF
```

Variables to explicitly set dependency paths:
```
Python_ROOT_DIR
PAPI_PREFIX
MPI_HOME
HDF5_ROOT
```
___
### Spack
This installation method will also install any required dependencies within the spack environment.

Spack provides an [installation guide](https://spack-tutorial.readthedocs.io/en/latest/tutorial_basics.html).

This package is in the current development branch (not the current latest tag: v0.23.1).

Basic spack installation directions:
```bash
$ . git/spack/share/spack/setup-env.sh
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
