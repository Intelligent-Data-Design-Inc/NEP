# Magic Number Handling Problems in NetCDF-C User-Defined Formats

## Summary

NetCDF-C's User-Defined Format (UDF) mechanism uses magic-number strings to
automatically route files to the correct dispatch handler. NEP implements five
UDF handlers (GeoTIFF, GRIB2, FITS, CDF, PDS4), each registered in the `.ncrc`
file with a `NETCDF.UDFx.MAGIC=` entry. For most formats this works, but the
NASA CDF magic number contains an embedded null byte, and the NetCDF-C APIs
that process magic numbers are not binary-safe. This prevents CDF files from
being auto-detected when opened through tools such as `ncdump`.

## The Magic Numbers

NEP assigns one UDF slot per format and uses a short byte sequence to identify
files:

| Slot | Format   | Magic bytes                                | Text-safe |
|------|----------|--------------------------------------------|-----------|
| UDF0 | GeoTIFF BigTIFF | `II+`                               | yes |
| UDF1 | GeoTIFF standard TIFF | `II*`                         | yes |
| UDF2 | GRIB2    | `GRIB`                                     | yes |
| UDF3 | FITS     | `SIMPLE`                                   | yes |
| UDF4 | NASA CDF | `0xCD 0xF3 0x00 0x01` (`0xCDF30001`)       | **no** |
| UDF5 | PDS4     | `<?xml`                                    | yes |

`NEP_MAGIC_CDF` is defined in `include/nep.h`:

```c
#define NEP_MAGIC_CDF "\xCD\xF3\x00\x01"
```

The null byte at position 2 is part of the official CDF file signature.

## Where the Problem Appears

### 1. `nc_def_user_format()` is not binary-safe

`libdispatch/dfile.c` in NetCDF-C stores magic numbers with `strlen()` and
`strncpy()`:

```c
if (magic_number && strlen(magic_number) > NC_MAX_MAGIC_NUMBER_LEN)
    return NC_EINVAL;
...
if (magic_number) {
    strncpy(UDF_magic_numbers[udf_index], magic_number, NC_MAX_MAGIC_NUMBER_LEN);
    UDF_magic_numbers[udf_index][NC_MAX_MAGIC_NUMBER_LEN] = '\0';
}
```

Both functions stop at the first null byte, so only `0xCD 0xF3` is stored
instead of the full 4-byte CDF signature.

### 2. `.ncrc` magic values cannot contain binary bytes

The RC file parser in `libdispatch/dudfplugins.c` passes the text value of
`NETCDF.UDFx.MAGIC` directly to `nc_def_user_format()`. If a user writes:

```ini
NETCDF.UDF4.MAGIC=\xCD\xF3\x00\x01
```

the parser delivers the literal 14-character string `\xCD\xF3\x00\x01`,
which exceeds `NC_MAX_MAGIC_NUMBER_LEN` (8) and causes registration to fail
with `NC_EINVAL`. There is currently no hex-unescape step.

Because of this, NEP's `.ncrc` template for CDF deliberately omits the
`NETCDF.UDF4.MAGIC` line:

```text
# NASA CDF format (UDF slot 4)
NETCDF.UDF4.LIBRARY=@NEP_LIBDIR@/libnccdf.so
NETCDF.UDF4.INIT=NC_CDF_initialize
```

Without a magic mapping, NetCDF-C has no way to auto-route an `nc_open()` call
to `libnccdf.so`.

### 3. UDF magic matches are discarded during format inference

Even if the magic were registered correctly, `NC_interpret_magic_number()` in
`libdispatch/dinfermodel.c` sets `model->impl` but leaves `model->format` as 0.
`check_file_type()` then treats the match as a failure and returns
`NC_ENOTNC` ("Unknown file format").

## Impact on NEP

- `ncdump`, `nccopy`, and any other tool that relies on auto-detection cannot
  open NASA CDF files through the NEP UDF handler.
- CDF support only works when the calling program explicitly registers the
  handler with `nc_def_user_format(NEP_UDF_CDF, CDF_dispatch_table,
  "\xCD\xF3\x00\x01")` before calling `nc_open()`. This is what NEP's own test
  programs do.
- The `.ncrc` autoload mechanism works for GeoTIFF, GRIB2, FITS, and PDS4,
  but not for CDF.

## Proposed Solution

Fix the problem in NetCDF-C so that magic numbers are treated as length-value
byte sequences rather than null-terminated C strings.

### Upstream NetCDF-C changes

1. **Make `nc_def_user_format()` binary-safe.**
   Replace the `strlen()` check with an explicit length argument, or always
   copy exactly `NC_MAX_MAGIC_NUMBER_LEN` bytes with `memcpy()`:

   ```c
   int nc_def_user_format(int mode_flag, NC_Dispatch *dispatch_table,
                          char *magic_number, size_t magic_len);
   ```

   or, as a minimal non-breaking change:

   ```c
   if (magic_number) {
       memcpy(UDF_magic_numbers[udf_index], magic_number, NC_MAX_MAGIC_NUMBER_LEN);
   }
   ```

2. **Preserve the UDF match in format inference.**
   In `NC_interpret_magic_number()`, set `model->format` when a UDF magic
   matches:

   ```c
   model->impl = NC_FORMATX_UDF0 + i;
   model->format = NC_FORMAT_CLASSIC;
   ```

3. **Support hex escapes in `.ncrc` magic values.**
   In `load_udf_plugin()` / `NC_udf_load_plugins()`, unescape `\xNN` sequences
   before passing the magic to `nc_def_user_format()`. This allows:

   ```ini
   NETCDF.UDF4.MAGIC=\xCD\xF3\x00\x01
   ```

   to be interpreted as the four-byte sequence rather than a 14-character
   literal.

### NEP changes after the upstream fix

1. Add the CDF magic line to the `.ncrc` fragment template
   `.ncrc-cdf.in`:

   ```ini
   NETCDF.UDF4.MAGIC=\xCD\xF3\x00\x01
   ```

2. Update `NC_CDF_initialize()` in `src/cdfdispatch.c` to register the
   handler at runtime, as the abstract already describes:

   ```c
   int NC_CDF_initialize(void) {
       return nc_def_user_format(NEP_UDF_CDF, &CDF_dispatcher,
                                 NEP_MAGIC_CDF);
   }
   ```

   If the upstream API gains a length argument, pass the known length (4)
   instead of relying on `strlen()`.

3. Update `docs/NEP_abstract.md` and any generated `.ncrc` examples so that the
   CDF entry includes the escaped magic number.

## Workaround Until Upstream is Fixed

Applications and tests must register the CDF handler explicitly before opening
a CDF file:

```c
char cdf_magic[5] = "\xCD\xF3\x00\x01";
nc_def_user_format(NEP_UDF_CDF, (NC_Dispatch *)CDF_dispatch_table, cdf_magic);
```

This is the approach currently used by `test/tst_cdf_udf.c` and
`test/tst_imap_mag.c`.

## References

- `include/nep.h` — `NEP_MAGIC_CDF` definition
- `src/cdfdispatch.c` — `NC_CDF_initialize()` (currently does not register)
- `.ncrc-cdf.in` — CDF `.ncrc` fragment without a magic line
- `docs/netcdf-c-cdf-magic-improvements.md` — detailed analysis of the upstream
  NetCDF-C limitations
- `test/tst_cdf_udf.c` and `test/tst_imap_mag.c` — explicit CDF registration
  in tests
