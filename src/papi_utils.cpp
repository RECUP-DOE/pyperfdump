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

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "papi_utils.h"

PAPIEventSet::PAPIEventSet(): event_set_(PAPI_NULL) {
  values = nullptr;
  event_set_ = PAPI_NULL;
  PAPI_CHECK(PAPI_create_eventset(&event_set_), "%s", "create_eventset()");
}

PAPIEventSet::~PAPIEventSet() {
  if (values) delete [] values;
  PAPI_CHECK(PAPI_cleanup_eventset(event_set_), "%s", "cleanup_eventset()");
  PAPI_CHECK(PAPI_destroy_eventset(&event_set_), "%s", "destroy_eventset()");
}

int PAPIEventSet::add_from_names(std::vector<std::string> &event_names) {
  const size_t num_counters = (size_t)PAPI_num_hwctrs();
  size_t i = 0;
  for (const auto &name : event_names) {
    int event;
    const int papi_code = PAPI_event_name_to_code(name.c_str(), &event);
    if (papi_code != PAPI_OK) {
      std::fprintf(stderr,
                  "PAPI WARNING: Unable to get code from \"%s\", %s (%d)\n",
                  name.c_str(), PAPI_strerror(papi_code), papi_code);
    }
    else if (PAPI_add_event(event_set_, event) == PAPI_OK) {
      event_names_.push_back(name);
      if (++i == num_counters)
        break;
    }
  }
  values = new long long[i];
  std::fill_n(values, i, 0);
  return event_names_.size();
}

int PAPIEventSet::add_from_codes(std::vector<int> &event_codes) {
  const size_t num_counters = (size_t)PAPI_num_hwctrs();
  size_t i = 0;
  for (const auto &event : event_codes) {
    char name[PAPI_MAX_STR_LEN];
    const int papi_code = PAPI_event_code_to_name(event, name);
    if (papi_code != PAPI_OK) {
      std::fprintf(stderr,
                  "PAPI WARNING: Unable to get name from \"%d\", %s (%d)\n",
                  event, PAPI_strerror(papi_code), papi_code);
    }
    else if (PAPI_add_event(event_set_, event) == PAPI_OK) {
      PAPI_event_code_to_name(event, name);
      event_names_.push_back(name);
      if (++i == num_counters)
        break;
    }
  }
  values = new long long[i];
  std::fill_n(values, i, 0);
  return event_names_.size();
}
