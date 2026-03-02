# NetCDF-C Changes Needed for Better CDF Magic Number Support

## Background

The NASA CDF (Common Data Format) magic number is a 4-byte binary sequence:

```
0xCD 0xF3 0x00 0x01
```

The third byte is a null byte (`0x00`). This causes two distinct problems in
NetCDF-C 4.10.0 that prevent CDF files from being auto-detected when opened
via `nc_open()` without an explicit `NC_UDF2` mode flag (e.g. from `ncdump`).

---

## Problem 1: `strlen()` truncates magic numbers containing null bytes

### Location
`libdispatch/dfile.c`, function `nc_def_user_format()`

### Current code (line ~173)
```c
if (magic_number && strlen(magic_number) > NC_MAX_MAGIC_NUMBER_LEN)
    return NC_EINVAL;
```
and (line ~195):
```c
if (magic_number) {
    strncpy(UDF_magic_numbers[udf_index], magic_number, NC_MAX_MAGIC_NUMBER_LEN);
}
```

### Problem
Both `strlen()` and `strncpy()` treat the null byte at index 2 as a string
terminator, so only 2 bytes (`0xCD 0xF3`) are stored instead of the full
4-byte magic. While 2 bytes may be sufficient for detection, the API silently
truncates without warning, and callers cannot register the full 4-byte magic
as documented.

### Fix
Accept a separate `magic_len` parameter (or use `memcpy` with a fixed
`NC_MAX_MAGIC_NUMBER_LEN` copy):

```c
/* Replace strlen check with an explicit length parameter */
int nc_def_user_format(int mode_flag, NC_Dispatch *dispatch_table,
                       char *magic_number, size_t magic_len);
```

Or, as a minimal non-breaking change, always copy exactly
`NC_MAX_MAGIC_NUMBER_LEN` bytes regardless of embedded nulls:

```c
if (magic_number) {
    memcpy(UDF_magic_numbers[udf_index], magic_number, NC_MAX_MAGIC_NUMBER_LEN);
}
```

and replace the `strlen` length check with an explicit `magic_len` argument
or drop it entirely (the buffer is fixed-size).

---

## Problem 2: UDF magic detection sets `model->impl` but not `model->format`

### Location
`libdispatch/dinfermodel.c`, function `NC_interpret_magic_number()`

### Current code (approximately lines 1570–1579)
```c
for (i = 0; i < NC_MAX_UDF_FORMATS; i++) {
    if (strlen(UDF_magic_numbers[i]) > 0
        && memcmp(magic, UDF_magic_numbers[i],
                  strlen(UDF_magic_numbers[i])) == 0) {
        model->impl = NC_FORMATX_UDF0 + i;  /* sets impl ... */
        goto done;                           /* ... but NOT model->format */
    }
}
```

### Problem
After the UDF magic match, `model->format` is left as `0`. The calling
function `check_file_type()` checks `model->format != 0` to decide whether
format inference succeeded. Because `model->format == 0`, it discards the
UDF match and falls through to report `NC_ENOTNC` ("Unknown file format").

This is the root cause of `ncdump` failing to open CDF files even when the
plugin is loaded via `.ncrc`.

### Fix
Set `model->format` to `NC_FORMAT_CLASSIC` (the default for UDFs that don't
set `NC_NETCDF4`) alongside `model->impl`:

```c
for (i = 0; i < NC_MAX_UDF_FORMATS; i++) {
    if (strlen(UDF_magic_numbers[i]) > 0
        && memcmp(magic, UDF_magic_numbers[i],
                  strlen(UDF_magic_numbers[i])) == 0) {
        model->impl = NC_FORMATX_UDF0 + i;
        model->format = NC_FORMAT_CLASSIC;  /* ADD THIS LINE */
        goto done;
    }
}
```

---

## Problem 3: RC file magic strings cannot represent binary bytes

### Location
`libdispatch/dudfplugins.c`, function `load_udf_plugin()` / `NC_udf_load_plugins()`

### Problem
The `.ncrc` file is a text file. The magic value read by `NC_rclookup()` is
passed as-is (no unescaping) to `nc_def_user_format()`. There is no way to
represent the binary byte `0x00` in a text RC file value.

If a user writes:
```
NETCDF.UDF2.MAGIC=\xCD\xF3\x00\x01
```
the RC parser delivers the 14-character literal string `\xCD\xF3\x00\x01`
to `nc_def_user_format()`. This exceeds `NC_MAX_MAGIC_NUMBER_LEN` (8) and
returns `NC_EINVAL` (-36).

### Fix
Add `\xNN` hex unescape processing in `load_udf_plugin()` before passing the
magic string to `nc_def_user_format()`:

```c
static void
unescape_magic(char *buf, const char *src, size_t buflen)
{
    size_t out = 0;
    while (*src && out < buflen) {
        if (src[0] == '\\' && src[1] == 'x' &&
            isxdigit((unsigned char)src[2]) &&
            isxdigit((unsigned char)src[3])) {
            char hex[3] = { src[2], src[3], '\0' };
            buf[out++] = (char)(unsigned char)strtol(hex, NULL, 16);
            src += 4;
        } else {
            buf[out++] = *src++;
        }
    }
    /* pad remainder with zeros */
    while (out < buflen) buf[out++] = '\0';
}
```

Then in `load_udf_plugin()`, before calling `nc_def_user_format()`:

```c
char unescaped_magic[NC_MAX_MAGIC_NUMBER_LEN + 1] = {0};
const char *effective_magic = NULL;
if (magic) {
    unescape_magic(unescaped_magic, magic, NC_MAX_MAGIC_NUMBER_LEN);
    effective_magic = unescaped_magic;
}
if ((stat = nc_def_user_format(mode_flag, table, (char*)effective_magic)))
    ...
```

---

## Summary of Required Changes

| File | Change | Impact |
|------|--------|--------|
| `libdispatch/dfile.c` | Use `memcpy` instead of `strncpy` for magic storage | Magic with null bytes stored correctly |
| `libdispatch/dinfermodel.c` | Set `model->format = NC_FORMAT_CLASSIC` in UDF magic match | UDF files auto-detected without explicit mode flag |
| `libdispatch/dudfplugins.c` | Unescape `\xNN` sequences from RC magic values | Binary magic expressible in `.ncrc` |

Problems 2 and 3 together are the reason `ncdump` cannot open CDF files via
`.ncrc` autoloading in NetCDF-C 4.10.0.
