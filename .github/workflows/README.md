CI tests
---

General Requirements
---
- The build output, `pyperfdump.so`, needs to be in `PYTHONPATH` for testing
- If an optional requirement isn't available, e.g., MPI, it isn't necessary
to explicitly disable it, but disabling the option will reduce log size
- Any output file should include values for `Runtime`

Github-provided Ubuntu Runner's Specific Requirements
---
- Change value of /proc/sys/kernel/perf_event_paranoid to allow use of PAPI
- Use Python v3.10
  - The current python3-mpi4y package, installed by apt, is for v3.10
  - The install path for mpi4py must be explicitly added to `PYTHONPATH`
  - Using `pip` to install mpi4py is possible, but greatly increases test time
- `HDF5_root` must be set for CMake in the MPI+HDF5 test

Testing steps
---
- General setup
  - Set `perf_event_paranoid` to -1, disabling paranoia
  - Checkout the repo
  - Setup Python with v3.10
- Setup for non-MPI tests
  - Install `apt` devel packages for base, Python3, and PAPI
  - Install serial HDF5
- Build+Test for non-MPI with HDF5 disabled
  - Run CMake with the option to disable HDF5
  - Run the test
  - Verify a `.csv` file exists with `Runtime` and remove it
- Build+Test for non-MPI with HDF5 enabled
  - Run CMake with HDF5 enabled
  - Set `PDUMP_OUTPUT_FORMAT` to csv and run the test
  - Verify a `.csv` file exists with `Runtime` and remove it
  - Set `PDUMP_OUTPUT_FORMAT` to hdf5 and run the test
  - Verify a `.h5` file exists with `Runtime` and remove it
- Setup for MPI tests
  - Purge `apt` HDF5 packages
  - Install `apt` packages for MPI, HDF5+MPI, and mpi4py
- Build+Test for MPI with HDF5 disabled
  - Run CMake with the option to disable HDF5
  - Run the test with 2 processes
  - Verify a `.csv` file exists with `Runtime` and remove it
- Build+Test for MPI with HDF5 enabled
  - Run CMake
  - Set `PDUMP_OUTPUT_FORMAT` to csv and run the test with 2 processes
  - Verify a `.csv` file exists with `Runtime` and remove it
  - Set `PDUMP_OUTPUT_FORMAT` to hdf5 and run the test
  - Verify a `.h5` file exists with `Runtime` and remove it
