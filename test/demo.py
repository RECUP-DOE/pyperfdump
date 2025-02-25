#! /usr/bin/env python3

# This demo can run and succeed both with and without MPI+mpi4py
try:
  from mpi4py import MPI
except ModuleNotFoundError:
  pass

import pyperfdump

# debugmsg can be swapped to optionally silence script messages
#debugmsg = lambda *args, **kwargs: None
debugmsg = lambda *args, **kwargs: print(*args, **kwargs)

# A small loop that will take a small amount of time
def run():
  for i in range(1000):
    for x in range(1000):
      i+=123

if __name__=='__main__':
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

