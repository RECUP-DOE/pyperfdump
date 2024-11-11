# Header files are in the include directory

# The sources for the shared library
add_library(pyperfdump SHARED papi_utils.cpp perf_dump.cpp dump_functions.cpp)

# Don't prepend lib to the output file, i.e., make it pyperfdump.so
set_target_properties(pyperfdump PROPERTIES PREFIX "")

target_include_directories(pyperfdump PRIVATE ${TARGET_INCLUDE_DIRS})
#target_link_libraries(pyperfdump ${TARGET_LINK_LIBS})
target_link_options(pyperfdump PUBLIC ${TARGET_LINK_LIBS})

if (ENABLE_HDF5)
  target_link_libraries(pyperfdump ${HDF5_LIBRARIES})
endif()
