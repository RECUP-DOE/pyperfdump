//////////////////////////////////////////////////////////////////////////////
// This file reduced and modified for PyPerfDump
// 2024 Chase Phelps
//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of perf-dump.
// Written by Todd Gamblin, tgamblin@llnl.gov, All rights reserved.
// LLNL-CODE-647187
//
// For details, see https://scalability-llnl.github.io/perf-dump
// Please also see the LICENSE file for our notice and the LGPL.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////

#ifndef PAPI_UTILS_H
#define PAPI_UTILS_H

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include "papi.h"

#ifdef USE_MPI
  #include <mpi.h>
  #define PAPI_ABORT MPI_Abort(MPI_COMM_WORLD, 1)
#else
  #define PAPI_ABORT std::exit(1)
#endif
// Use this on a PAPI call to automatically make sure its result is PAPI_OK
// If the call fails this will print the file and the line along with a useful
// abort message.
#define PAPI_CHECK(papi_call, fmt, ...)                   \
  do {                                                    \
    const int papi_code = papi_call;                      \
    if (papi_code != PAPI_OK) {                           \
      std::fprintf(stderr,                                \
                    "PAPI ERROR: %s (%d) at %s:%d\n",     \
                    PAPI_strerror(papi_code), papi_code,  \
                    __FILE__, __LINE__);                  \
      std::fprintf(stderr, fmt "\n", __VA_ARGS__);        \
      PAPI_ABORT;                                         \
    }                                                     \
  }while(false)

// Thin wrapper around PAPI's event set API.  Makes it easy to get some events
// from the environment and add them to an event set.  This handles all the
// name to code translation and keeps track of the particular events beng
// monitored.
class PAPIEventSet {
  public:
    // Create an empty event set.
    PAPIEventSet();

    // Free the underlying event set.
    ~PAPIEventSet();

    // Start the counters in this event set.
    void start() {
      PAPI_CHECK(PAPI_start(event_set_), "%s", "start()");
    }

    // Stop the counters in this event set and put their
    // values into this->values.
    void stop() {
      PAPI_CHECK(PAPI_stop(event_set_, values), "%s", "stop()");
    }

    // Number of events in this event set.
    size_t size() const {
      return event_names_.size();
    }

    // Read a list of event names/codes
    // Adds valid events to this PAPIEventSet
    // Number of events is limited to the number the architecture supports
    // Returns: The number of events that have been added.
    size_t add_from_names(std::vector<std::string> &event_names);
    size_t add_from_codes(std::vector<int> &event_codes);

    const std::vector<std::string>& event_names() const {
      return event_names_;
    }

    // Values of the events counted by this event set.
    // Valid after a call to stop().
    long long *values;

  private:

    // PAPI event set handle
    int event_set_;

    // Names of events added to the event set
    std::vector<std::string> event_names_;
};

#endif // PAPI_UTILS_H
