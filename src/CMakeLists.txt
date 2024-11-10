# Header files are in the include directory

# The sources for the shared library
add_library(pyperfdump SHARED papi_utils.cpp perf_dump.cpp dump_functions.cpp)

# Don't prepend lib to the output file, i.e., make it pyperfdump.so
set_target_properties(pyperfdump PROPERTIES PREFIX "")

# std= for compilation
set(CMAKE_CXX_STANDARD 11)

set(TARGET_INCLUDE_DIRS ${Python_INCLUDE_DIRS} ../include)
set(TARGET_LINK_LIBS ${PAPI_LIBRARIES})

if (USE_MPI)
  add_compile_definitions("USE_MPI")
  set(TARGET_INCLUDE_DIRS ${TARGET_INCLUDE_DIRS} ${MPI_CXX_INCLUDE_DIRS})
  set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} ${MPI_CXX_LIBRARIES})
endif()
if (ENABLE_HDF5)
  add_compile_definitions("ENABLE_HDF5")
  set(TARGET_INCLUDE_DIRS ${TARGET_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
  set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} ${HDF5_LIBRARIES})
endif()

# The include directories
target_include_directories(pyperfdump PRIVATE ${TARGET_INCLUDE_DIRS})

target_link_libraries(pyperfdump ${TARGET_LINK_LIBS})
