#! /usr/bin/env bash

echo "$0"

# pyperfdump library not found in PYTHONPATH
if ! python -c 'import pyperfdump' 2>/dev/null ; then
  # Check if we've built a test library already
  LIBPATH="$(find "build/install" -type f -name pyperfdump.so 2>/dev/null)"
  # Indicate whether testing module found and offer to {Reb,B}uild
  if [ -f "$LIBPATH" ] ; then
    echo "PyPerfDump module built for testing found"
    echo -n "Reb"
  else
    echo "PyPerfDump module not found"
    echo -n "B"
  fi
  # The default choice is to rebuild or build PyPerfDump for testing
  read -t 5 -p "uild PyPerfDump for testing (Y/n) " choice || echo ""
  # Choosing "No" will quit the test if the module not previously built
  if [[ "$choice" =~ [nN].* ]] ; then
    [ ! -f "$LIBPATH" ] && exit 1
  else
    ./build.sh
  fi
  # Set the location of the built module
  LIBPATH="$(pwd)/build/install/lib"
  LIBPATH="${LIBPATH}/$(ls "$LIBPATH")/site-packages"
  PYTHONPATH="$LIBPATH:$PYTHONPATH"
  export PYTHONPATH
fi
if ! python -c 'import pyperfdump' 2>/dev/null ; then
  # There is a problem with the path
  echo "$ ls $LIBPATH"
  ls "$LIBPATH"
  echo "PyPerfDump module still not found"
  exit 1
fi

# In order for a test to succeed, we need at least 1 valid counter
# To avoid arbitrarily selecting a counter that isn't available,
# we will just list all potential counters as "selected" counters
# a subset of valid counters will be automatically chosen
PAPI_EVENTS="$(papi_native_avail | grep "::" | awk '{print $2}')"
for event in $PAPI_EVENTS ; do
  [ -n "$PDUMP_EVENTS" ] && PDUMP_EVENTS=",$PDUMP_EVENTS"
  PDUMP_EVENTS="$event$PDUMP_EVENTS"
done
PAPI_EVENTS="$(papi_avail | grep -E "Yes[ ]+(No|Yes)" | awk '{print $1}')"
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

# hdf5 check for the h5dump bin
which h5dump >/dev/null 2>&1
havehdf5="$?"

# the path for the output dump file
PDUMP_DUMP_DIR="$(pwd)"
export PDUMP_DUMP_DIR
# the PAPI events we found earlier, any in the list that work can be chosen
export PDUMP_EVENTS
# set output format to hdf5, if we don't have hdf5 support it will create a csv
PDUMP_OUTPUT_FORMAT=hdf5
export PDUMP_OUTPUT_FORMAT

# run command and (possible) h5 output filename
if mpiexec -n 1 python -c 'from mpi4py import MPI' 2>/dev/null ; then
  echo "###"
  echo -n "# Running test with MPI"
  [ "$havehdf5" -eq 0 ] && echo " and PHDF5" || echo ""
  echo "###"
  cmd="mpiexec --oversubscribe -n 2 python3 demo.py"
  h5file="perf_dump.2.h5"
else
  echo "###"
  echo -n "# Running test without MPI"
  [ "$havehdf5" -eq 0 ] && echo " and with HDF5" || echo ""
  echo "###"
  cmd="python3 demo.py"
  h5file="perf_dump.h5"
fi
[ -f "$h5file" ] && rm "$h5file"
# same csv output filename for with/out mpi
csvfile="perf_dump.csv"
[ -f "$csvfile" ] && rm "$csvfile"

# run the test
echo "$cmd"
$cmd

# we have an hdf5 output file
if [ "$havehdf5" -eq 0 ] ; then
  if [ ! -f "$h5file" ] ; then
    echo "No HDF5 output file"
    havehdf5=1
  elif [ ! -s "$h5file" ] ; then
    echo "HDF5 output file is empty and shouldn't be"
  elif ! h5dump "$h5file" | grep -q "Runtime" ; then
    echo "HDF5 file output doesn't contain runtime and should"
  else
    echo "HDF5 output appears correct"
  fi
  if [ "$havehdf5" -eq 0 ] ; then
    # Generate a csv output now
    export PDUMP_OUTPUT_FORMAT=csv
    echo "$cmd"
    $cmd
  fi
fi
if [ ! -f "$csvfile" ] ; then
  echo "No csv output file"
  exit 1
elif [ ! -s "$csvfile" ] ; then
  echo "csv output file is empty and shouldn't be"
  exit 1
elif ! grep -q "Runtime" "$csvfile" ; then
  echo "csv output doesn't contain Runtime and should"
  exit 1
else
  echo "csv output appears correct"
fi

echo "Test successful"
exit 0
