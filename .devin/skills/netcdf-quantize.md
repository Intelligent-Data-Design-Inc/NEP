# NetCDF Compression with Quantization

## Overview

Quantization is a **lossy pre-filter** that zeroes excess bits of the floating-point significand before lossless compression (e.g. DEFLATE/zlib). It dramatically improves compression ratios at the cost of controlled, bounded precision loss.

Key constraints:
- Only applies to `NC_FLOAT` and `NC_DOUBLE` variables
- Must be set **after** `nc_def_var()` and **before** `nc_enddef()` (or the first write/read for netCDF-4/HDF5 files, which call `nc_enddef()` automatically)
- Quantization alone does **not** reduce file size — lossless compression must also be enabled

---

## Quantization Algorithms

Three algorithms are available, selected by the `quantize_mode` constant:

| Constant | C Name | Fortran 90 Name | NSD meaning | NSD range (float) | NSD range (double) |
|----------|--------|-----------------|-------------|-------------------|-------------------|
| BitGroom | `NC_QUANTIZE_BITGROOM` | `nf90_quantize_bitgroom` | Decimal digits | 1–7 | 1–15 |
| Granular BitRound | `NC_QUANTIZE_GRANULARBR` | `nf90_quantize_granularbr` | Decimal digits | 1–7 | 1–15 |
| BitRound | `NC_QUANTIZE_BITROUND` | `nf90_quantize_bitround` | Binary bits | 1–23 | 1–52 |

### BitGroom
- Determines the number of bits needed for NSD significant decimal digits once, then applies that mask uniformly across all values
- Alternates excess bits between 0 and 1 on successive array elements, preserving the mean
- Relative error guaranteed within `0.5 * V * 2^{-NSB}`

### Granular BitRound
- Computes the required bits **per value** using IEEE rounding, more aggressively reducing bits where values permit
- Better overall compression ratio than BitGroom, but slightly higher worst-case error margin
- Free from the multipoint-statistics artifacts that BitGroom can introduce

### BitRound
- User specifies the number of **binary** significand bits to retain (NSB), not decimal digits
- Same speed as BitGroom
- Free from BitGroom multipoint artifacts (see doi:10.5194/gmd-14-377-2021)

### NSD ↔ NSB correspondence for BitRound (to match BitGroom decimal precision)

| NSD | 1 | 2 | 3 | 4  | 5  | 6  | 7  |
|-----|---|---|---|----|----|----|-----|
| NSB | 3 | 6 | 9 | 13 | 16 | 19 | 23 |

### Error margins by algorithm and NSD

| NSD | 1 | 2 | 3 | 4 | 5 | 6 |
|-----|---|---|---|---|---|---|
| BitGroom error | 3.1e-2 | 3.9e-3 | 4.9e-4 | 3.1e-5 | 3.8e-6 | 4.7e-7 |
| GranularBitRound error | 1.4e-1 | 1.9e-2 | 2.2e-3 | 1.4e-4 | 1.8e-5 | 2.2e-6 |

---

## Variable Attributes Added by Quantization

When quantization is applied, the library automatically adds an integer attribute to the variable encoding the NSD/NSB value. These are library-managed attributes (underscore prefix) and must not be modified or deleted by users.

| Algorithm | Attribute Name |
|-----------|---------------|
| BitGroom | `_QuantizeBitGroomNumberOfSignificantDigits` |
| Granular BitRound | `_QuantizeGranularBitRoundNumberOfSignificantDigits` |
| BitRound | `_QuantizeBitRoundNumberOfSignificantBits` |

---

## Fill Value Handling

Fill values are **not quantized**. Array elements equal to `_FillValue` (or the type default fill value) are left unmodified so they continue to serve as missing-data indicators.

---

## C API

```c
/* Set quantization mode on a variable */
int nc_def_var_quantize(int ncid, int varid, int quantize_mode, int nsd);

/* Inquire whether quantization is set (optional, for reader convenience) */
int nc_inq_var_quantize(int ncid, int varid, int *quantize_modep, int *nsdp);
```

### Full C example

```c
/* Variables must be NC_FLOAT or NC_DOUBLE */
if (nc_def_var(ncid, "var1", NC_FLOAT,  NDIM1, &dimid, &varid1)) ERR;
if (nc_def_var(ncid, "var2", NC_DOUBLE, NDIM1, &dimid, &varid2)) ERR;

/* Enable quantization — 3 significant decimal digits */
if (nc_def_var_quantize(ncid, varid1, NC_QUANTIZE_BITGROOM, 3)) ERR;
if (nc_def_var_quantize(ncid, varid2, NC_QUANTIZE_BITGROOM, 3)) ERR;

/* Enable zlib compression (level 1 is usually optimal) */
/* shuffle=0, deflate=1, deflate_level=1 */
if (nc_def_var_deflate(ncid, varid1, 0, 1, 1)) ERR;
if (nc_def_var_deflate(ncid, varid2, 0, 1, 1)) ERR;
```

---

## Fortran 90 API

Quantization is passed as optional arguments to `nf90_def_var()`:

```fortran
call check(nf90_def_var(ncid, "var1", NF90_FLOAT, dimids, varid1, &
     deflate_level = 1, quantize_mode = nf90_quantize_bitgroom, nsd = 3))

call check(nf90_def_var(ncid, "var2", NF90_DOUBLE, dimids, varid2, &
     contiguous = .TRUE., quantize_mode = nf90_quantize_bitgroom, nsd = 3))
```

---

## Fortran 77 API

Separate functions mirror the C API:

```fortran
retval = nf_def_var_quantize(ncid, varid, NF_QUANTIZE_BITGROOM, NSD_3)
if (retval .ne. nf_noerr) stop 3

retval = nf_def_var_deflate(ncid, varid, 0, 1, 1)
if (retval .ne. nf_noerr) stop 3
```

---

## Known Bug: NC_QUANTIZE_MAX_FLOAT_NSD = 7 (NetCDF-C ≤ 4.10.0)

`netcdf.h` defines `NC_QUANTIZE_MAX_FLOAT_NSD (7)` but this value is incorrect
for `NC_FLOAT` and triggers data corruption in BitGroom:

- `NC_FLOAT` has a 23-bit significand → max meaningful decimal digits =
  `floor(log10(2^23)) ≈ 6.92` → **correct max NSD = 6**
- At NSD=7: `bits_to_keep = floor(7 × log2(10)) = 23`, so `bits_to_zero = 0`
- The BitGroom alternating-mask path breaks at `bits_to_zero = 0`:
  even-indexed values round to `2.0`, odd-indexed values become `NaN`
- Results in `max_abs_err ≈ 330` on a 280–332 K dataset instead of near-zero
- `GranularBitRound` and `BitRound` do **not** exhibit this bug at NSD/NSB=7

**Workaround**: Cap NSD at 6 for `NC_FLOAT` with BitGroom and GranularBitRound.
`NC_QUANTIZE_MAX_DOUBLE_NSD (15)` is correct for `NC_DOUBLE` (52-bit significand,
`floor(15 × log2(10)) = 49`, so `bits_to_zero = 3`).

This is a bug in NetCDF-C and should be reported upstream. The library fix is
either to lower the constant to 6, or guard the alternating-mask path with
`if (bits_to_zero > 0)`.

## Performance Notes

- Quantization + DEFLATE level 1 is the recommended combination
- Compression level 1 is usually the best choice (speed vs. ratio trade-off) when combined with quantization
- E3SM Atmosphere Model benchmark (445 MB raw): quantization pre-filters can yield substantially higher compression ratios than DEFLATE alone, with the ratio increasing as NSD decreases (precision decreases)
- GranularBitRound typically achieves the best compression ratio among the three algorithms

---

## HDF5 Plugin Path

Quantization uses the standard NetCDF-C filter mechanism. Lossless filters such
as Zstandard, LZ4, and BZIP2 are dynamically loaded HDF5 plugins. If they
cannot be found at runtime, `nc_create`/`nc_open` returns `NC_ENOFILTER` or
`NetCDF: HDF error`.

### Discovery rules (NetCDF-C ≥ 4.9.3)

1. If `HDF5_PLUGIN_PATH` env var is set, it is used as the plugin search path
   (colon-separated on Linux).
2. If unset, the compiled-in default is used: typically `/usr/local/hdf5/lib/plugin`.
3. The global path is shared between HDF5 and NCZarr dispatchers.
4. **Set before any `nc_open` or `nc_create` call** — changing it later may
   have no effect because HDF5 caches the path on first use.

### Programmatic API (netcdf_filter.h, ≥ 4.9.3)

```c
int nc_plugin_path_ndirs(size_t *ndirsp);
int nc_plugin_path_get(NCPluginList *dirs);
int nc_plugin_path_set(const NCPluginList *dirs);
```

### Local machine (Ed's dev box)

All HDF5 filter plugins (bzip2, lz4, zstandard, szip, deflate, shuffle, …)
are installed at:

```
/usr/local/hdf5/lib/plugin/
```

This is the compiled-in default for the local NetCDF-C 4.10.0 install, so
`HDF5_PLUGIN_PATH` does not need to be set explicitly when running against
`/usr/local/netcdf-c-4.10.0`. However, the full runtime `LD_LIBRARY_PATH`
must include all dependency libraries:

```bash
export LD_LIBRARY_PATH=/home/ed/NEP/build/src:/usr/local/hdf5-2.1.0/lib:/usr/local/netcdf-c-4.10.0/lib:/usr/local/jasper-3.0.3/lib:$LD_LIBRARY_PATH
```

If running a program that uses LZ4 or BZIP2 and plugins are not found, set:

```bash
export HDF5_PLUGIN_PATH=/usr/local/hdf5/lib/plugin
```

---

## References

- Zender, C. S. (2016), Bit Grooming: doi:10.5194/gmd-9-3199-2016
- Delaunay et al. (2019), Evaluation of lossless and lossy algorithms: doi:10.5194/gmd-2018-250
- Htrenka et al. (2021), BitRound: doi:10.5194/gmd-14-377-2021
- Hartnett et al., netcdf-c GitHub Issue #1548
- Unidata quantize docs: https://docs.unidata.ucar.edu/netcdf-c/current/netcdf_quantize.html
- Unidata plugin path docs: https://docs.unidata.ucar.edu/netcdf-c/current/pluginpaths.html
