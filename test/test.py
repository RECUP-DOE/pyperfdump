#! /usr/bin/env python3

try:
  import mpi4py
  mpi4py.rc.initialize = False
  mpi4py.rc.finalize = False
  from mpi4py import MPI
except Exception as e:
  MPI = None

import pyperfdump, sys

debugmsg = lambda *args, **kwargs: None
#debugmsg = lambda *args, **kwargs: print(*args, **kwargs)

def run():
  for i in range(1000):
    for x in range(1000):
      i+=123

debugmsg('hallo',flush=True)
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

