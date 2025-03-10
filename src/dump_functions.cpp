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

#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include "papi_utils.h"
#include "pyperfdump.h"

#ifdef USE_MPI
  #include <cstdio>
  #include <cstdlib>
  #include <mpi.h>
  #include <string.h>
#endif
#ifdef ENABLE_HDF5
  #include <hdf5.h>
#endif

void dumpcsv(const int rank, const int num_procs,
              const char *const filename,
              const char *const region_name,
              const PAPIEventSet *const event_set,
              std::unordered_map<std::string,unsigned long long> &buffer,
              const double runtime) {
#ifdef USE_MPI
  // the length of each process' contribution to the csv
  unsigned int lens[num_procs];
  // the length of this rank begins at zero
  lens[rank] = 0;
  // a buffer that will be used to build intermediate strings
  char linebuffer[256];
  // the output file
  // open at end, create if doesn't exist, write only, no concurrent opens
  MPI_File output_file;
  MPI_File_open(MPI_COMM_WORLD, filename,
        MPI_MODE_APPEND|MPI_MODE_CREATE|MPI_MODE_WRONLY|MPI_MODE_UNIQUE_OPEN,
                MPI_INFO_NULL, &output_file);
  // the current offset is the end of the file, this is the offset start
  MPI_Offset offset;
  MPI_File_get_position(output_file, &offset);
  // gather the length of our rank's data
  for (const auto &event : event_set->event_names()) {
    snprintf(linebuffer, 256, "%d,%s,%s,%llu\n",
                            rank, region_name, event.c_str(), buffer[event]);
    lens[rank] += strlen(linebuffer);
  }
  // precision of 7 decimals matched hdf5 output in test
  snprintf(linebuffer, 256, "%d,%s,Runtime,%.7f\n",
                            rank, region_name, runtime);
  lens[rank] += strlen(linebuffer);
  // this rank's length is calculated, start a non-blocking allgather
  MPI_Request request;
  MPI_Iallgather(MPI_IN_PLACE, 1, MPI_UNSIGNED,
                  lens, 1, MPI_UNSIGNED, MPI_COMM_WORLD, &request);
  // start with a null string for strcat
  char *lines = (char*)malloc(sizeof(char)*(lens[rank]+1));
  lines[0] = '\0';
  // build 1 big string
  for (const auto &event : event_set->event_names()) {
    snprintf(linebuffer, 256, "%d,%s,%s,%llu\n",
                            rank, region_name, event.c_str(), buffer[event]);
    strcat(lines, linebuffer);
  }
  snprintf(linebuffer, 256, "%d,%s,Runtime,%.7f\n",
                            rank, region_name, runtime);
  strcat(lines, linebuffer);
  // we have our string built, wait for lengths to determine offset
  MPI_Wait(&request, MPI_STATUS_IGNORE);
  for (int i=0;i<rank;++i) {
    offset += lens[i];
  }
  MPI_File_write_at_all(output_file, offset, lines,
                        lens[rank], MPI_CHAR, MPI_STATUS_IGNORE);
  MPI_File_close(&output_file);
#else //ifndef USE_MPI
  std::ofstream output_file(filename, std::ios::app);
  for (const auto &event : event_set->event_names()) {
    output_file << region_name << "," << event << ","
                << buffer[event] << std::endl;
  }
  output_file << region_name << "," << "Runtime" << ","
              << runtime << std::endl;
  output_file.close();
#endif
}

#ifdef ENABLE_HDF5
static hid_t open_hdf5(const char *const filename) {
  hid_t access_plist = H5Pcreate(H5P_FILE_ACCESS);

#ifdef USE_MPI
  MPI_Info info;
  /* Create info to be attached to HDF5 file */
  MPI_Info_create(&info);

  /* Disables ROMIO's data-sieving */
  MPI_Info_set(info, "romio_ds_read", "disable");
  MPI_Info_set(info, "romio_ds_write", "disable");

  /* Enable ROMIO's collective buffering */
  MPI_Info_set(info, "romio_cb_read", "enable");
  MPI_Info_set(info, "romio_cb_write", "enable");

  H5Pset_fapl_mpio(access_plist, MPI_COMM_WORLD, info);
#else
  H5Pset_fapl_stdio(access_plist);
#endif
  // Just fail if it exists, then we'll open
  hid_t h5file = H5Fcreate(filename,
                            H5F_ACC_EXCL, H5P_DEFAULT, access_plist);
  // If it failed then it exists and we have to open it
  if (h5file == H5I_INVALID_HID) {
    h5file = H5Fopen(filename, H5F_ACC_RDWR, access_plist);
  }
  H5Pclose(access_plist);
  return h5file;
}

static void create_datasets(const int num_procs,
                            const char *const region_name,
                            const PAPIEventSet *const event_set,
                            const hid_t h5file) {
  hsize_t cur_dims[]   = {static_cast<hsize_t>(num_procs),
                          1,
                          1};
  hsize_t chunk_dims[] = {static_cast<hsize_t>(num_procs),
                          1,
                          1};
  hsize_t max_dims[]   = {static_cast<hsize_t>(num_procs),
                          1,
                          H5S_UNLIMITED};
  const hsize_t ndim = 3;
  hid_t group_id;
  {
    /* Save old error handler */
    H5E_auto2_t  oldfunc;
    void *old_client_data;
    H5Eget_auto(H5E_DEFAULT, &oldfunc, &old_client_data);
    // Disable the error handler
    H5Eset_auto(H5E_DEFAULT, NULL, NULL);
    const htri_t exists = H5Lexists(h5file, region_name, H5P_DEFAULT);
    // Enable the error handler
    H5Eset_auto(H5E_DEFAULT, oldfunc, old_client_data);
    if (exists == 1)
      group_id = H5Gopen(h5file, region_name, H5P_DEFAULT);
    else
      group_id = H5Gcreate(h5file, region_name,
                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
  for (const auto &event : event_set->event_names()) {
    hid_t space = H5Screate_simple(ndim, cur_dims, max_dims);
    hid_t chunk_plist = H5Pcreate(H5P_DATASET_CREATE);
    const int status = H5Pset_chunk(chunk_plist, ndim, chunk_dims);
    PD_ASSERT(status >= 0,
              "setting size of chunks, H5Pset_chunk returned %d", status);
    const htri_t exists = H5Lexists(group_id, event.c_str(), H5P_DEFAULT);
    if (!exists) {
      hid_t dset = H5Dcreate(group_id,event.c_str(),H5T_NATIVE_LLONG,
                              space, H5P_DEFAULT, chunk_plist, H5P_DEFAULT);
      H5Dclose(dset);
    }
    H5Pclose(chunk_plist);
    H5Sclose(space);
  }
  hid_t space = H5Screate_simple(ndim, cur_dims, max_dims);
  hid_t chunk_plist = H5Pcreate(H5P_DATASET_CREATE);
  const int status = H5Pset_chunk(chunk_plist, ndim, chunk_dims);
  PD_ASSERT(status >= 0,
            "setting size of chunks, H5Pset_chunk returned %d", status);
  hid_t dset = H5Dcreate(group_id, "Runtime", H5T_NATIVE_DOUBLE,
                          space, H5P_DEFAULT, chunk_plist, H5P_DEFAULT);
  H5Pclose(chunk_plist);
  H5Dclose(dset);
  H5Sclose(space);
  H5Gclose(group_id);
}

static void append_row(const int rank,
                        const char *const region_name,
                        const hid_t dump_file_id,
                        const std::string &event_name,
                        const hid_t data_type,
                        void *data) {
  const hid_t group_id = H5Gopen1(dump_file_id, region_name);
  const hid_t dataset_id = H5Dopen1(group_id, event_name.c_str());
  const hid_t space_id = H5Dget_space(dataset_id);
  const hsize_t ndim = H5Sget_simple_extent_ndims(space_id);
  hsize_t dims[ndim], maxdims[ndim];
  H5Sget_simple_extent_dims(space_id, dims, maxdims);
  dims[ndim-1] += 1; // add another time step
  H5Dset_extent(dataset_id, dims);
  hsize_t offset[] = {static_cast<hsize_t>(rank), 0, dims[ndim - 1] - 2};
  hsize_t count[] = {1, 1, 1};
  hid_t xfer_plist = H5Pcreate(H5P_DATASET_XFER);
#ifdef USE_MPI
  H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);
#else
  H5Pset_fapl_stdio(xfer_plist);
#endif
  const hid_t memspaceid = H5Screate_simple(ndim, count, NULL);
  H5Sselect_hyperslab(space_id, H5S_SELECT_SET, offset, NULL, count, NULL);
  H5Dwrite(dataset_id, data_type, memspaceid, space_id, xfer_plist, data);
  H5Dclose(dataset_id);
  H5Sclose(space_id);
  H5Pclose(xfer_plist);
  H5Gclose(group_id);
}
void dumphdf5(const int rank, const int num_procs,
              const char *const filename,
              const char *const region_name,
              const PAPIEventSet *const event_set,
              std::unordered_map<std::string,unsigned long long> &buffer,
              const double runtime) {
  /* Save old error handler */
  H5E_auto2_t oldfunc;
  void *old_client_data;
  H5Eget_auto(H5E_DEFAULT, &oldfunc, &old_client_data);

  /* Turn off error handling */
  H5Eset_auto(H5E_DEFAULT, NULL, NULL);

  const hid_t dump_file_id = open_hdf5(filename);
  create_datasets(num_procs, region_name, event_set, dump_file_id);

  for (const auto &event : event_set->event_names()) {
    append_row(rank, region_name,
                dump_file_id, event, H5T_NATIVE_LLONG, (void*)&buffer[event]);
  }
  append_row(rank, region_name,
              dump_file_id, "Runtime", H5T_NATIVE_DOUBLE, (void*)&runtime);

  H5Fclose(dump_file_id);

  /* Restore previous error handler */
  H5Eset_auto(H5E_DEFAULT, oldfunc, old_client_data);
}
#endif //ENABLE_HDF5
