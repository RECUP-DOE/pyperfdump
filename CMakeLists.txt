cmake_minimum_required(VERSION 3.15.0)

# Project information
set(PROJECT_NAME PyPerfDump)
project(
  ${PROJECT_NAME}
  VERSION 1.0
  DESCRIPTION "Python Performance Dump module for PAPI")

set(CMAKE_CXX_STANDARD 11)

# Set to build-type to release by default
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  endif()
endif()

# Append the cmake dir for FindPAPI
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Find the dependencies, PAPI and Python+development are hard requirements
find_package(PAPI REQUIRED)
set(TARGET_LINK_LIBS ${PAPI_LIBRARIES})

set(Python_FIND_VIRTUALENV FIRST)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
set(TARGET_INCLUDE_DIRS ${Python_INCLUDE_DIRS} ../include)

# An option to silence the break_state function warnings
# these warnings are only encountered during improper API usage
option(SILENCE_WARNINGS "Silence warnings for out-of-order usage" OFF)
if (SILENCE_WARNINGS)
  add_compile_definitions("SILENCE_WARNINGS")
endif()

# The Py_None object was made immortal (no ref count) in 3.12
# Python < 3.12 increments the None counter in the macro Py_RETURN_NONE
# The return value is ignored sometimes (shouldn't be incremented)
# Let's set a flag to turn on a define to handle that here
if (Python_VERSION_MAJOR LESS 3 OR Python_VERSION_MINOR LESS 12)
  add_compile_definitions("DECREFNONE")
endif()

# We will use MPI for parallel output, changes default behavior to use MPI
# MPI_Init and MPI_Finalize are not called, but other MPI functions will be
option(USE_MPI "Use MPI" OFF)
if (USE_MPI)
  find_package(MPI)
  if (NOT MPI_CXX_FOUND)
    message("MPI was not found, disabling MPI")
    set(USE_MPI OFF)
  else()
    add_compile_definitions("USE_MPI")
    set(TARGET_INCLUDE_DIRS ${TARGET_INCLUDE_DIRS} ${MPI_CXX_INCLUDE_DIRS})
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} ${MPI_CXX_LIBRARIES})
  endif()
endif()

# We can create HDF5 output, will still be optional
# If also using MPI, this requires HDF5 to actually be PHDF5
option(ENABLE_HDF5 "Enable HDF5 output" OFF)
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
  else()
    add_compile_definitions("ENABLE_HDF5")
    set(TARGET_INCLUDE_DIRS ${TARGET_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} ${HDF5_LIBRARIES})
  endif()
endif()

# The source of the package
add_subdirectory(src)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  #if (IS_DIRECTORY $ENV{PYTHONPATH})
  #  set(SITELIB $ENV{PYTHONPATH})
  set(SITELIB "${Python_SITELIB}")
else()
  set(SITELIB "${CMAKE_INSTALL_PREFIX}/lib/python")
  set(SITELIB "${SITELIB}${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
  set(SITELIB "${SITELIB}/site-packages/")
endif()
install(TARGETS pyperfdump DESTINATION ${SITELIB})
