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

## Problem 4: Broken `mode_flag` computation for UDF slots ≥ 3

### Location
`libdispatch/dudfplugins.c`, function `load_udf_plugin()`

### Current code (line ~120)
```c
if (udf_number == 0)
    mode_flag = NC_UDF0;
else if (udf_number == 1)
    mode_flag = NC_UDF1;
else
    mode_flag = NC_UDF2 << (udf_number - 2);
```

### Problem
The `NC_UDFn` mode flags are **not** a simple bit-shift sequence:

```c
#define NC_UDF0   0x000040   /* bit 6  */
#define NC_UDF1   0x000080   /* bit 7  */
#define NC_UDF2   0x010000   /* bit 16 */
#define NC_UDF3   0x080000   /* bit 19 */
#define NC_UDF4   0x100000   /* bit 20 */
...
```

For `udf_number=3`, the expression `NC_UDF2 << (3-2) = 0x020000`, but
`NC_UDF3 = 0x080000`. `udf_mode_to_index()` in `dfile.c` finds no matching
flag, returns -1, and `nc_def_user_format()` returns `NC_EINVAL` (-36).

This means **any plugin in UDF slot 3 or higher cannot be loaded** from
`.ncrc` autoloading. The error surfaces as:
```
ERR: Failed to register dispatch table for UDF3: -36
```

### Fix
Replace the shift expression with a lookup table:

```c
static const int udf_mode_flags[] = {
    NC_UDF0, NC_UDF1, NC_UDF2, NC_UDF3, NC_UDF4,
    NC_UDF5, NC_UDF6, NC_UDF7, NC_UDF8, NC_UDF9
};
if (udf_number < 0 || udf_number >= NC_MAX_UDF_FORMATS) {
    stat = NC_EINVAL;
    goto done;
}
mode_flag = udf_mode_flags[udf_number];
```

### NEP Workaround (without changing netcdf-c)

Since slots 0 and 1 are hardcoded correctly and slot 2 is correct
(`NC_UDF2 << 0 = NC_UDF2`), only slots ≥ 3 are broken.

NEP originally assigned:
- UDF0, UDF1 → GeoTIFF
- UDF2 → CDF
- UDF3 → GRIB2  ← broken slot

Three options were considered:

**Option A — Reassign slots so GRIB2 uses slot 2:**
Swap CDF and GRIB2 in `.ncrc` and update `NC_FORMATX_NC_CDF` /
`NC_FORMATX_NC_GRIB2` defines accordingly. This keeps all active plugins
within the working slots (0–2). CDF becomes non-default (opt-in only),
making it and GRIB2 mutually exclusive occupants of slot 2.

**Option B — Reassign GeoTIFF to one slot, freeing slot 1 or 2 for GRIB2:**
GeoTIFF currently uses two slots (0 and 1) because it handles two TIFF
magic variants (`II*` and `II+`). If the GeoTIFF dispatch handler is
extended to check both magic values internally, it could occupy a single
slot, freeing one slot for GRIB2 within the working range.

**Option C — Self-register at process startup using `nc_def_user_format()` directly:**
Each NEP dispatch library exports an `__attribute__((constructor))` function
that calls `nc_def_user_format(NC_UDF3, &GRIB2_dispatcher, "GRIB")` with the
correct flag directly, bypassing `dudfplugins.c` entirely. This requires no
changes to netcdf-c and works correctly with any UDF slot number, but creates
a tight coupling between the shared library and the running netcdf-c dispatch
system at `dlopen()` time.

**Implemented solution: Option A.**

NEP v1.7.0 adopts Option A. GRIB2 moves to UDF slot 2. CDF is disabled by
default and can be enabled with `--enable-cdf` (Autotools) or
`-DENABLE_CDF=ON` (CMake), but is mutually exclusive with GRIB2 since both
use slot 2. The slot assignment in `include/nep.h` is:

```
UDF0: GeoTIFF BigTIFF  (magic: "II+")
UDF1: GeoTIFF standard TIFF  (magic: "II*")
UDF2: GRIB2  (magic: "GRIB")  [default]
      or CDF  (magic: 0xCDF30001)  [if built with --enable-cdf; mutually exclusive]
```

This keeps all active plugins within the working range (slots 0–2) and
requires no changes to netcdf-c. Option C (the `__attribute__((constructor))`
approach) was prototyped but abandoned: it required the GRIB2 library to call
into netcdf-c during `dlopen()`, which is fragile and depends on internal
netcdf-c state being initialized at that point.

> **Note:** Problem 4 (`dudfplugins.c` bit-shift bug) is a genuine bug in
> NetCDF-C 4.10.0. I intend to submit a fix upstream so that future releases
> of NetCDF-C support all 10 UDF slots correctly from `.ncrc` autoloading,
> at which point the slot 2 mutual-exclusion constraint between GRIB2 and CDF
> can be removed and each format can occupy its own dedicated slot.

---

## Summary of Required Changes

| File | Change | Impact |
|------|--------|--------|
| `libdispatch/dfile.c` | Use `memcpy` instead of `strncpy` for magic storage | Magic with null bytes stored correctly |
| `libdispatch/dinfermodel.c` | Set `model->format = NC_FORMAT_CLASSIC` in UDF magic match | UDF files auto-detected without explicit mode flag |
| `libdispatch/dudfplugins.c` | Unescape `\xNN` sequences from RC magic values | Binary magic expressible in `.ncrc` |
| `libdispatch/dudfplugins.c` | Replace bit-shift `mode_flag` with lookup table | UDF slots 3–9 load correctly from `.ncrc` |

Problems 2 and 3 together are the reason `ncdump` cannot open CDF files via
`.ncrc` autoloading in NetCDF-C 4.10.0. Problem 4 is the reason GRIB2 (and
any plugin in slot 3+) fails to register from `.ncrc` autoloading.
