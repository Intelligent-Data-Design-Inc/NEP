## Reading and Writing Data

Variables are defined during define mode but data is written and read only in data mode. The netCDF C library provides a family of typed functions for both operations; the Fortran library uses generic overloaded functions instead. This section covers the full read/write cycle: writing an entire variable at once, writing a subset with start/count arrays, reading data back, and handling the variable ID lookup you need when opening an existing file.

### Writing an Entire Variable

The simplest write operation transfers all elements of a variable in one call. In C, use `nc_put_var_<type>()` where `<type>` matches the variable's netCDF type:

```c
/* Write entire 2D integer array */
if ((retval = nc_put_var_int(ncid, varid, &data_out[0][0])))
   ERR(retval);
```

In Fortran, `nf90_put_var()` is overloaded and accepts any array type directly:

```fortran
! Write entire 2D integer array
retval = nf90_put_var(ncid, varid, data_out)
if (retval /= nf90_noerr) call handle_err(retval)
```

The full set of typed write functions in C mirrors the attribute functions:

| NetCDF type | C write function       | C read function       |
|-------------|------------------------|-----------------------|
| NC_CHAR     | nc_put_var_text        | nc_get_var_text       |
| NC_BYTE     | nc_put_var_schar       | nc_get_var_schar      |
| NC_SHORT    | nc_put_var_short       | nc_get_var_short      |
| NC_INT      | nc_put_var_int         | nc_get_var_int        |
| NC_FLOAT    | nc_put_var_float       | nc_get_var_float      |
| NC_DOUBLE   | nc_put_var_double      | nc_get_var_double     |

In Fortran, all types use `nf90_put_var(ncid, varid, values)` and `nf90_get_var(ncid, varid, values)`. The compiler selects the correct implementation from the type of `values`.

### Writing a Subset with Start and Count

Many applications need to write only part of a variable—a single time step, a spatial tile, or a block of records. The `nc_put_vara_<type>()` functions in C accept a `start` array and a `count` array that specify the hyperslab to write. `start` gives the zero-based index of the first element along each dimension; `count` gives how many elements to write along each dimension.

This example from `simple_2D.c` in the NetCDF Expansion Pack writes the first `NY-1` rows of a two-dimensional integer variable, leaving the last row unwritten:

```c
size_t start[NDIMS] = {0, 0};
size_t count[NDIMS] = {NY - 1, NX};
if ((retval = nc_put_vara_int(ncid, varid, start, count, &data_out[0][0])))
   ERR(retval);
```

The Fortran equivalent passes `start` and `count` as keyword arguments to `nf90_put_var()`. In Fortran the indices are 1-based and the dimension order is reversed relative to C:

```fortran
start_idx = (/ 1, 1 /)
count_idx = (/ NX, NY - 1 /)
retval = nf90_put_var(ncid, varid, data_out(:, 1:NY-1), start=start_idx, count=count_idx)
if (retval /= nf90_noerr) call handle_err(retval)
```

The same `start`/`count` mechanism works for appending to an unlimited dimension. The `unlimited_dim.c` example in the NetCDF Expansion Pack writes three initial time steps and then reopens the file to append two more:

```c
/* Write initial timesteps (0, 1, 2) */
size_t start[3] = {0, 0, 0};
size_t count[3] = {INITIAL_TIMESTEPS, NLAT, NLON};
if ((retval = nc_put_vara_float(ncid, temp_varid, start, count, &temp_data[0][0][0])))
   ERR(retval);

/* Later: append timesteps 3 and 4 */
start[0] = INITIAL_TIMESTEPS;
count[0] = APPEND_TIMESTEPS;
if ((retval = nc_put_vara_float(ncid, temp_varid, start, count,
                                 &temp_data[INITIAL_TIMESTEPS][0][0])))
   ERR(retval);
```

### Reading an Entire Variable

To read all elements of a variable in one call, use `nc_get_var_<type>()` in C:

```c
/* Read entire 2D integer array */
if ((retval = nc_get_var_int(ncid, varid, &data_in[0][0])))
   ERR(retval);
```

In Fortran, use `nf90_get_var()`:

```fortran
! Read entire 2D integer array
retval = nf90_get_var(ncid, varid, data_in)
if (retval /= nf90_noerr) call handle_err(retval)
```

The buffer you pass must be large enough to hold the entire variable. For large variables this can be impractical; use `nc_get_vara_<type>()` with start and count to read one hyperslab at a time.

### Looking Up Variable IDs in an Existing File

When you write a file and immediately reopen it, you know the variable IDs from the define phase. When you open a file written by someone else—or open your own file in a later program run—you must look up the IDs by name. Use `nc_inq_varid()` in C:

```c
/* Look up variable ID by name */
if ((retval = nc_inq_varid(ncid, "temperature", &temp_varid)))
   ERR(retval);
```

In Fortran, use `nf90_inq_varid()`:

```fortran
! Look up variable ID by name
retval = nf90_inq_varid(ncid, "temperature", temp_varid)
if (retval /= nf90_noerr) call handle_err(retval)
```

Once you have the variable ID you can query its type, number of dimensions, and dimension IDs with `nc_inq_var()` in C or `nf90_inquire_variable()` in Fortran, as shown in the Inquiring about an Open NetCDF File section above.

### A Complete Read/Write Cycle

The `simple_2D.c` and `f_simple_2D.f90` programs in the NetCDF Expansion Pack demonstrate the complete cycle. The write phase creates the file, defines dimensions and a variable with a custom fill value, leaves define mode, writes a partial array, and closes the file. The read phase reopens the file, verifies metadata with `nc_inq()` and `nc_inq_dim()`, reads the data with `nc_get_var_int()`, and confirms that written elements contain the expected sequential integers while unwritten elements contain the fill value.

The key steps in order are:

- `nc_create()` or `nc_open()` — obtain the `ncid`
- `nc_def_dim()`, `nc_def_var()`, `nc_put_att_*()`, `nc_def_var_fill()` — define mode only
- `nc_enddef()` — enter data mode (classic formats)
- `nc_put_var_*()` or `nc_put_vara_*()` — write data
- `nc_get_var_*()` or `nc_get_vara_*()` — read data
- `nc_close()` — flush buffers and release the file

(The full examples for reading and writing data are covered in detail for C in chapter 6 and for Fortran in chapter 7.)
