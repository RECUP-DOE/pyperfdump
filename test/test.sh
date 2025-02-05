#! /usr/bin/env bash

# pyperfdump requires papi
if ! which papi_avail >/dev/null 2>&1 ; then
  echo "PyPerfDump requires PAPI"
  exit 1
fi

# In order for a test to succeed, we need at least 1 valid counter
# To avoid arbitrarily selecting a counter that isn't available,
# we will just list all potential counters as "selected" counters
# a subset of valid counters will be automatically chosen
PAPI_EVENTS="$(papi_avail | grep -E "Yes[ ]+(No|Yes)" | awk '{print $1}')"
for event in $PAPI_EVENTS ; do
  [ -n "$PDUMP_EVENTS" ] && PDUMP_EVENTS=",$PDUMP_EVENTS"
  PDUMP_EVENTS="$event$PDUMP_EVENTS"
done
PAPI_EVENTS="$(papi_native_avail | grep "::" | awk '{print $2}')"
for event in $PAPI_EVENTS ; do
  [ -n "$PDUMP_EVENTS" ] && PDUMP_EVENTS=",$PDUMP_EVENTS"
  PDUMP_EVENTS="$event$PDUMP_EVENTS"
done

# If no counters are found, or none of these work, then PAPI won't work
if [ -z "$PDUMP_EVENTS" ] ; then
  echo "No possible PAPI counters found

(Possible issue with kernel paranoia setting)
https://www.kernel.org/doc/Documentation/sysctl/kernel.txt
perf_event_paranoid:

Controls use of the performance events system by unprivileged
users (without CAP_SYS_ADMIN).  The default value is 2.

 -1: Allow use of (almost) all events by all users
     Ignore mlock limit after perf_event_mlock_kb without CAP_IPC_LOCK
>=0: Disallow ftrace function tracepoint by users without CAP_SYS_ADMIN
     Disallow raw tracepoint access by users without CAP_SYS_ADMIN
>=1: Disallow CPU event access by users without CAP_SYS_ADMIN
>=2: Disallow kernel profiling by users without CAP_SYS_ADMIN
"
  echo "$ cat /proc/sys/kernel/perf_event_paranoid"
  cat /proc/sys/kernel/perf_event_paranoid
  exit 1
fi

# Optional dependencies for variant features
# The test.py script always tries to import mpi4py
# Use of HDF5 or PHDF5 depends on whether MPI is available
python -c 'import mpi4py' 2>/dev/null
havempi="$?"
[ "$havempi" -eq 0 ] && usempi="ON" || usempi="OFF"
# hdf5 check for the h5dump bin
which h5dump >/dev/null 2>&1
havehdf5="$?"
[ "$havehdf5" -eq 0 ] && enablehdf5="ON" || enablehdf5="OFF"

# fresh build directory
set -e
rm -rf build/ && mkdir build && cd build
cmake "-DCMAKE_INSTALL_PREFIX=$(pwd)/install" \
      "-DUSE_MPI:BOOL=$usempi" \
      "-DENABLE_HDF5:BOOL=$enablehdf5" \
      "../../"
make
make install
set +e

# the location built pyperfdump.so file
# (could just use pwd/src, doing an install will put it in python path)
LIBPATH="$(pwd)/install/lib"
LIBPATH="${LIBPATH}/$(ls "$LIBPATH")/site-packages"
PYTHONPATH="$LIBPATH:$PYTHONPATH"
export PYTHONPATH
# the path for the output dump file
PDUMP_DUMP_DIR="$(pwd)"
export PDUMP_DUMP_DIR
# the PAPI events we found earlier, any in the list that work can be chosen
export PDUMP_EVENTS
# set output format to hdf5, if we don't have hdf5 support it will create a csv
PDUMP_OUTPUT_FORMAT=hdf5
export PDUMP_OUTPUT_FORMAT

# run command and (possible) h5 output filename
if [ "$havempi" -eq 0 ] ; then
  cmd="mpiexec --oversubscribe -n 2 python3 ../test.py"
  h5file="perf_dump.2.h5"
else
  cmd="python3 ../test.py"
  h5file="perf_dump.h5"
fi
# same csv output filename for with/out mpi
csvfile="perf_dump.csv"

# run the test
$cmd

# we have an hdf5 output file
if [ "$havehdf5" -eq 0 ] ; then
  if [ ! -f "$h5file" ] ; then
    echo "Expected HDF5 output file not found"
  elif [ ! -s "$h5file" ] ; then
    echo "HDF5 output file is empty"
  elif ! h5dump "$h5file" | grep -q "Runtime" ; then
    echo "HDF5 file output doesn't contain runtime and should"
  else
    echo "HDF5 output appears correct"
  fi
  # Generate a csv output now
  export PDUMP_OUTPUT_FORMAT=csv
  $cmd
fi
if ! grep -q "Runtime" "$csvfile" ; then
  echo "csv output doesn't contain runtime and should"
  exit 1
else
  echo "csv output appears correct"
fi

echo "Test complete"
exit 0
