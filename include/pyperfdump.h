//////////////////////////////////////////////////////////////////////////////
//  PyPerfDump, An MPI- and HDF5- enabled Python module to create PAPI dumps
//  Copyright (C) 2024, Chase Phelps, chaseleif@icloud.com
//                     Tanzima Islam, tanzima@txstate.edu
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#ifndef PYPERFDUMP_H_
#define PYPERFDUMP_H_

#include <unordered_map>
#include "papi_utils.h"

#ifdef USE_MPI
  #include <mpi.h>
  #define PD_ABORT MPI_Abort(MPI_COMM_WORLD, 1)
#else
  #define PD_ABORT std::exit(1)
#endif

#define PD_ASSERT(condition, fmt, ...)                            \
  do {                                                            \
    if (!(condition)) {                                           \
      std::fprintf(stderr, "%s:%d: PyPerfDump ERROR: " fmt "\n",  \
                    __FILE__, __LINE__, __VA_ARGS__);             \
      PD_ABORT;                                                   \
    }                                                             \
  }while(false)

void dumpcsv(const int rank, const int num_procs,
              const char *const filename,
              const char *const region_name,
              const PAPIEventSet *const event_set,
              std::unordered_map<std::string,unsigned long long> &buffer,
              const double runtime);
#ifdef ENABLE_HDF5
void dumphdf5(const int rank, const int num_procs,
              const char *const filename,
              const char *const region_name,
              const PAPIEventSet *const event_set,
              std::unordered_map<std::string,unsigned long long> &buffer,
              const double runtime);
#endif

#endif //PYPERFDUMP_H_
