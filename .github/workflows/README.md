CI tests
---

General
---
- The build output, `pyperfdump.so`, needs to be in `PYTHONPATH` for testing
- Any output file should include values for `Runtime`

Github-provided Ubuntu Runner
---
- Change value of /proc/sys/kernel/perf_event_paranoid to allow use of PAPI
- Use Python v3.10
  - The current python3-mpi4y package, installed by apt, is for v3.10
  - The install path for mpi4py must be explicitly added to `PYTHONPATH`
  - Using `pip` to install mpi4py is possible, but greatly increases test time
- `HDF5_ROOT` must be set for CMake in the MPI+HDF5 test for the OpenMPI HDF5

Testing steps
---
- General setup
  - Set `perf_event_paranoid` to -1, disabling paranoia
  - Checkout the repo
  - Setup Python with v3.10
- Setup for non-MPI tests
  - Install `apt` devel packages for base, Python3, and PAPI
  - Install serial HDF5
- Build+Test for non-MPI
  - The script, `test/test.sh` will test both HDF5 and csv output
- Setup for MPI tests
  - Purge `apt` HDF5 packages
  - Install `apt` packages for MPI, HDF5+MPI, and mpi4py
- Build+Test for MPI
  - The script, `test/test.sh` will test both HDF5 and csv output
