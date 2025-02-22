PyPerfDump with Spack
---
For information regarding installation and setup for Spack, visit:
[their installation tutorial](https://spack-tutorial.readthedocs.io/en/latest/tutorial_basics.html)

Base Usage
---
From this directory, with Spack activated, run:
```bash
$ spack repo add repo/
$ spack env create pyperfdump
$ spack env activate pyperfdump
$ spack add py-perfdump+mpi+hdf5
$ spack concretize
$ spack install
```

The Variants
---
This package has 2 variant options, resulting in 4 combinations:
- Using MPI with HDF5 enabled: `py-perfdump+mpi+hdf5`
- Using MPI without HDF5: `py-perfdump+mpi~hdf5`
- No MPI with HDF5 enabled: `py-perfdump~mpi+hdf5`
- No MPI and no HDF5: `py-perfdump~mpi~hdf5` (default)

More Verbose Setup Procedure Using an Environment
---
1) Clone this repo, and activate Spack if it is not activated:
```bash
$ git clone https://github.com/RECUP-DOE/pyperfdump.git
$ . "$SPACK_ROOT/share/spack/setup-env.sh"
```
(`SPACK_ROOT` refers to the root directory the Spack repo.)

2) Modify `env.yaml`, confirm the PyPerfDump variant and set the repo path:
```bash
$ sed -i "s|@pyperfdumpspack@|$(pwd)|" env.yaml
```

3) Create a new environment and install PyPerfDump
```bash
$ spack env create pyperfdump env.yaml
$ spack env activate pyperfdump
$ spack concretize
$ spack install
```

4) Test the environment
```bash
$ cd ../test
$ ./test.sh
```
- If you use an MPI variant, you should use mpiexec/mpirun.
- If HDF5 is enabled, set `PDUMP_OUTPUT_FORMAT` to csv for a csv output
