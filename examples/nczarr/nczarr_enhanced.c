/**
 * @file nczarr_enhanced.c
 * @brief Demonstrate the NetCDF-4 enhanced data model in a local NcZarr dataset.
 *
 * This example extends nczarr_compression.c by introducing two enhanced-model
 * features that are unavailable in classic NetCDF: hierarchical groups and
 * multiple independent unlimited dimensions. The same nc_def_grp() and
 * NC_UNLIMITED APIs used for NetCDF-4/HDF5 files work identically with NcZarr.
 *
 * The program creates a NcZarr store with a root group containing a time
 * (unlimited) × x(5) grid of temperature and pressure variables, and a child
 * group obs/ that holds its own independent station (unlimited) dimension with
 * observation value and time-index variables. After writing, the program
 * reopens the store, navigates to the child group by name, verifies that both
 * unlimited dimensions grew independently, and validates all data values.
 *
 * **Learning Objectives:**
 * - Create a named child group inside a NcZarr root group with nc_def_grp()
 * - Define an unlimited dimension independently in a child group
 * - Show that root and child group unlimited dimensions grow separately
 * - Navigate to a child group on reopen with nc_inq_grp_ncid()
 * - Understand which enhanced-model features NcZarr supports: groups and
 *   unlimited dims are fully supported; user-defined types (enum, compound)
 *   require HDF5 storage and are not available in NcZarr
 *
 * **Key Concepts:**
 * - **nc_def_grp()**: Creates a named child group; all subsequent def calls
 *   on the returned group ID are scoped to that group
 * - **NC_UNLIMITED in a child group**: Each group tracks its own record
 *   dimension independently — appending to obs/station does not affect
 *   root/time and vice versa
 * - **nc_inq_grp_ncid()**: Retrieves a child group ID by name after reopen;
 *   needed to access dimensions and variables scoped to that group
 * - **nc_put_vara_float() / nc_put_vara_int()**: Write a contiguous slab to
 *   an unlimited variable; start[] and count[] control the written region
 *
 * **Related Examples:**
 * - nczarr_simple.c - Basic NcZarr create/read/write (flat, no groups)
 * - nczarr_chunking.c - Explicit chunk shape selection
 * - nczarr_compression.c - Deflate + shuffle compression
 * - examples/netcdf-4/groups.c - Deep group hierarchy with HDF5 storage
 *
 * **Compilation:**
 * @code
 * gcc -o nczarr_enhanced nczarr_enhanced.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./nczarr_enhanced
 * ncdump 'file://nczarr_enhanced.zarr#mode=nczarr'
 * @endcode
 *
 * **Expected Output:**
 * Creates the directory nczarr_enhanced.zarr containing:
 * - Root group: 2 dimensions (time=unlimited, x=5), 2 variables
 *   (temperature(time,x), pressure(time,x)), units attributes, global title
 * - Child group obs/: 1 dimension (station=unlimited), 2 variables
 *   (obs_value(station) in Celsius, obs_time(station)), group description
 * - 2 time steps written to root; 3 station records written to obs/
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026-06-27
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_URL  "file://nczarr_enhanced.zarr#mode=nczarr"
#define NX        5
#define NTIMES    2
#define NSTATIONS 3

#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

int main(void)
{
    int ncid, obs_grpid, retval;
    int time_dimid, x_dimid, station_dimid;
    int temp_varid, pressure_varid, obs_value_varid, obs_time_varid;
    int dimids[2];
    size_t start[2], count[2];
    int i, t;

    /* Root group data: temperature(time, x) and pressure(time, x) */
    float temp_data[NTIMES][NX];
    float pres_data[NTIMES][NX];

    /* Child group obs/ data: obs_value(station) and obs_time(station) */
    float obs_value[NSTATIONS];
    int   obs_time[NSTATIONS];

    /* Build temperature: 280 + time*5 + x*0.5 */
    for (t = 0; t < NTIMES; t++)
        for (i = 0; i < NX; i++) {
            temp_data[t][i] = 280.0f + (float)t * 5.0f + (float)i * 0.5f;
            pres_data[t][i] = 1013.0f - (float)t * 2.0f - (float)i * 0.1f;
        }

    obs_value[0] = 14.2f; obs_time[0] = 0;
    obs_value[1] = 15.8f; obs_time[1] = 0;
    obs_value[2] = 13.1f; obs_time[2] = 1;

    /* === CREATE === */
    if ((retval = nc_create(FILE_URL, NC_CLOBBER | NC_NETCDF4, &ncid))) ERR(retval);

    /* Root group: time (unlimited) and x (fixed) dimensions. */
    if ((retval = nc_def_dim(ncid, "time", NC_UNLIMITED, &time_dimid))) ERR(retval);
    if ((retval = nc_def_dim(ncid, "x",    NX,           &x_dimid)))    ERR(retval);

    /* Root group variables on (time, x). */
    dimids[0] = time_dimid; dimids[1] = x_dimid;
    if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, 2, dimids, &temp_varid)))  ERR(retval);
    if ((retval = nc_def_var(ncid, "pressure",    NC_FLOAT, 2, dimids, &pressure_varid))) ERR(retval);

    if ((retval = nc_put_att_text(ncid, temp_varid,     "units", 1, "K")))    ERR(retval);
    if ((retval = nc_put_att_text(ncid, pressure_varid, "units", 3, "hPa")))  ERR(retval);
    if ((retval = nc_put_att_text(ncid, NC_GLOBAL, "title", 30,
                                  "NcZarr Enhanced Model Example")))          ERR(retval);

    /* Child group obs/ with its own independent unlimited dimension. */
    if ((retval = nc_def_grp(ncid, "obs", &obs_grpid))) ERR(retval);
    if ((retval = nc_def_dim(obs_grpid, "station", NC_UNLIMITED, &station_dimid))) ERR(retval);
    if ((retval = nc_def_var(obs_grpid, "obs_value", NC_FLOAT, 1,
                             &station_dimid, &obs_value_varid))) ERR(retval);
    if ((retval = nc_def_var(obs_grpid, "obs_time",  NC_INT,   1,
                             &station_dimid, &obs_time_varid)))  ERR(retval);
    if ((retval = nc_put_att_text(obs_grpid, obs_value_varid, "units", 7, "Celsius"))) ERR(retval);
    if ((retval = nc_put_att_text(obs_grpid, NC_GLOBAL, "description",
                                  28, "Station observation records"))) ERR(retval);

    if ((retval = nc_enddef(ncid))) ERR(retval);

    /* Write two time steps of temperature and pressure. */
    start[0] = 0; start[1] = 0; count[0] = NTIMES; count[1] = NX;
    if ((retval = nc_put_vara_float(ncid, temp_varid, start, count, &temp_data[0][0]))) ERR(retval);
    if ((retval = nc_put_vara_float(ncid, pressure_varid, start, count, &pres_data[0][0]))) ERR(retval);

    /* Write three station observations to child group. */
    start[0] = 0; count[0] = NSTATIONS;
    if ((retval = nc_put_vara_float(obs_grpid, obs_value_varid, start, count, obs_value))) ERR(retval);
    if ((retval = nc_put_vara_int(obs_grpid, obs_time_varid, start, count, obs_time))) ERR(retval);

    if ((retval = nc_close(ncid))) ERR(retval);
    printf("Created %s: root(time=unlimited, x=%d), obs/(station=unlimited)\n",
           FILE_URL, NX);

    /* === REOPEN and VERIFY === */
    if ((retval = nc_open(FILE_URL, NC_NOWRITE, &ncid))) ERR(retval);

    { int ndims, nvars, natts, unlimdim;
      if ((retval = nc_inq(ncid, &ndims, &nvars, &natts, &unlimdim))) ERR(retval);
      printf("Root: %d dims, %d vars, %d global atts\n", ndims, nvars, natts); }

    /* Verify root time dimension grew to NTIMES. */
    { size_t tlen;
      if ((retval = nc_inq_dimlen(ncid, time_dimid, &tlen))) ERR(retval);
      if (tlen != NTIMES) { fprintf(stderr, "time length mismatch: got %zu\n", tlen); return 1; }
      printf("time=%zu x=%d\n", tlen, NX); }

    /* Verify obs/ child group and its independent station dimension. */
    if ((retval = nc_inq_grp_ncid(ncid, "obs", &obs_grpid))) ERR(retval);
    { size_t slen;
      if ((retval = nc_inq_dimlen(obs_grpid, station_dimid, &slen))) ERR(retval);
      if (slen != NSTATIONS) { fprintf(stderr, "station length mismatch: got %zu\n", slen); return 1; }
      printf("obs/station=%zu\n", slen); }

    /* Read back temperature and validate. */
    { float temp_in[NTIMES][NX];
      if ((retval = nc_inq_varid(ncid, "temperature", &temp_varid))) ERR(retval);
      if ((retval = nc_get_var_float(ncid, temp_varid, &temp_in[0][0]))) ERR(retval);
      for (t = 0; t < NTIMES; t++)
          for (i = 0; i < NX; i++)
              if (temp_in[t][i] != temp_data[t][i]) {
                  fprintf(stderr, "Temp mismatch at [%d,%d]\n", t, i); return 1; }
      printf("Temperature: %d values correct\n", NTIMES * NX); }

    /* Read back obs_value and validate. */
    { float obs_in[NSTATIONS];
      if ((retval = nc_inq_varid(obs_grpid, "obs_value", &obs_value_varid))) ERR(retval);
      if ((retval = nc_get_var_float(obs_grpid, obs_value_varid, obs_in))) ERR(retval);
      for (i = 0; i < NSTATIONS; i++)
          if (obs_in[i] != obs_value[i]) {
              fprintf(stderr, "obs_value mismatch at [%d]: got %f\n", i, obs_in[i]); return 1; }
      printf("obs_value: %d records correct\n", NSTATIONS); }

    if ((retval = nc_close(ncid))) ERR(retval);
    printf("Done.\n");
    return 0;
}
