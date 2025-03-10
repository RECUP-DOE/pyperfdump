project(
  'pyperfdump',
  'cpp',
  version: '1.1',
  meson_version: '>=1.6.1',
  default_options: ['buildtype=release',
                    'python.install_env=auto'],
)

project_description = 'Python Performance Dump module for PAPI'

pyperfdump_headers = ['papi_utils.h', 'pyperfdump.h']
pyperfdump_sources = ['src/dump_functions.cpp',
                      'src/papi_utils.cpp',
                      'src/perf_dump.cpp']
inc = include_directories('include')

papi_dep = dependency('papi', required: true)
deps = [papi_dep]
build_args = []

module_deps = []

if get_option('use_mpi')
  mpi_dep = dependency('mpi', language: 'cpp', required: true)
  module_deps += 'mpi4py'
  deps += mpi_dep
  build_args += '-DUSE_MPI'
endif
if get_option('enable_hdf5')
  hdf5_dep = dependency('hdf5', language: 'cpp', required: true)
  deps += hdf5_dep
  build_args += '-DENABLE_HDF5'
endif

py = import('python').find_installation('python3', modules: module_deps)
if py.language_version().version_compare('<3.12')
  build_args += '-DDECREFNONE'
endif
if get_option('silence_warnings')
  build_args += '-DSILENCE_WARNINGS'
endif

py.extension_module(
  'pyperfdump',
  pyperfdump_sources,
  override_options: ['cython_language=cpp'],
  install: true,
  cpp_args: build_args,
  include_directories: inc,
  dependencies: deps,
)
