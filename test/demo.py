#! /usr/bin/env python3

# The test can run and succeed both with and without MPI
try:
  import mpi4py
  # We could leave initialize and finalize as True here
  # With these set to False, we must manually initialize() and finalize() MPI
  mpi4py.rc.initialize = False
  mpi4py.rc.finalize = False
  # Otherwise, initialize and finalize would be handled just by importing MPI
  from mpi4py import MPI
except Exception as e:
  MPI = None

import pyperfdump, sys

# debugmsg can be swapped to optionally silence script messages
#debugmsg = lambda *args, **kwargs: None
debugmsg = lambda *args, **kwargs: print(*args, **kwargs)

# A small loop that will take a small amount of time
def run():
  for i in range(1000):
    for x in range(1000):
      i+=123

if __name__=='__main__':
  if MPI is not None:
    debugmsg('Calling MPI.Init()',flush=True)
    MPI.Init()
    comm = MPI.COMM_WORLD
    mpirank = comm.Get_rank()
    mpisize = comm.Get_size()
    debugmsg(f'{mpirank=}, {mpisize=}',flush=True)
  debugmsg('Calling pyperfdump.init()',flush=True)
  pyperfdump.init()
  debugmsg('Calling pyperfdump.start_region()',flush=True)
  pyperfdump.start_region('testing_region')
  debugmsg('Calling pyperfdump.start_profile()',flush=True)
  pyperfdump.start_profile()
  debugmsg('Calling run()',flush=True)
  run()
  debugmsg('Calling pyperfdump.end_profile()',flush=True)
  pyperfdump.end_profile()
  debugmsg('Calling pyperfdump.end_region()',flush=True)
  pyperfdump.end_region()
  debugmsg('Calling pyperfdump.finalize()',flush=True)
  pyperfdump.finalize()
  if MPI is not None:
    debugmsg('Calling MPI.Finalize()',flush=True)
    MPI.Finalize()
  sys.exit(0)

