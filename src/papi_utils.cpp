//////////////////////////////////////////////////////////////////////////////
// This file reduced and modified for PyPerfDump
// 2024 Chase Phelps
//////////////////////////////////////////////////////////////////////////////
// Copyright 2013-2024 Lawrence Livermore National Security, LLC and other
// perf-dump Developers. See the top-level LICENSE file for details.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-Exception
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

size_t PAPIEventSet::add_from_names(std::vector<std::string> &event_names) {
  const size_t num_counters = (size_t)PAPI_num_hwctrs();
  size_t count = 0;
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
      if (++count == num_counters)
        break;
    }
  }
  values = new long long[count];
  std::fill_n(values, count, 0);
  return count;
}

size_t PAPIEventSet::add_from_codes(std::vector<int> &event_codes) {
  const size_t num_counters = (size_t)PAPI_num_hwctrs();
  size_t count = 0;
  for (const auto &event : event_codes) {
    char name[PAPI_MAX_STR_LEN];
    const int papi_code = PAPI_event_code_to_name(event, name);
    if (papi_code != PAPI_OK) {
      std::fprintf(stderr,
                  "PAPI WARNING: Unable to get name from \"%d\", %s (%d)\n",
                  event, PAPI_strerror(papi_code), papi_code);
    }
    else if (PAPI_add_event(event_set_, event) == PAPI_OK) {
      event_names_.push_back(name);
      if (++count == num_counters)
        break;
    }
  }
  values = new long long[count];
  std::fill_n(values, count, 0);
  return count;
}
