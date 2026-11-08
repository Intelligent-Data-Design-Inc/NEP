# NEXTCDF-4 float16 Storage

NEXTCDF-4 supports two 16-bit floating-point atomic types: `NC_FLOAT16` (IEEE 754 binary16) and `NC_BFLOAT16` (brain floating point). Both store 2 bytes per value on disk, but they split the bits differently and suit different data.

| Property | `NC_FLOAT16` | `NC_BFLOAT16` |
|----------|--------------|---------------|
| `nc_type` value | 17 (`include/nep.h`) | 18 (`include/nep.h`) |
| Layout | 1 sign, 5 exponent, 10 mantissa bits | 1 sign, 8 exponent, 7 mantissa bits |
| Range | up to 65504 | float32 range, up to ~3.39e38 |
| Precision | ~3 significant decimal digits | ~2-3 significant decimal digits |
| File type | `H5T_IEEE_F16LE` | `H5T_FLOAT_BFLOAT16LE` |
| Memory type | raw `uint16_t` (untyped calls), or `float`/`double` (typed calls) | the same |

Even smaller floats exist — `NC_FLOAT8_E4M3`, `NC_FLOAT8_E5M2`, `NC_FLOAT6_E2M3`, `NC_FLOAT6_E3M2` and `NC_FLOAT4_E2M1` are stored as raw `uint8_t` patterns; see the type table in [nextcdf4.md](nextcdf4.md).

## Rules

- **Native mode only.** Small floating-point types are rejected with `NC_ENOTNC4` in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL` mode (`NEXTCDF4_check_atomic_type` in `src/nextcdf4/nxt4meta.c`).
- **No coordinate variables.** Small floating-point types are not supported as coordinate variables by design (v4.0.0 Sprint 9). The library does not currently reject a 1-D variable whose name matches its dimension, and reopening such a file is untested, so do not create them yet ([issue #380](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/380)).
- **Raw bits or float/double.** Untyped calls exchange raw bit patterns. For `NC_FLOAT16` and `NC_BFLOAT16`, the typed `float` and `double` calls convert through HDF5. Reads are exact.
- **HDF5 write rounding.** float16 rounds ties to even, and any value above 65504 becomes +inf. bfloat16 rounds ties away from zero; finite float input past the largest bfloat16 saturates to `0x7f7f`, and double input past the float range becomes inf. NaN becomes `0x7fff` for bfloat16 and `0x7e00` for float16. There is no `NC_ERANGE` ([issue #381](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/381)). To get round-to-nearest-even, use the `nc_*_bfloat16` functions, which convert in NEP (see below), or convert in user code (`bfloat16_conv.h`).
- **Other typed calls are rejected.** Integer, string and other typed calls return `NC_EBADTYPE`; the `_text` calls return `NC_ECHAR`.
- **FP8/FP6/FP4: untyped calls only.** All typed calls are rejected, because HDF5 2.1.1 converts these types incorrectly (for example, E2M1 6.0 is stored as infinity). See HDF5 2.1.1 "Known Problems".
- **No fill value.** `_FillValue` attributes on `NC_BFLOAT16` variables are rejected, and a fill value set with `nc_def_var_fill` is not recovered on reopen ([issue #377](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/377)). Define small-float variables without a fill value.
- NEXTCDF-4 itself requires HDF5 2.1.1, so both types are available whenever NEXTCDF-4 is built. (The per-type minimums are HDF5 1.14 for `NC_FLOAT16` and HDF5 2.1.1 for `NC_BFLOAT16`.)

## Why bfloat16 for the GDAS sample

bfloat16 keeps the full float32 exponent range and pays for it with a 7-bit mantissa, so it suits data where a relative error of up to 2^-8 (about 0.4%) is acceptable. IEEE binary16 makes the opposite trade: 3 more mantissa bits (for example, values 0.25 K apart in 256-512 K instead of 2 K), but a maximum of 65504 and a smallest normal value of about 6.1e-5, below which precision drops. The non-fill values in this sample (`tmp2m` 218.976-316.82 K, `prate_ave` 0-0.0109397 kg/m**2/s, `soilw1` 0.02-1, `land` 0-2) all fit within binary16's maximum, but the input's declared fill value `9.99e+20` and any small `prate_ave` values below 6.1e-5 do not fit binary16 well, while bfloat16 represents both. (No point in this sample actually equals the fill value.) In absolute terms the maximum errors on the four GDAS fields are 1 K for `tmp2m` (bfloat16 values are 1 K apart in 128-256 K and 2 K apart in 256-512 K, so `tmp2m` is stored to the nearest 1-2 K), 3.0e-05 kg/m**2/s for `prate_ave`, 9.8e-04 (fraction) for `soilw1`, and 0 for `land`, whose three small integer values are exactly representable.

## bfloat16 I/O functions

`libnep` provides eight bfloat16 memory-type functions that work on any file the netCDF API can open, not just NEXTCDF-4:

```c
nc_get_var_bfloat16   nc_put_var_bfloat16
nc_get_var1_bfloat16  nc_put_var1_bfloat16
nc_get_vara_bfloat16  nc_put_vara_bfloat16
nc_get_vars_bfloat16  nc_put_vars_bfloat16
```

The buffer type is `nc_bfloat16_t`: the compiler's `__bf16` where available (`NEP_HAVE_NATIVE_BFLOAT16` = 1), otherwise `uint16_t`. Both are 2 bytes holding the bfloat16 bit pattern, so raw-bits code casts the pointer — `(nc_bfloat16_t *)(void *)bits` — rather than assigning values.

What the functions do depends on the variable's type:

| Variable type | Behavior |
|---------------|----------|
| `NC_BFLOAT16` | Raw bit patterns, no conversion |
| `NC_FLOAT`, `NC_DOUBLE` | NEP converts: reads round to nearest, ties to even (single rounding from double); writes widen exactly |
| `NC_CHAR` | `NC_ECHAR` |
| anything else (incl. `NC_FLOAT16`) | `NC_EBADTYPE` |

On a get, a finite float or double whose rounding overflows is stored as ±inf, every value is still converted, and the call returns `NC_ERANGE`. Infinity and NaN are not range errors, and puts never overflow because widening is exact.

```c
nc_bfloat16_t btmp[NVALS];

/* Read a float variable as bfloat16, converted by NEP. */
ret = nc_get_vara_bfloat16(ncid, varid, start, count, btmp);
if (ret == NC_ERANGE)
    fprintf(stderr, "some values overflowed to bfloat16 infinity\n");
```

## Example: `examples/nextcdf4/bfloat16.c`

`bfloat16.c` reads the `tmp2m` field and its `grid_yt`/`grid_xt` coordinate variables from the GDAS sample — twice: as `NC_FLOAT` for reference and as bfloat16 through `nc_get_var_bfloat16`, so NEP does the conversion — writes two NEXTCDF-4 files — `float_tmp2m.nc` (`NC_FLOAT`) and `bfloat16_tmp2m.nc` (`NC_BFLOAT16`) — then reopens the bfloat16 file, verifies the bits round-tripped exactly, and reports file sizes and the maximum conversion error.

### Conversion (bfloat16_conv.h)

float32 to bfloat16 rounds to nearest, ties to even; NaN stays a quiet NaN and values past the maximum overflow to infinity. bfloat16 to float32 is an exact 16-bit left shift. The `nc_*_bfloat16` functions use the same rounding; the program cross-checks `nc_get_var_bfloat16` against `float_to_bfloat16` on every point:

```c
static inline uint16_t
float_to_bfloat16(float f)
{
    uint32_t u;

    memcpy(&u, &f, sizeof(u));
    if ((u & 0x7fffffffu) > 0x7f800000u)
        return (uint16_t)((u >> 16) | 0x0040u);
    return (uint16_t)((u + 0x7fffu + ((u >> 16) & 1u)) >> 16);
}

static inline float
bfloat16_to_float(uint16_t b)
{
    uint32_t u = (uint32_t)b << 16;
    float f;

    memcpy(&f, &u, sizeof(f));
    return f;
}
```

### Writing the bfloat16 file

The file is created with `NC_NEXTCDF4`, the variable is defined with `NC_BFLOAT16` and no fill value, and the raw `uint16_t` bits are written with `nc_put_vara_bfloat16` (the `NC_FLOAT` file is written with `nc_put_vara_float`):

```c
if ((ret = nc_create(path, NC_NEXTCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
...
if ((ret = nc_def_var(ncid, "tmp2m", xtype, 3, dimids, &varid))) ERR(ret);
if ((ret = nc_def_var_fill(ncid, varid, NC_NOFILL, NULL))) ERR(ret);
...
/* nc_put_vara_bfloat16 stores the raw bits on an NC_BFLOAT16
 * variable; nc_put_vara_float would also work, but HDF5 rounds
 * ties away from zero, while float_to_bfloat16() and the NEP
 * functions round ties to even. */
if ((ret = nc_put_vara_bfloat16(ncid, varid, start, count,
                                (nc_bfloat16_t *)(void *)data))) ERR(ret);
```

### Reopening and checking

The program reopens `bfloat16_tmp2m.nc` with `NC_NEXTCDF4 | NC_NOWRITE`, checks `nc_inq_vartype` returns `NC_BFLOAT16`, reads the bits with `nc_get_vara_bfloat16` into a `uint16_t` buffer, `memcmp`s them against the written bits, and computes the maximum absolute and relative error over the non-fill points. As a cross-check it also reads the same hyperslab with `nc_get_vara_float` into a `float` buffer and verifies that every value's bits equal the stored `uint16_t` pattern shifted left 16 bits — HDF5's bfloat16-to-float conversion is exact:

```c
if ((ret = nc_get_vara_bfloat16(ncid, varid, start, count,
                                (nc_bfloat16_t *)(void *)bdata_in))) ERR(ret);
if ((ret = nc_get_vara_float(ncid, varid, start, count, typed_in))) ERR(ret);
```

### Building and running

```sh
cmake -S . -B build -DNEP_ENABLE_NEXTCDF4=ON -DNEP_BUILD_EXAMPLES=ON \
      -DNEP_ZSTD_PLUGIN_DIR=$HOME/plugins/zstd-only
cmake --build build -j

cd build/examples/nextcdf4
HDF5_PLUGIN_PATH=$HOME/plugins/zstd-only \
NCRCENV_RC=<build>/.ncrc NETCDF_RC=<build> \
./bfloat16 ../../../test/data/netCDF-4/gdas_sfc_sample.nc
```

`-DNEP_ZSTD_PLUGIN_DIR` points ctest at a directory holding exactly one HDF5 zstd plugin, needed to read the compressed sample; `NCRCENV_RC`/`NETCDF_RC` point netcdf-c at the build-tree `.ncrc` that autoloads the NEXTCDF-4 backend. Real output on the reference machine:

```
tmp2m: 1179648 values
float32  file: 4744291 bytes
bfloat16 file: 2384815 bytes (50.3% of float32)
max abs error: 1 K
max rel error: 0.00389105
typed nc_get_vara_float: 0 mismatches
nc_get_var_bfloat16 vs float_to_bfloat16: 0 mismatches
```

`h5dump -H bfloat16_tmp2m.nc` shows `DATATYPE H5T_FLOAT_BFLOAT16LE` for `tmp2m`.

## Benchmark: `bfloat16_perf` and `plot_bfloat16.py`

`bfloat16_perf.c` runs all four GDAS fields (`tmp2m`, `prate_ave`, `soilw1`, `land`) through six configurations — `float`/`bfloat16`/`bfloat16_hdf5` x `contiguous`/`deflate1` (shuffle + deflate level 1, chunks of 1x768x1536) — with 3 repetitions each, keeping the minimum times. The `bfloat16` rows convert in user code and use the untyped calls; the `bfloat16_hdf5` rows define the same `NC_BFLOAT16` variable but write with `nc_put_vara_float` and read with `nc_get_vara_float`, so HDF5 does the conversion inside the timed I/O and `wconv_s`/`rconv_s` are 0. For each `bfloat16_hdf5` row a `#`-prefixed line is printed to stdout (not to the CSV) counting the stored values that differ from round-to-nearest-even, because HDF5 rounds bfloat16 ties away from zero:

```
# tmp2m contiguous: 9 of 1179648 values differ from round-to-nearest-even
# tmp2m deflate1: 9 of 1179648 values differ from round-to-nearest-even
# prate_ave contiguous: 8 of 1179648 values differ from round-to-nearest-even
# prate_ave deflate1: 8 of 1179648 values differ from round-to-nearest-even
# soilw1 contiguous: 3 of 1179648 values differ from round-to-nearest-even
# soilw1 deflate1: 3 of 1179648 values differ from round-to-nearest-even
# land contiguous: 0 of 1179648 values differ from round-to-nearest-even
# land deflate1: 0 of 1179648 values differ from round-to-nearest-even
```

Write time runs from just before conversion (bfloat16 only) through `nc_close`; read time runs from `nc_open` through `nc_close` plus the conversion back to float. The `bfloat16` rows are checked bit-exact and the `bfloat16_hdf5` rows are checked against the stored raw bits; the program fails if any contiguous bfloat16 file is not smaller than its float file, or if any relative error exceeds 2^-8.

The CSV written has columns:

```
field,type,storage,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,wconv_s,rconv_s
```

(`ratio_vs_float` is the matching float row's `file_bytes` divided by this row's; throughput is measured in MB/s of float32 data for every row. `wconv_s`/`rconv_s` are the minimum float32-to-bfloat16 and bfloat16-to-float32 conversion times — parts of `write_s`/`read_s`; 0 for float and `bfloat16_hdf5` rows.)

```sh
HDF5_PLUGIN_PATH=$HOME/plugins/zstd-only \
NCRCENV_RC=<build>/.ncrc NETCDF_RC=<build> \
./bfloat16_perf ../../../test/data/netCDF-4/gdas_sfc_sample.nc bfloat16_results.csv
python3 ../../../examples/nextcdf4/plot_bfloat16.py bfloat16_results.csv bfloat16
```

### Results

Measured on the machine recorded in `examples/nextcdf4/reference_machine.txt`; the full CSV is `examples/nextcdf4/reference_results.csv`. File sizes are in bytes.

| Field | float contiguous | bfloat16 contiguous | bfloat16 (HDF5) contiguous | float deflate1 | bfloat16 deflate1 | bfloat16 (HDF5) deflate1 | max rel err | max rel err (HDF5) |
|-------|-----------------:|--------------------:|---------------------------:|---------------:|------------------:|-------------------------:|------------:|-------------------:|
| `tmp2m` | 4724931 | 2365635 | 2365635 | 2507866 | 203408 | 203550 | 0.00389105 | 0.00389105 |
| `prate_ave` | 4724931 | 2365635 | 2365635 | 3359982 | 1321355 | 1321355 | 0.00389081 | 0.00389081 |
| `soilw1` | 4724931 | 2365635 | 2365635 | 859442 | 307824 | 307825 | 0.00388914 | 0.00388914 |
| `land` | 4724931 | 2365635 | 2365635 | 81474 | 70249 | 70249 | 0 | 0 |

![NEXTCDF-4 file size: float32 vs bfloat16](images/bfloat16_size.png)

![NEXTCDF-4 write/read throughput: float32 vs bfloat16](images/bfloat16_speed.png)

What the numbers show:

- Every contiguous bfloat16 file is 2365635 bytes, 50.1% of its float file (ratio 1.997) — essentially the 2:1 element-size ratio plus fixed metadata. The bfloat16 (HDF5) files are byte-identical in size contiguous, and within a few bytes deflate1.
- With shuffle+deflate level 1, bfloat16 compresses much better than float32, because rounding collapses the number of distinct values the filters see. The four fields contain 858874 (`tmp2m`), 954856 (`prate_ave`), 202809 (`soilw1`) and 3 (`land`) distinct float32 values; after bfloat16 rounding only 68, 6114, 594 and 3 distinct values remain. Compressed sizes drop accordingly: 203408 vs 2507866 bytes for `tmp2m` (12.3x smaller), 1321355 vs 3359982 for `prate_ave` (2.5x), and 307824 vs 859442 for `soilw1` (2.8x). `land` already has only 3 distinct values, so rounding changes nothing and the compressed files are close in size (70249 vs 81474 bytes, 1.16x).
- The maximum relative error is 0.00389 across all fields, just under the 2^-8 = 0.00390625 round-to-nearest bound; `land` is exact because its three values are representable in bfloat16. The HDF5-converted rows show the same maximum errors — ties away from zero stays within the same bound.
- bfloat16 total times are dominated by the in-memory conversion, not I/O. For `tmp2m` contiguous, `write_s` is 0.008186 s of which `wconv_s` is 0.005953 s — the actual write I/O (~0.0022 s) is slightly faster than the float write (0.003112 s), as expected for half the bytes. The same holds for reads: `read_s` 0.006230 s minus `rconv_s` 0.005333 s leaves ~0.0009 s against 0.001442 s for float. With deflate1 the I/O savings grow with the compression gain: `tmp2m` bfloat16 write is 0.020202 s versus 0.077324 s for float, so even including conversion the total is faster.
- HDF5 conversion is much slower than the user-code conversion on this data. For `tmp2m` contiguous, `bfloat16_hdf5` write is 0.117677 s and read 0.085464 s, against 0.008186 s and 0.006230 s for user conversion — HDF5's generic software conversion inside `H5Dwrite`/`H5Dread` costs an order of magnitude more than `float_to_bfloat16()`/`bfloat16_to_float()`. Stored file sizes are nearly identical (2365635 bytes contiguous for both paths; 203550 vs 203408 bytes deflate1 for `tmp2m`, 307825 vs 307824 for `soilw1`), and only a handful of stored values differ from round-to-nearest-even: 9 of 1179648 for `tmp2m`, 8 for `prate_ave`, 3 for `soilw1` and 0 for `land`.
