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

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef USE_MPI
  #include <mpi.h>
#else
  // when not using MPI we use std::chrono for runtime
  #include <chrono>
#endif
#ifdef ENABLE_HDF5
  #include <hdf5.h>
#endif

#include "papi_utils.h"
#include "pyperfdump.h"

// This module changes and relies on current state
static enum {PD_NOTSTARTED, PD_LIBINIT, PD_INREGION, PD_INPROFILE}
             current_state = PD_NOTSTARTED;
// rank=0 and num_procs=1 for non-MPI usage
static int rank=0, num_procs=1;
// region_count is a counter for unnamed regions -> region_1, region_2, ...
static int region_count = 0;
// the filename for output, includes both the path and filename
static std::string filename;
// the current event set, setup from environment variables in init()
static PAPIEventSet* event_set=nullptr;
// the region name, used in the output
// generic_region_name is used when region is unnamed and deleted in end_region
static char *region_name, *generic_region_name=nullptr;
#ifdef USE_MPI
  // when MPI is enabled we will use an MPI function for the runtime
  static double t_start;
#else
  // when not using MPI we use std::chrono
  static std::chrono::time_point<std::chrono::high_resolution_clock> t_start;
#endif
static double runtime;
// buffer maps counter names to counter values
static std::unordered_map<std::string,unsigned long long> buffer;
// the dump function we use is a pointer, set in init()
static void (*dump)(const int, const int,
                    const char *const, const char *const,
                    const PAPIEventSet *const,
                    std::unordered_map<std::string,unsigned long long> &,
                    const double);

/***
update_counter_values - adds the values of counters of the event_set to buffer
***/
static void update_counter_values() {
  size_t i = 0;
  for (const auto &event : event_set->event_names()) {
    buffer[event] += event_set->values[i++];
  }
}

/***
PyPerfDump
***/
// an error object to raise exceptions
static PyObject *PyPerfDumpError;
// the module methods
static PyObject *method_init(PyObject *self,PyObject *args);
static PyObject *method_start_region(PyObject *self,PyObject *args);
static PyObject *method_start_profile(PyObject *self,PyObject *args);
static PyObject *method_end_profile(PyObject *self,PyObject *args);
static PyObject *method_end_region(PyObject *self, PyObject *args);
static PyObject *method_finalize(PyObject *self,PyObject *args);
static PyMethodDef pyperfdumpMethods[] = {
  { "init", method_init, METH_VARARGS,
    "Init PyPerfDump"},
  { "start_region", method_start_region, METH_VARARGS,
    "Start a region with an optional name"},
  { "start_profile", method_start_profile, METH_VARARGS,
    "Start counters from within a region"},
  { "end_profile", method_end_profile, METH_VARARGS,
    "End counter collection"},
  { "end_region", method_end_region, METH_VARARGS,
    "End a region"},
  { "finalize", method_finalize, METH_VARARGS,
    "Finalize PyPerfDump"},
  {NULL, NULL, 0, NULL}
};
static struct PyModuleDef pyperfdumpmodule = {
  PyModuleDef_HEAD_INIT,
  "pyperfdump",
#ifdef USE_MPI
  #ifdef ENABLE_HDF5
    "MPI-aware and parallel HDF5-enabled PyPerfDump for PAPI",
  #else
    "MPI-aware PyPerfDump for PAPI",
  #endif
#else
  #ifdef ENABLE_HDF5
    "HDF5-enabled PyPerfDump for PAPI",
  #else
    "PyPerfDump for PAPI",
  #endif
#endif
  -1,
  pyperfdumpMethods
};
PyMODINIT_FUNC PyInit_pyperfdump(void) {
  PyObject *m = PyModule_Create(&pyperfdumpmodule);
  if (m == NULL) return NULL;
  PyPerfDumpError = PyErr_NewException("pyperfdump.error", NULL, NULL);
  if (PyModule_AddObjectRef(m, "error", PyPerfDumpError) < 0) {
    Py_CLEAR(PyPerfDumpError);
    Py_DECREF(m);
    return NULL;
  }
  return m;
}

/***
break_state - adds the current state onto a (warning/error) message
              if iserror = true:  raise a PyPerfDumpError
              else:               print the warning message
***/
static PyObject *break_state(std::string msg, const bool iserror) {
  if (iserror) {
    msg = "PyPerfDump ERROR: " + msg;
  }
  else {
#ifdef SILENCE_WARNINGS
    Py_RETURN_NONE;
#else
    msg = "PyPerfDump WARNING: " + msg;
#endif
  }
  msg += "\nPyPerfDump STATE: ";
  switch(current_state) {
    case PD_NOTSTARTED:
      msg += "Not initialized";
      break;
    case PD_LIBINIT:
      msg += "Initialized, not within a region";
      break;
    case PD_INREGION:
      msg += "Initialized and within a region, not profiling";
      break;
    default: // case PD_INPROFILE:
      msg += "Initialized and profiling within a region";
      break;
  }
  if (iserror)
    PyErr_SetString(PyPerfDumpError, msg.c_str());
  else
    std::fprintf(stderr, "%s\n", msg.c_str());
  Py_RETURN_NONE;
}

/***
method_init - initialize PyPerfDump and PAPI
***/
static PyObject *method_init(PyObject *self,PyObject *args) {
  // warn and return if we are already initialized
  if (current_state != PD_NOTSTARTED)
    return break_state("Already initialized", false);
#ifdef USE_MPI
  // get rank and number of processes
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
#endif
  // initialize PAPI
  PAPI_library_init(PAPI_VER_CURRENT);
  event_set = (PAPIEventSet*) new PAPIEventSet();
  // char pointer for getenv
  char *env_str;
  // get counters by name with PDUMP_EVENTS
  if ((env_str = std::getenv("PDUMP_EVENTS")) && *env_str != '\0') {
    std::vector<std::string> env_event_names;
    // allow for either user-defined or comma separated list
    char *next = std::getenv("PDUMP_DELIMITER");
    const char separator = (next)? *next : ',';
    // while we found a delimiter, trim words from the front
    while (*env_str != '\0' && (next=strchr(env_str, separator))) {
      // set the delimiter to null
      *next = '\0';
      // add the next name
      env_event_names.push_back(std::string(env_str));
      // put the string start at the delimiter + 1
      env_str = next+1;
    }
    // we have exactly 1 name to add here or the list ended in a delimiter
    if (*env_str != '\0')
      env_event_names.push_back(std::string(env_str));
    // if we couldn't add any event from these names break and raise error
    if (event_set->add_from_names(env_event_names) == 0)
      return break_state("No valid event in PDUMP_EVENTS", true);
  }
  // or get counters by value with PDUMP_CODES
  else if ((env_str = std::getenv("PDUMP_CODES")) && *env_str != '\0') {
    std::vector<int> env_event_codes;
    // allow for either user-defined or comma separated list
    char *next = std::getenv("PDUMP_DELIMITER");
    const char separator = (next)? *next : ',';
    // while we found a delimiter, trim words from the front
    while (*env_str != '\0' && (next=strchr(env_str, separator))) {
      *next = '\0';
      // add the next code
      env_event_codes.push_back(atoi(env_str));
      // put the string start at delimiter + 1
      env_str = next+1;
    }
    // we have exactly 1 name to add here or the list ended in a delimiter
    if (*env_str != '\0')
      env_event_codes.push_back(atoi(env_str));
    // if we couldn't add any event from these codes break and raise error
    if (event_set->add_from_codes(env_event_codes) == 0)
      return break_state("No valid code in PDUMP_CODES", true);
  }
  // fail if no counters were given
  else
    return break_state("Neither PDUMP_EVENTS nor PDUMP_CODES is set", true);
  // setup our output filename, begin with the directory
  if ((env_str = std::getenv("PDUMP_DUMP_DIR")) && *env_str != '\0') {
    filename = std::string(env_str);
    // ensure a trailing path separator
    if (env_str[strlen(env_str)-1] != '/')
      filename += "/";
  }
  else
    filename = "./";
  if ((env_str = std::getenv("PDUMP_FILENAME")) && *env_str != '\0')
    filename += std::string(env_str);
  else
    filename += "perf_dump";
#ifdef ENABLE_HDF5
  // if we have HDF5 enabled, determine whether we should do csv or hdf5
  env_str = std::getenv("PDUMP_OUTPUT_FORMAT");
  if (env_str && (!strcmp(env_str, "CSV") || !strcmp(env_str, "csv"))) {
    dump = dumpcsv;
    filename += ".csv";
  }
  else {
    dump = dumphdf5;
  #ifdef USE_MPI
    // The number of processes affects the dimensionality of HDF5 files
    // use the number of ranks in the end of the filename to prevent issues
    filename += "." + std::to_string(num_procs) + ".h5";
  #else
    filename += ".h5";
  #endif
  }
#else
  // if HDF5 is not enabled then we will dump csv
  dump = dumpcsv;
  filename += ".csv";
#endif
  // move state to initialized and increment our reference count
  current_state = PD_LIBINIT;
  Py_INCREF(self);
  Py_RETURN_NONE;
}

static PyObject *method_start_region(PyObject *self,PyObject *args) {
  // we are either not initialized or are already within a region
  if (current_state != PD_LIBINIT) {
    return break_state("Cannot start a region here", false);
  }
  // increment region count every time regardless, used in generic_region_name
  ++region_count;
  if (!PyArg_ParseTuple(args,"|s",&region_name)) {
    generic_region_name = new char[32];
    region_name = generic_region_name;
    snprintf(region_name,32,"region_%d",region_count);
  }
  runtime = 0.0;
  current_state = PD_INREGION;
  Py_RETURN_NONE;
}

static PyObject *method_start_profile(PyObject *self,PyObject *args) {
  // we aren't initialized, not within a region, or are already profiling
  if (current_state != PD_INREGION) {
    return break_state("Cannot start profiling here", false);
  }
#ifdef USE_MPI
  t_start = MPI_Wtime();
#else
  t_start = std::chrono::high_resolution_clock::now();
#endif
  event_set->start();
  current_state = PD_INPROFILE;
  Py_RETURN_NONE;
}

static PyObject *method_end_profile(PyObject *self,PyObject *args) {
  // we aren't profiling
  if (current_state != PD_INPROFILE) {
    return break_state("No profile to end", false);
  }
#ifdef USE_MPI
  runtime += MPI_Wtime() - t_start;
#else
  runtime += std::chrono::duration<double, std::ratio<1,1>>
            (std::chrono::high_resolution_clock::now() - t_start).count();
#endif
  event_set->stop();
  update_counter_values();
  current_state = PD_INREGION;
  Py_RETURN_NONE;
}

static PyObject *method_end_region(PyObject *self, PyObject *args) {
  if (current_state != PD_INREGION) {
    // if we're actually profiling
    if (current_state == PD_INPROFILE) {
#ifdef DECREFNONE
      PyObject *none =
#endif
      break_state("Implicitly ending profile with call to end region", false);
#ifdef DECREFNONE
      Py_DECREF(none);
      none =
#endif
      method_end_profile(self, args);
#ifdef DECREFNONE
      Py_DECREF(none);
#endif
    }
    // otherwise we're not even in a region
    else return break_state("No region to end", false);
  }
  dump(rank, num_procs,
        filename.c_str(), region_name, event_set, buffer, runtime);
  buffer.clear();
  if (generic_region_name) {
    delete [] generic_region_name;
    generic_region_name = nullptr;
  }
  current_state = PD_LIBINIT;
  Py_RETURN_NONE;
}

static PyObject *method_finalize(PyObject *self,PyObject *args) {
  // We aren't in a state to finalize
  if (current_state != PD_LIBINIT) {
    // We aren't even initialized
    if (current_state == PD_NOTSTARTED) {
      return break_state("Can\'t finalize before initialization", false);
    }
#ifdef DECREFNONE
    PyObject *none =
#endif
    break_state("Finalize called out of order", false);
#ifdef DECREFNONE
    Py_DECREF(none);
#endif
    // stop profiling if we're profiling
    if (current_state == PD_INPROFILE) {
#ifdef DECREFNONE
      none =
#endif
      method_end_profile(self, args);
#ifdef DECREFNONE
      Py_DECREF(none);
#endif
    }
    // end the region that we must be in
#ifdef DECREFNONE
    none =
#endif
    method_end_region(self, args);
#ifdef DECREFNONE
    Py_DECREF(none);
#endif
    // we are now in the correct state
  }
  // put our state back to where we could do init() again
  delete event_set;
  event_set = nullptr;
  current_state = PD_NOTSTARTED;
  // let PAPI release resources and decrement our reference count
  PAPI_shutdown();
  Py_DECREF(self);
  Py_RETURN_NONE;
}
