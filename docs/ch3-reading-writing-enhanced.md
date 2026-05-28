## Reading and Writing Data with the Enhanced Model

The enhanced model extends the read/write API in several directions. New atomic types require typed functions in C. Variables inside groups need the group's `ncid` rather than the file's root `ncid`. User-defined types introduce their own write and read patterns. Strings require memory management that differs from all other types. This section covers each area in turn, following the pattern established in the Classic Model section above.

### Creating a NetCDF-4 File

All enhanced-model features require the `NC_NETCDF4` flag at file creation. Without it, the library creates a classic-format file and rejects any enhanced-model operation.

In C:

```c
if ((retval = nc_create(FILE_NAME, NC_CLOBBER|NC_NETCDF4, &ncid)))
    ERR(retval);
```

In Fortran:

```fortran
retval = nf90_create(FILE_NAME, NF90_CLOBBER + NF90_NETCDF4, ncid)
if (retval /= nf90_noerr) call handle_err(retval)
```

To enforce classic-model behavior in a netCDF-4/HDF5 file, add `NC_CLASSIC_MODEL` (C) or `NF90_CLASSIC_MODEL` (Fortran) alongside `NC_NETCDF4`. This prevents enhanced features from being used while still producing an HDF5 file.

### Writing and Reading New Atomic Types

The five additional integer types available in netCDF-4 each have their own typed C functions. These follow the same naming pattern as the classic types but use the type name as a suffix.

| NetCDF-4 type | C write function          | C read function           |
|---------------|---------------------------|---------------------------|
| NC_UBYTE      | nc_put_var_uchar          | nc_get_var_uchar          |
| NC_USHORT     | nc_put_var_ushort         | nc_get_var_ushort         |
| NC_UINT       | nc_put_var_uint           | nc_get_var_uint           |
| NC_INT64      | nc_put_var_longlong       | nc_get_var_longlong       |
| NC_UINT64     | nc_put_var_ulonglong      | nc_get_var_ulonglong      |

This code from `groups.c` in the NetCDF Expansion Pack writes one variable of each type, each into a different group:

```c
if ((retval = nc_put_var_uchar(ncid, ubyte_varid, &ubyte_data[0][0])))
    ERR(retval);
if ((retval = nc_put_var_ushort(grp1_id, ushort_varid, &ushort_data[0][0])))
    ERR(retval);
if ((retval = nc_put_var_uint(grp2_id, uint_varid, &uint_data[0][0])))
    ERR(retval);
if ((retval = nc_put_var_longlong(nested_id, int64_varid, &int64_data[0][0])))
    ERR(retval);
if ((retval = nc_put_var_ulonglong(nested_id, uint64_varid, &uint64_data[0][0][0])))
    ERR(retval);
```

Reading them back uses the parallel `nc_get_var_*` functions:

```c
if ((retval = nc_get_var_uchar(ncid, ubyte_varid, &ubyte_in[0][0])))
    ERR(retval);
if ((retval = nc_get_var_ushort(grp1_id, ushort_varid, &ushort_in[0][0])))
    ERR(retval);
if ((retval = nc_get_var_longlong(nested_id, int64_varid, &int64_in[0][0])))
    ERR(retval);
if ((retval = nc_get_var_ulonglong(nested_id, uint64_varid, &uint64_in[0][0][0])))
    ERR(retval);
```

Notice that the `ncid` passed to each call is the group ID where that variable lives, not the root file ID. Variables defined in a group are only accessible through that group's `ncid`.

In Fortran, `nf90_put_var` and `nf90_get_var` are overloaded and handle all five new types automatically. The correct implementation is selected from the declared type of the array argument, just as with classic types. No new function names are needed.

### Variables in Groups

When reading a file with groups, use `nc_inq_grp_ncid()` to navigate to a group by name. The returned group ID then serves as the `ncid` for all variable operations in that group. This code from `groups.c` navigates the group hierarchy after reopening the file:

```c
if ((retval = nc_inq_grp_ncid(ncid, "SubGroup1", &grp1_id)))
    ERR(retval);
if ((retval = nc_inq_grp_ncid(ncid, "SubGroup2", &grp2_id)))
    ERR(retval);
if ((retval = nc_inq_grp_ncid(grp2_id, "NestedGroup", &nested_id)))
    ERR(retval);
```

Once you have the group ID, `nc_inq_varid()` and the typed get/put functions work exactly as they do in the root group. Variable IDs are scoped to their group; the same integer ID value in two different groups refers to two different variables.

In Fortran, `nf90_inq_grp_ncid()` returns the group ID:

```fortran
retval = nf90_inq_grp_ncid(ncid, "SubGroup1", grp1_id)
if (retval /= nf90_noerr) call handle_err(retval)
```

### Appending Along Multiple Unlimited Dimensions

In the classic model only one unlimited dimension is permitted. In netCDF-4/HDF5 files any number of unlimited dimensions may be defined, and they may appear in any position in a variable's dimension list. The `multi_unlimited.c` example in the NetCDF Expansion Pack demonstrates a variable indexed by both an unlimited `station` dimension and an unlimited `time` dimension:

```c
if ((retval = nc_def_dim(ncid, "station", NC_UNLIMITED, &station_dimid)))
    ERR(retval);
if ((retval = nc_def_dim(ncid, "time", NC_UNLIMITED, &time_dimid)))
    ERR(retval);

dimids[0] = station_dimid;
dimids[1] = time_dimid;
if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, NDIMS, dimids, &varid)))
    ERR(retval);
```

Appending along either dimension uses `nc_put_vara_float()` with the appropriate `start` and `count` arrays, exactly as with a single unlimited dimension. Because HDF5 stores data in chunks, there is no restriction on which dimension is unlimited or where unlimited dimensions appear.

### Writing and Reading Compound Variables

Compound type variables are written and read with the generic `nc_put_var()` and `nc_get_var()` functions in C. These functions transfer raw bytes between the file and a C struct whose layout matches the compound type definition. The `user_types.c` example from the NetCDF Expansion Pack writes an array of `WeatherObs` structs:

```c
WeatherObs obs_data[NOBS];
for (int i = 0; i < NOBS; i++) {
    obs_data[i].time = 1000.0 + i * 3600.0;
    obs_data[i].temperature = 20.0 + i * 2.0;
    obs_data[i].pressure = 1013.0 + i * 0.5;
    obs_data[i].humidity = 60.0 - i * 5.0;
}
if ((retval = nc_put_var(ncid, compound_varid, obs_data)))
    ERR(retval);
```

Reading uses `nc_get_var()` into a matching struct array:

```c
WeatherObs obs_read[NOBS];
if ((retval = nc_get_var(ncid, compound_varid, obs_read)))
    ERR(retval);
```

The struct layout must match the byte offsets registered when the type was defined with `nc_insert_compound()`. Use `offsetof()` when defining the type and declare the struct fields in the same order to ensure the layouts agree.

Compound variable I/O is not available from Fortran. The Fortran interface has no mechanism to match derived type memory layouts to the byte offsets required by a compound type definition. Fortran users who need to store grouped fields should decompose them into separate scalar variables, or call the NetCDF C API directly through `ISO_C_BINDING`. See the discussion under The Compound Type above for a full explanation.

### Writing and Reading VLEN Variables

Variable-length type variables are written and read with `nc_put_var()` and `nc_get_var()` using arrays of `nc_vlen_t`. Each element of the array has a `len` field for the number of values and a `p` pointer to the data for that element. After reading, call `nc_free_vlen()` to release the library-allocated memory. This code from `user_types.c` demonstrates both:

```c
/* Write */
nc_vlen_t vlen_data[NDAYS];
int day1_obs[] = {10, 15, 20};
int day2_obs[] = {12, 18, 22, 25};
int day3_obs[] = {8, 14};

vlen_data[0].len = 3; vlen_data[0].p = day1_obs;
vlen_data[1].len = 4; vlen_data[1].p = day2_obs;
vlen_data[2].len = 2; vlen_data[2].p = day3_obs;

if ((retval = nc_put_var(ncid, vlen_varid, vlen_data)))
    ERR(retval);

/* Read */
nc_vlen_t vlen_read[NDAYS];
if ((retval = nc_get_var(ncid, vlen_varid, vlen_read)))
    ERR(retval);
/* ... use vlen_read[d].len and (int *)vlen_read[d].p ... */
if ((retval = nc_free_vlen(vlen_read)))
    ERR(retval);
```

VLEN variable I/O is not available from Fortran for the same reasons as compound types. The `nc_vlen_t` struct depends on C pointer semantics and heap management that have no portable Fortran equivalent. See the discussion under The VLEN Type above.

### Writing and Reading String Variables

String variables in C use `nc_put_var_string()` and `nc_get_var_string()`. The write call accepts an array of `const char *` pointers. The read call fills an array of `char *` pointers, each pointing to library-allocated memory; call `nc_free_string()` after use. This code from `user_types.c` writes station names and then reads them back:

```c
/* Write */
const char *station_names[NSTATIONS] = {
    "Boulder, CO", "Cape Canaveral, FL",
    "Wallops Island, VA", "White Sands, NM"
};
if ((retval = nc_put_var_string(ncid, string_varid, station_names)))
    ERR(retval);

/* Read */
char *station_read[NSTATIONS];
if ((retval = nc_get_var_string(ncid, string_varid, station_read)))
    ERR(retval);
/* ... use station_read[i] ... */
if ((retval = nc_free_string(NSTATIONS, station_read)))
    ERR(retval);
```

String variable I/O is not supported in the Fortran interface. Fortran programs that need to store text should use `NF90_CHAR` variables with an extra character-length dimension instead. See the discussion under Strings and Fortran above.

### Writing and Reading Enum Variables

Enum variables store integer data, so the write and read operations transfer integer arrays. In C, use `nc_put_var()` and `nc_get_var()` with a buffer whose C type matches the enum's base integer type. In the `user_types.c` example the enum is based on `NC_INT` and the underlying C type is `CloudCover` (an `int`-sized enum):

```c
CloudCover cloud_data[NOBS] = {CLEAR, PARTLY_CLOUDY, CLOUDY, PARTLY_CLOUDY, OVERCAST};
if ((retval = nc_put_var(ncid, enum_varid, cloud_data)))
    ERR(retval);

CloudCover cloud_read[NOBS];
if ((retval = nc_get_var(ncid, enum_varid, cloud_read)))
    ERR(retval);
```

In Fortran, enum variable I/O uses the F77 generic functions `nf_put_var` and `nf_get_var`, which transfer raw bytes without type checking. The variable must be defined with `nf_def_var` rather than `nf90_def_var`, because `nf90_def_var` validates the type argument against built-in types and rejects user-defined type IDs. This code from the `f_user_types.f90` example in the NetCDF Expansion Pack shows the pattern:

```fortran
! Define using F77 API (nf_def_var accepts user-defined type IDs)
retval = nf_def_var(ncid, "cloud_cover", enum_typeid, 1, dimids, enum_varid)
if (retval /= nf90_noerr) call handle_err(retval)

! Write enum data as integer array
retval = nf_put_var(ncid, enum_varid, cloud_out)
if (retval /= nf90_noerr) call handle_err(retval)

! Read enum data back
retval = nf_get_var(ncid, enum_varid, cloud_in)
if (retval /= nf90_noerr) call handle_err(retval)
```

The integer values written must be members of the enum type; the library enforces this on write.

### Writing and Reading Opaque Variables

Opaque variables store fixed-size binary blobs. In C, `nc_put_var()` and `nc_get_var()` treat the buffer as raw bytes; no type conversion is performed. In `user_types.c` an array of `unsigned char` fills a scalar opaque variable:

```c
unsigned char calib_data[CALIB_SIZE];
for (int i = 0; i < CALIB_SIZE; i++)
    calib_data[i] = (unsigned char)(i * 17);
if ((retval = nc_put_var(ncid, opaque_varid, calib_data)))
    ERR(retval);

unsigned char calib_read[CALIB_SIZE];
if ((retval = nc_get_var(ncid, opaque_varid, calib_read)))
    ERR(retval);
```

In Fortran, a fixed-length `character` buffer of the correct size serves as the container. As with enum variables, `nf_def_var` is needed to define the variable, and `nf_put_var1` / `nf_get_var1` handle single-element I/O. From `f_user_types.f90`:

```fortran
character(len=CALIB_SIZE) :: calib_out, calib_in

! Write opaque data
index(1) = 1
retval = nf_put_var1(ncid, opaque_varid, index, calib_out)
if (retval /= nf90_noerr) call handle_err(retval)

! Read opaque data
retval = nf_get_var1(ncid, opaque_varid, index, calib_in)
if (retval /= nf90_noerr) call handle_err(retval)
```

### Summary of I/O Support by Type and Language

| Type          | C write/read                         | Fortran write/read                          |
|---------------|--------------------------------------|---------------------------------------------|
| New atomics   | nc_put_var_*/nc_get_var_*            | nf90_put_var / nf90_get_var (overloaded)    |
| Compound      | nc_put_var / nc_get_var              | Not supported                               |
| VLEN          | nc_put_var / nc_get_var + nc_free_vlen | Not supported                             |
| String        | nc_put_var_string / nc_get_var_string + nc_free_string | Not supported           |
| Enum          | nc_put_var / nc_get_var              | nf_put_var / nf_get_var (F77 API)           |
| Opaque        | nc_put_var / nc_get_var              | nf_put_var1 / nf_get_var1 (F77 API)         |

(The full enhanced-model examples for C are covered in chapter 6 and for Fortran in chapter 7.)
