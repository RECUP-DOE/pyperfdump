Python Performance Dump Module (PyPerfDump)
---

*PyPerfDump* is a tool for collecting counter data from one or multiple
processes and dumping counter values to a csv or HDF5 file with/out MPI.

Original source from [perf-dump](https://github.com/LLNL/perf-dump)
by Todd Gamblin with modification by Tanzima Islam.

Many changes were made to create this Python module.

Requirements
---
1) The Python development package, including, e.g., `Python.h`
2) PAPI (The Performance Application Programming Interface)

Module Variants
---
1) Without MPI and without HDF5
2) Without MPI and with HDF5
3) With MPI and without parallel HDF5
4) With MPI and with parallel HDF5

**NOTE:** *MPI variants additionally require mpi4py*

Building and Installation
---
To install within a virtualenv or conda environment,
ensure the environment is activated before running `cmake`.

The library can be built with CMake:
```bash
$ mkdir build && cd build/
$ cmake ..
$ make
```
The Python module can be located in ./build/src/.

Either manually add this file to a location within `PYTHONPATH`,
or complete installation:
```bash
$ make install
```

`make install` will move `pyperfdump.so` into the system (or environment)
Python site-library directory.

CMake Module Variant Options
---
CMake will attempt to automatically detect MPI and HDF5.
`USE_MPI` and `ENABLE_HDF5` are both disabled by default,
and can only be enabled if support is found for these features.

Configuration occurs in this order:
- If `USE_MPI` is true, and MPI is not found, `USE_MPI` will be false.
- If `ENABLE_HDF5` is true, and HDF5 is not found, `ENABLE_HDF5` will be false.
- If `ENABLE_HDF5` and `USE_MPI` are true and the HDF5 found is not parallel,
`ENABLE_HDF5` will be false.
- If `ENABLE_HDF5` is true, `USE_MPI` is false, and the HDF5 found is parallel,
`ENABLE_HDF5` will be false.

*A message will be printed if an option is disabled by the above conditions.*

Use of MPI changes multiple aspects within the module,
so the option is to `USE_MPI`.
Enabling HDF5 does not preclude generation of csv outputs,
so the option is to `ENABLE_HDF5`.

To enable options, modify the CMake command, e.g, `cmake -DUSE_MPI:BOOL=ON ..`

Additional CMake Variables
---
- `DECREFNONE` handles differing treatment of Python None objects
and is *automatically* configured.
- `SILENCE_WARNINGS` is by default false
and can be enabled to suppress warnings for non-fatal out-of-order module usage.

Build Issues
---
If CMake is unable to locate a requirement, e.g., HDF5, it will accept hints:
```bash
$ Python_ROOT_DIR=/path/to/python3
$ export Python_ROOT_DIR
$ PAPI_PREFIX=/path/to/papi
$ export PAPI_PREFIX
$ cmake -DMPI_HOME=/path/to/mpi -DHDF5_ROOT=/path/to/hdf5 ..
```

Usage
---
**PyPerfDump with MPI:** *mpi4py* **must** be used as
the *pyperfdump* module **requires**
`MPI_Init()` and `MPI_Finalize()` to be called elsewhere.

Within a Python script, use of the *pyperfdump* module
follows a pairwise nested usage:
```python3
#from mpi4py import MPI
import pyperfdump

# Call init to initialize the module
pyperfdump.init()

# Start a named region
pyperfdump.start_region('region1')

# Start counter collection
pyperfdump.start_profile()

# Execute the region of interest within a profile
interesting_work_function()

# End counter collection, appends counter values
pyperfdump.end_profile()

# End the current region, saves counter values
pyperfdump.end_region()

# Call finalize to free resources when finished
pyperfdump.finalize()
```
- Multiple regions can exist between `init` and `finalize`.
- Regions cannot overlap.
- If a name isn't provided to `start_region`, a generic region name is used.
- Multiple profiles can be performed between `start_region` and `end_region`.
- Profiles cannot overlap.
- With MPI *pyperfdump* methods should be called
**collectively** by all processes.

Environment Variables
---
*PyPerfDump* uses environment variables for runtime configuration:
- `PDUMP_DELIMITER`:
Specify the delimiter between PAPI counter names or codes, defaults to comma
- `PDUMP_EVENTS`:
A delimiter separated list of PAPI counters by name
- `PDUMP_CODES`:
A delimiter separated list of PAPI counters by numeric code
- `PDUMP_DUMP_DIR`:
The output directory for the dump file, defaults to `./`
- `PDUMP_FILENAME`:
The base filename for the dump file, defaults to `perf_dump`
- `PDUMP_OUTPUT_FORMAT`:
The format (csv or hdf5) for the dump, defaults to hdf5 if enabled

Output filenames are automatically given either `.csv` or `.h5` endings.
Additionally, when using MPI, HDF5 output filenames will include the number
of ranks, e.g., `.2.h5`, to prevent dimension-related issues.

Either `PDUMP_EVENTS` *or* `PDUMP_CODES`
**must** be set prior to calling `pyperfdump.init()`.
An exception will be raised if there no counters to collect.

A ***warning*** will be printed if a counter name or code cannot be used
(with the reason why).

An ***error*** will occur if no counters can be added.
