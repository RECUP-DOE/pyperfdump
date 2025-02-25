#! /usr/bin/env bash

echo "$0"

# pyperfdump requires papi
if ! which papi_avail >/dev/null 2>&1 ; then
  echo "PyPerfDump requires PAPI"
  exit 1
fi

# Optional dependencies for variant features
# The demo.py script always tries to import mpi4py
# Use of HDF5 or PHDF5 depends on whether MPI is available
if ! which mpiexec >/dev/null 2>&1 ; then
  echo "Command mpiexec not found, setting usempi=OFF"
  usempi="OFF"
elif mpiexec -n 1 python -c 'from mpi4py import MPI' 2>/dev/null ; then
  echo "MPI and mpi4py found, setting usempi=ON"
  usempi="ON"
else
  echo "MPI found but mpi4py not found, setting usempi=OFF"
  usempi="OFF"
fi
# hdf5 check for the h5dump bin
if which h5dump >/dev/null 2>&1 ; then
  echo "Command h5dump found, setting enablehdf5=ON"
  enablehdf5="ON"
else
  echo "Command h5dump not found, setting enablehdf5=OFF"
  enablehdf5="OFF"
fi

# fresh build directory
[ -d "build/" ] && rm -rf build/
mkdir build && cd build || exit 1
cmake "-DCMAKE_INSTALL_PREFIX=$(pwd)/install" \
      "-DUSE_MPI:BOOL=$usempi" \
      "-DENABLE_HDF5:BOOL=$enablehdf5" \
      "../../"
make
make install

