## Variable Attributes

Attributes attach metadata to variables or to the file as a whole. A global attribute uses `NC_GLOBAL` (C) or `NF90_GLOBAL` (Fortran) in place of a variable ID. Attributes are written during define mode and read at any time after the file is opened.

### Writing Attributes

In C, the write function name encodes the attribute type. In Fortran, `nf90_put_att` is overloaded and accepts any scalar or array value directly.

| NetCDF type | C write function | C read function |
|---|---|---|
| `NC_CHAR` | `nc_put_att_text` | `nc_get_att_text` |
| `NC_BYTE` | `nc_put_att_schar` | `nc_get_att_schar` |
| `NC_SHORT` | `nc_put_att_short` | `nc_get_att_short` |
| `NC_INT` | `nc_put_att_int` | `nc_get_att_int` |
| `NC_FLOAT` | `nc_put_att_float` | `nc_get_att_float` |
| `NC_DOUBLE` | `nc_put_att_double` | `nc_get_att_double` |

In Fortran, all types use `nf90_put_att(ncid, varid, name, value)` and `nf90_get_att(ncid, varid, name, value)`. The compiler resolves the correct implementation from the type of `value`.

The most common attribute type is text. `coord_vars.c` attaches CF convention metadata to coordinate variables:

```c
if ((retval = nc_put_att_text(ncid, lat_varid, "units", 13, "degrees_north")))
    ERR(retval);
if ((retval = nc_put_att_float(ncid, temp_varid, "_FillValue", NC_FLOAT, 1, &fill_value)))
    ERR(retval);
```
`@/home/ed/NEP/examples/classic/coord_vars.c:116-151`

The Fortran equivalent passes the value directly without a length or type argument:

```fortran
retval = nf90_put_att(ncid, lat_varid, "units", "degrees_north")
retval = nf90_put_att(ncid, temp_varid, "_FillValue", fill_value)
```
`@/home/ed/NEP/examples/f_classic/f_coord_vars.f90:98-132`

### Reading Attributes

Before reading a text attribute in C, call `nc_inq_attlen()` to get the length so you can allocate the right buffer. Numeric attributes have a known size from their type.

```c
size_t att_len;
char att_text[NC_MAX_NAME + 1];
if ((retval = nc_inq_attlen(ncid, lat_varid, "units", &att_len)))
    ERR(retval);
if ((retval = nc_get_att_text(ncid, lat_varid, "units", att_text)))
    ERR(retval);
att_text[att_len] = '\0';
```
`@/home/ed/NEP/examples/classic/coord_vars.c:201-205`

In Fortran, `nf90_get_att` writes directly into a character variable of sufficient length; `nf90_inquire_attribute` provides the length if you need it.

```fortran
retval = nf90_inquire_attribute(ncid, lat_varid, "units", len=att_len)
retval = nf90_get_att(ncid, lat_varid, "units", att_text)
```
`@/home/ed/NEP/examples/f_classic/f_coord_vars.f90:180-183`

`dump_classic_metadata.c` shows the general pattern for reading an attribute of unknown type: call `nc_inq_att()` to get the type and length, then dispatch to the appropriate typed getter.

## Fill Values and Fill Mode

A fill value is a sentinel that NetCDF returns for any element that was never written. It lets readers distinguish "no data here" from a legitimate zero or negative number. Without a fill value, unwritten storage contains whatever bytes happened to be on disk—garbage.

### Default Fill Values

Every NetCDF type has a built-in default fill value:

| Type | Default fill value |
|---|---|
| `NC_BYTE` | −127 |
| `NC_SHORT` | −32767 |
| `NC_INT` | −2147483647 |
| `NC_FLOAT` | 9.9692099683868690e+36 |
| `NC_DOUBLE` | 9.9692099683868690e+36 |
| `NC_CHAR` | `\0` |

Use the default when any out-of-range sentinel is acceptable. Override it when you have an established convention or when the default could collide with valid data. Always set a fill value for variables that may be partially written—sparse grids, time series with gaps, or any array where some elements may never be written. Never store a legitimate data value that equals the fill value; CF-aware tools treat it as missing and will silently exclude it from computations.

### Setting a Custom Fill Value

Call `nc_def_var_fill()` (C) or `nf90_def_var_fill()` (Fortran) during define mode, before `nc_enddef()`. The second argument is the *no-fill flag*: pass `NC_FILL` (0) to enable fill mode, `NC_NOFILL` (1) to disable it. The fill value must match the variable's type exactly; passing a float fill value for an integer variable will truncate it silently.

**C:**
```c
int fill_value = FILL_VALUE;   /* FILL_VALUE = -9999 */
if ((retval = nc_def_var_fill(ncid, varid, NC_FILL, &fill_value)))
    ERR(retval);
```
`@/home/ed/NEP/examples/classic/simple_2D.c:130-134`

**Fortran:**
```fortran
retval = nf90_def_var_fill(ncid, varid, 0, FILL_VALUE)
if (retval /= nf90_noerr) call handle_err(retval)
```
`@/home/ed/NEP/examples/f_classic/f_simple_2D.f90:138-139`

`simple_2D.c` writes a 6×12 integer array but leaves the last row unwritten. Those 6 elements return −9999 on any subsequent read.

### What Fill Mode Actually Does

When fill mode is `NC_FILL`, the library pre-fills every element with the fill value before any application write. This costs one extra write pass per variable at `nc_enddef()` time. The payoff is predictable behavior for partial writes.

When fill mode is `NC_NOFILL`, the library skips that pre-fill pass. Use `NC_NOFILL` when you know you will write every element—it can cut I/O time roughly in half for large variables with many small writes. Note that `NC_NOFILL` also suppresses the `_FillValue` attribute entirely, so readers have no sentinel to check against.

### Querying the Fill Value

To check the fill value on an existing file, use `nc_inq_var_fill()` (C) or `nf90_inq_var_fill()` (Fortran). Both return the no-fill flag and the fill value itself.

**C:**
```c
int no_fill, fill_value_in;
if ((retval = nc_inq_var_fill(ncid, varid, &no_fill, &fill_value_in)))
    ERR(retval);
```
`@/home/ed/NEP/examples/classic/simple_2D.c:273-275`

**Fortran:**
```fortran
retval = nf90_inq_var_fill(ncid, varid, no_fill, fill_value_in)
if (retval /= nf90_noerr) call handle_err(retval)
```
`@/home/ed/NEP/examples/f_classic/f_simple_2D.f90:269-270`

### The `_FillValue` Attribute

`nc_def_var_fill()` works by writing a `_FillValue` attribute internally — the two are the same mechanism, not alternatives. The attribute is visible in `ncdump` output and to any reader (CDL tools, Python xarray, etc.) regardless of which API wrote it.

The practical difference is the **no-fill flag**. Calling `nc_def_var_fill()` with `NC_NOFILL` both suppresses the pre-fill write pass *and* removes the `_FillValue` attribute. Writing the attribute directly with `nc_put_att_*` cannot do that.

CF Conventions require the fill value to appear as `_FillValue`. `coord_vars.c` writes it directly as an attribute, which is the typical CF pattern:

**C:**
```c
float fill_value = -999.0;
if ((retval = nc_put_att_float(ncid, temp_varid, "_FillValue", NC_FLOAT, 1, &fill_value)))
    ERR(retval);
```
`@/home/ed/NEP/examples/classic/coord_vars.c:149-151`

**Fortran:**
```fortran
fill_value = -999.0
retval = nf90_put_att(ncid, temp_varid, "_FillValue", fill_value)
if (retval /= nf90_noerr) call handle_err(retval)
```
`@/home/ed/NEP/examples/f_classic/f_coord_vars.f90:131-133`

Writing `_FillValue` directly is equivalent to calling `nc_def_var_fill()` with `NC_FILL`; the library implementation deletes any existing `_FillValue` attribute and writes a new one with the given value. Use `nc_def_var_fill()` when you also need to control the no-fill flag; use `nc_put_att_*` when the CF attribute pattern is more readable in context. Either way, the resulting file is identical.

## NetCDF-4 Attribute Functions

NetCDF-4 (HDF5-backed files) extends the classic type set with six additional atomic types. The C API adds a typed function for each one. Fortran continues to use the same overloaded `nf90_put_att` and `nf90_get_att`; the compiler selects the correct implementation from the type of the value argument.

### New Types in NetCDF-4

The six new types and their C write/read functions are:

| NetCDF-4 type | C type | C write function | C read function |
|---|---|---|---|
| `NC_UBYTE` | `unsigned char` | `nc_put_att_ubyte` | `nc_get_att_ubyte` |
| `NC_USHORT` | `unsigned short` | `nc_put_att_ushort` | `nc_get_att_ushort` |
| `NC_UINT` | `unsigned int` | `nc_put_att_uint` | `nc_get_att_uint` |
| `NC_INT64` | `long long` | `nc_put_att_longlong` | `nc_get_att_longlong` |
| `NC_UINT64` | `unsigned long long` | `nc_put_att_ulonglong` | `nc_get_att_ulonglong` |
| `NC_STRING` | `char **` | `nc_put_att_string` | `nc_get_att_string` |

All six are available only in NetCDF-4 format files. Attempting to use them on a classic-format file returns `NC_ENOTNC4`.

### Strings

`NC_STRING` stores variable-length character strings, unlike `NC_CHAR` which stores fixed-length character arrays. Attributes of type `NC_STRING` are arrays of pointers; the library allocates the string memory on read and you must free it with `nc_free_string()`. `dump_nc4_metadata.c` shows the pattern:

```c
char **val = malloc(len * sizeof(char *));
if ((retval = nc_get_att_string(ncid, varid, att_name, val)))
    ERR(retval);
for (i = 0; i < len; i++)
    printf("%s\"%s\"", i ? ", " : "", val[i] ? val[i] : "(null)");
nc_free_string(len, val);
free(val);
```
`@/home/ed/NEP/examples/netcdf-4/dump_nc4_metadata.c:170-181`

In Fortran, `NC_STRING` variables and attributes are not handled portably by the Fortran 90 interface. The Fortran `dump_nc4_metadata` example notes this explicitly and reports string-typed attributes by type name only, without reading their values.
`@/home/ed/NEP/examples/f_netcdf-4/f_dump_nc4_metadata.f90:11-13`

### Reading Attributes of Unknown Type

When reading a file whose attribute types are not known in advance, use `nc_inq_att()` (C) or `nf90_inquire_attribute()` (Fortran) to retrieve the type and length, then dispatch to the appropriate typed getter. `dump_nc4_metadata.c` does this for every attribute it encounters, covering all classic and NetCDF-4 types including `NC_STRING`.
`@/home/ed/NEP/examples/netcdf-4/dump_nc4_metadata.c:74-189`
