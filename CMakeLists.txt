cmake_minimum_required(VERSION 3.15.0)

# Project information
set(PROJECT_NAME PyPerfDump)
project(
  ${PROJECT_NAME}
  VERSION 0.1
  DESCRIPTION "Python Performance Dump module for PAPI")

# Set to build-type to release by default
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  endif()
endif()

# Append the cmake dir for FindPAPI
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Find the dependencies, PAPI and Python+development are hard requirements
set(Python_FIND_VIRTUALENV FIRST)
find_package(PAPI REQUIRED)
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# An option to silence the break_state function warnings
# these warnings are only encountered during improper API usage
option(SILENCE_WARNINGS OFF)

# The Py_None object was made immortal (no ref count) in 3.12
# Python < 3.12 increments the None counter in the macro Py_RETURN_NONE
# The return value is ignored sometimes (shouldn't be incremented)
# Let's set a flag to turn on a define to handle that here
if (Python_VERSION_MAJOR LESS 3 OR Python_VERSION_MINOR LESS 12)
  set(DECREFNONE ON)
endif()

# We will use MPI for parallel output, changes default behavior to use MPI
# MPI_Init and MPI_Finalize are not called, but other MPI functions will be
option(USE_MPI "Use MPI" ON)
if (USE_MPI)
  find_package(MPI)
  if (NOT MPI_CXX_FOUND)
    message("MPI was not found, disabling MPI")
    set(USE_MPI OFF)
  endif()
endif()

# We can create HDF5 output, will still be optional
# If also using MPI, this requires HDF5 to actually be PHDF5
option(ENABLE_HDF5 "Enable HDF5 output" ON)
if (ENABLE_HDF5)
  set(HDF5_USE_STATIC_LIBRARIES OFF)
  if (USE_MPI)
    set(HDF5_PREFER_PARALLEL ON)
  endif()
  find_package(HDF5)
  if (NOT HDF5_FOUND)
    message("HDF5 was not found, disabling HDF5")
    set(ENABLE_HDF5 OFF)
  elseif (USE_MPI AND NOT HDF5_IS_PARALLEL)
    message("MPI with HDF5 requires parallel HDF5, disabling HDF5")
    set(ENABLE_HDF5 OFF)
  elseif (NOT USE_MPI AND HDF5_IS_PARALLEL)
    message("Found parallel HDF5 which requires MPI, disabling HDF5")
    set(ENABLE_HDF5 OFF)
  endif()
endif()

configure_file(include/papi_utils.h.in
                ${CMAKE_CURRENT_SOURCE_DIR}/include/papi_utils.h)
configure_file(include/pyperfdump.h.in
                ${CMAKE_CURRENT_SOURCE_DIR}/include/pyperfdump.h)
configure_file(src/perf_dump.cpp.in
                ${CMAKE_CURRENT_SOURCE_DIR}/src/perf_dump.cpp)
configure_file(src/dump_functions.cpp.in
                ${CMAKE_CURRENT_SOURCE_DIR}/src/dump_functions.cpp)

# The source of the package
add_subdirectory(src)

install(TARGETS pyperfdump DESTINATION ${Python_SITELIB})
