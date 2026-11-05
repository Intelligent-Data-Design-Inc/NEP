---
name: netcdf
description: NetCDF-C and NetCDF-Fortran reference for filters, compression, HDF5 plugins, plugin paths, chunking and quantization. Use when writing or reviewing code that defines/inquires filters, builds HDF5 filter plugins, or debugs plugin discovery.
allowed-tools:
  - read
  - grep
  - glob
---

# NetCDF filters and compression

Reference for NetCDF-C (4.10.x, `main` at 4.10.2-development) and
NetCDF-Fortran (4.6.4+), focused on filters and compression. Everything
below was checked against source. When a detail matters, re-verify it in
the source rather than trusting memory.

## Reference sources

Local, up-to-date checkouts (development machine):

- NetCDF-C: `/fast/ed/netcdf-c`
- NetCDF-Fortran: `/fast/ed/netcdf-fortran`
- Installed NetCDF-C: `/usr/local/netcdf-c` (`nc-config --all`)

Key files:

| Topic | NetCDF-C file |
| --- | --- |
| Public filter API, filter ID macros, plugin-path API | `include/netcdf_filter.h` |
| deflate/szip/fletcher32/quantize prototypes, constants, error codes | `include/netcdf.h` |
| `nc_def_var_filter`, bzip2/zstandard/blosc wrappers | `libdispatch/dfilter.c` |
| `nc_def_var_deflate/szip/quantize/fletcher32` dispatch | `libdispatch/dvar.c` |
| `nc_inq_var_deflate/szip` | `libdispatch/dvarinq.c` |
| HDF5 filter validation, ordering, availability | `libhdf5/hdf5filter.c` |
| Shuffle/fletcher32/storage/quantize validation | `libhdf5/hdf5var.c` (`nc_def_var_extra`) |
| Filters applied to the HDF5 DCPL at dataset creation | `libhdf5/nc4hdf.c` (~line 878) |
| Filters read from existing files; missing-filter marking | `libhdf5/hdf5open.c` (~line 1080) |
| Plugin-path management | `libdispatch/dplugins.c`, `libhdf5/hdf5plugins.c` |
| Filter-spec text parsers | `include/netcdf_aux.h`, `libdispatch/daux.c` |
| Headers for plugin authors | `include/netcdf_filter_build.h`, `include/netcdf_filter_hdf5_build.h` |
| Example plugins (bzip2, zstd, blosc, szip, ...) | `plugins/H5Z*.c`, `plugins/CMakeLists.txt` |
| Feature macros (`NC_HAS_ZSTD`, `NC_HAS_BZ2`, ...) | `include/netcdf_meta.h.in` |
| Docs | `docs/filters.md`, `docs/pluginpath.md`, `docs/quickstart_filters.md`, `docs/quantize.md` |
| Tests | `nc_test4/test_filter*.c`, `tst_multifilter.c`, `tst_zstd.c`, `tst_quantize.c`, `tst_filterparser.c`, `tst_parallel_compress.c` |

| Topic | NetCDF-Fortran file |
| --- | --- |
| F90 `nf90_def_var` optional compression args | `fortran/netcdf4_variables.F90` |
| `nf90_def_var_deflate/szip/zstandard/quantize/filter` | `fortran/netcdf4_func.F90` |
| F77 `nf_def_var_filter`, `nf_def_var_zstandard`, ... | `fortran/nf_nc4.F90` |
| C bindings (`BIND(C)`) | `fortran/module_netcdf4_nc_interfaces.F90` |
| `nc_inq_varnparams` helper (C) | `fortran/nf_lib.c` |
| Tests | `nf_test4/ftst_filter.F`, `nf03_test4/f90tst_var_szip.F90`, `f90tst_parallel_compressed.F90` |

## Filter IDs

| Filter | ID | Notes |
| --- | --- | --- |
| deflate (zlib) | 1 | Built into HDF5. `nc_def_var_deflate` |
| shuffle | 2 | Built into HDF5. 0 params |
| fletcher32 | 3 | Checksum, not compression. 0 params |
| szip | 4 | Built into HDF5 if HDF5 has libaec/szip. 2 params in, 4 params reported |
| bzip2 | 307 | Needs plugin. 1 param, level 1-9 |
| blosc | 32001 | Needs plugin. 7 params |
| LZ4 (nep-compress) | 32004 | Needs plugin. 2 params: block size (0 = 1 GiB), encoder selector (signed; ≥2 HC, 0/1 default, <0 acceleration 1-v) |
| zstandard | 32015 | Needs plugin. 1 param, level -131072..22 |
| JPEG-LS (EUMETSAT FCIDECOMP) | 32018 | Needs plugin (fcidecomp / hdf5plugin libh5fcidecomp). 9 user params, 13 stored; near at index 3 / 7 |

IDs are assigned by The HDF Group; IDs above 32767 may be used for testing
without registration.

## C API

All filter functions live in `<netcdf_filter.h>`; include `<netcdf.h>` first.

```c
int nc_def_var_filter(int ncid, int varid, unsigned int id, size_t nparams, const unsigned int *params);
int nc_inq_var_filter_ids(int ncid, int varid, size_t *nfilters, unsigned int *filterids);
int nc_inq_var_filter_info(int ncid, int varid, unsigned int id, size_t *nparams, unsigned int *params);
int nc_inq_var_filter(int ncid, int varid, unsigned int *idp, size_t *nparams, unsigned int *params); /* first filter only */
int nc_inq_filter_avail(int ncid, unsigned int id);   /* NC_NOERR or NC_ENOFILTER */

int nc_def_var_bzip2(int ncid, int varid, int level);
int nc_inq_var_bzip2(int ncid, int varid, int *hasfilterp, int *levelp);
int nc_def_var_zstandard(int ncid, int varid, int level);
int nc_inq_var_zstandard(int ncid, int varid, int *hasfilterp, int *levelp);
int nc_def_var_blosc(int ncid, int varid, unsigned subcompressor, unsigned level, unsigned blocksize, unsigned addshuffle);
int nc_inq_var_blosc(int ncid, int varid, int *hasfilterp, unsigned *subcompressorp, unsigned *levelp, unsigned *blocksizep, unsigned *addshufflep);

/* netcdf.h */
int nc_def_var_deflate(int ncid, int varid, int shuffle, int deflate, int deflate_level);
int nc_inq_var_deflate(int ncid, int varid, int *shufflep, int *deflatep, int *deflate_levelp);
int nc_def_var_szip(int ncid, int varid, int options_mask, int pixels_per_block);
int nc_inq_var_szip(int ncid, int varid, int *options_maskp, int *pixels_per_blockp);
int nc_def_var_fletcher32(int ncid, int varid, int fletcher32);
int nc_def_var_quantize(int ncid, int varid, int quantize_mode, int nsd);
int nc_inq_var_quantize(int ncid, int varid, int *quantize_modep, int *nsdp);
```

Inquiry functions follow the usual two-call pattern: call once with NULL
buffers to get counts, allocate, call again. Any output pointer may be NULL.

### Rules enforced by `NC4_hdf5_def_var_filter` (libhdf5/hdf5filter.c)

- File must be netCDF-4/HDF5 (`NC_ENOTNC4` otherwise) and in define mode
  (`NC_EINDEFINE`). After the dataset is created: `NC_ELATEDEF`.
- Scalar variables (ndims == 0) cannot have filters: `NC_EINVAL`.
- The filter must be available to HDF5 (`H5Zfilter_avail`); otherwise
  `NC_ENOFILTER`. This check triggers HDF5's plugin search, so
  `HDF5_PLUGIN_PATH` must be set before the call.
- Adding a filter forces `NC_CHUNKED` storage and computes default chunk
  sizes if none were set. Setting `NC_CONTIGUOUS`/`NC_COMPACT` on a var that
  already has filters is `NC_EINVAL`.
- Deflate requires exactly 1 param (`NC_EFILTER` otherwise), level 0-9.
  Szip requires exactly 2 params (`NC_EFILTER`), pixels_per_block even and
  <= 32 and <= number of elements. Deflate and szip are mutually exclusive
  (`NC_EINVAL`).
- `nparams > 0` with `params == NULL` is `NC_EINVAL`.
- Defining the same ID again replaces its params but keeps its position.
  Filters cannot be removed or reordered once defined.
- Order: filters apply in definition order on write and reverse on read,
  except fletcher32 is always inserted first and shuffle is inserted
  first (or second, after fletcher32).
- Parallel I/O: defining any filter switches the variable to
  `NC_COLLECTIVE` access. Without HDF5 parallel-filter support
  (`NC_HAS_PAR_FILTERS == 0`) it is `NC_EINVAL`.

### Behaviors that commonly surprise

- `nc_def_var_deflate(ncid, varid, shuffle, 1, 0)` sets shuffle but does
  **not** add deflate: level 0 is treated as "no compression".
- `nc_inq_var_deflate` on a classic-format file returns `NC_NOERR` with all
  outputs 0, not `NC_ENOTNC4`.
- `nc_inq_var_filter` returns `NC_NOERR` with `*idp = 0` when there are no
  filters. `nc_inq_var_filter_info` for an ID not on the variable returns
  `NC_ENOFILTER` (and logs a warning).
- `nc_inq_var_filter` reports only the **first** filter. If shuffle or
  fletcher32 is present it will be the first filter, not your compressor.
  Use `nc_inq_var_filter_ids` + `nc_inq_var_filter_info` to inspect a
  pipeline, and size `params` from the first call. Filters report as many
  params as were stored (blosc 7; any filter defined with extra params),
  so a fixed one-element buffer can overflow.
- Szip: per `docs/filters.md`, HDF5 may expand szip to 4 params with
  modified values; `nc_inq_var_szip` always truncates to 2 and
  `options_mask` may have extra bits set. (With NetCDF-C 4.10.2-dev and
  HDF5 2.1.1, a reopened file reported 2 params.)
- `nc_def_var_zstandard`, `nc_inq_var_zstandard`, `nc_def_var_blosc`,
  `nc_inq_var_blosc` compile to `return NC_NOERR` when NetCDF-C was built
  without zstd/blosc (`HAVE_ZSTD`/`HAVE_BLOSC` unset). They silently do
  nothing. Check `NC_HAS_ZSTD`/`NC_HAS_BLOSC` in `netcdf_meta.h` or verify
  with an inquiry.
- `nc_def_var_bzip2`/`nc_inq_var_bzip2` are always compiled into libnetcdf
  (since 4.9.0) and validate level 1-9 after checking availability.
  nep-compress no longer defines them; its Fortran
  `nf90_def_var_bzip2`/`nf90_inq_var_bzip2` wrappers bind to NetCDF-C's
  symbols. A missing plugin returns `NC_ENOFILTER`.
- Variable-length types (`NC_STRING`, `NC_VLEN`, compounds containing
  them): non-builtin filters are set with `H5Z_FLAG_OPTIONAL`, so HDF5
  skips them. When opening a file where a varsized variable has a
  mandatory filter, that variable is marked unreadable.
- All non-builtin filters (anything except deflate, shuffle, fletcher32,
  szip) are added with `H5Pset_filter(..., H5Z_FLAG_OPTIONAL, ...)`. If a
  plugin's encoder returns 0, HDF5 stores that chunk **unfiltered** without
  reporting an error. Compression failures in a plugin are therefore silent
  on write.
- Opening a file whose filter plugin is missing succeeds; the variable is
  flagged and every `nc_get_var*`/`nc_put_var*` returns `NC_ENOFILTER`.
  Metadata (including filter ID and params) is still readable:
  `ncdump -hs file.nc`.
- Filters (and quantize) must be set after `nc_def_var` and before
  `nc_enddef` or the first data write.
- Filter 32018 (JPEG-LS/fcidecomp) on an unsupported type or chunk
  shape stores data raw with no NetCDF error — only stderr messages
  from the plugin. Validate type (1- or 2-byte integer), rank (2 or 3)
  and chunk shape before defining it.

### Quantization (lossy, applied before filters on write)

- Modes: `NC_NOQUANTIZE` (0), `NC_QUANTIZE_BITGROOM` (1),
  `NC_QUANTIZE_GRANULARBR` (2), `NC_QUANTIZE_BITROUND` (3).
- Only `NC_FLOAT`/`NC_DOUBLE`; other types are `NC_EINVAL`. `nsd` must be
  > 0. BitGroom/GranularBR: nsd is decimal digits, max 6 (float) / 15
  (double). BitRound: nsd is bits, max 23 (float) / 52 (double).
- Not a filter: does not appear in `nc_inq_var_filter_ids`. Recorded as an
  attribute (`_QuantizeBitGroomNumberOfSignificantDigits`,
  `_QuantizeGranularBitRoundNumberOfSignificantDigits`,
  `_QuantizeBitRoundNumberOfSignificantBits`). No decoder needed to read.
- Quantization alone does not shrink data; pair it with a lossless filter.
- Fill values are not quantized. Type conversion happens before quantizing.

### Feature detection

- Compile time: `netcdf_meta.h` defines `NC_HAS_SZIP`, `NC_HAS_ZSTD`,
  `NC_HAS_BZ2`, `NC_HAS_BLOSC`, `NC_HAS_QUANTIZE`, `NC_HAS_PAR_FILTERS`,
  `NC_HAS_NCZARR`.
- Run time: `nc_inq_filter_avail(ncid, id)` or HDF5 `H5Zfilter_avail(id)`.
- Shell: `nc-config --has-stdfilters` (e.g. `deflate szip zstd bz2`),
  `--has-zstd`, `--has-quantize`, `--has-multifilters`, `--plugindir`,
  `--plugin-searchpath`.

## Fortran API

Fortran `varid` is 1-based; wrappers subtract 1 before calling C.

F90 (`use netcdf`):

```fortran
nf90_def_var(ncid, name, xtype, dimids, varid, contiguous, chunksizes, &
             deflate_level, shuffle, fletcher32, endianness, &
             cache_size, cache_nelems, cache_preemption, &
             quantize_mode, nsd, zstandard_level)
nf90_inquire_variable(ncid, varid, ..., deflate_level, shuffle, fletcher32, ..., &
                      quantize_mode, nsd, zstandard, zstandard_level)
nf90_def_var_deflate(ncid, varid, shuffle, deflate, deflate_level)
nf90_def_var_szip(ncid, varid, options_mask, pixels_per_block)
nf90_def_var_zstandard(ncid, varid, zstandard_level)
nf90_def_var_quantize(ncid, varid, quantize_mode, nsd)
nf90_def_var_fletcher32(ncid, varid, fletcher32)
nf90_def_var_filter(ncid, varid, filterid, nparams, params)   ! integer params(*)
nf90_inq_var_filter(ncid, varid, filterid, nparams, params)   ! first filter only
```

F77 equivalents: `nf_def_var_filter`, `nf_inq_var_filter`,
`nf_def_var_zstandard`, `nf_def_var_quantize`, `nf_def_var_szip`, etc.

Fortran gaps and quirks:

- No Fortran wrappers for `nc_inq_var_filter_ids`,
  `nc_inq_var_filter_info`, `nc_inq_filter_avail`, bzip2, blosc, or the
  plugin-path API. Bind to the C functions directly with `bind(C)` (as
  `fsrc/nep_compress.F90` does), remembering `varid - 1` and
  `integer(c_size_t)` for `nparams`.
- Filter IDs and params are default `integer`; they are passed to C as
  `unsigned int` bit patterns.
- `nf90_def_var`: `shuffle` is applied only when `deflate_level > 0` or
  `zstandard_level` is present; `shuffle=.true.` alone is ignored.
  `deflate_level = 0` is ignored. There is no `szip` or generic-filter
  argument.
- `nf90_def_var` sets options in this order: chunking, deflate+shuffle,
  fletcher32, endianness, cache, quantize, zstandard (then shuffle via
  `nf_def_var_deflate(ncid, varid, 1, 0, 0)`).
- `nf90_def_var_quantize`/`nf90_inq_var_quantize` return
  `nf90_enotbuilt` if NetCDF-Fortran was built without `NF_HAS_QUANTIZE`;
  `nf_def_var_zstandard` returns `nc_enotbuilt` without `ENABLE_ZSTD`.
  `quantize_mode` without `nsd` in `nf90_def_var` is `nf90_einval`.
- `nf_inq_var_filter` sizes its C buffer with `nc_inq_varnparams`
  (`fortran/nf_lib.c`), which reports the first filter's param count; the
  caller's `params` array must be large enough.
- Constants: `nf90_noquantize`, `nf90_quantize_bitgroom`,
  `nf90_quantize_granularbr`, `nf90_quantize_bitround`; `NF_` forms in
  `netcdf4.inc`.

## Plugin discovery and paths

- At library init, the global plugin path is `HDF5_PLUGIN_PATH` if set,
  otherwise the build-time `NETCDF_PLUGIN_SEARCH_PATH`
  (`<prefix>/hdf5/lib/plugin:/usr/local/hdf5/lib/plugin` by default on
  Unix). The same path is pushed to HDF5 and NCZarr. Separator is `:` on
  Unix, `;` on Windows.
- Programmatic control (NetCDF-C 4.9.3+), best done before any
  `nc_open`/`nc_create`, and not thread-safe without an external lock:

  ```c
  typedef struct NCPluginList { size_t ndirs; char **dirs; } NCPluginList;
  int nc_plugin_path_ndirs(size_t *ndirsp);
  int nc_plugin_path_get(NCPluginList *dirs);  /* allocates dirs->dirs if NULL */
  int nc_plugin_path_set(NCPluginList *dirs);  /* replaces; ndirs==0 clears */
  ```

- HDF5 scans every file matching `lib*.so*` (Linux), `lib*.dylib*`
  (macOS), `cyg*.dll*` (Cygwin), `*.dll` (Windows) in each directory. A
  plugin must export `H5PLget_plugin_type()` returning `H5PL_TYPE_FILTER`
  and `H5PLget_plugin_info()` returning an `H5Z_class2_t*` whose `id`
  matches. The first matching library wins, so two providers for one ID on
  the path is a configuration error.
- NetCDF-C's own wrappers are named `lib__nch5<name>.so` (e.g.
  `lib__nch5bzip2.so`, `lib__nch5zstd.so`) and NCZarr codec libraries
  `lib__nczhdf5filters.so`, `lib__nczstdfilters.so`. Installed with
  `-DNETCDF_PLUGIN_INSTALL=ON -DNETCDF_WITH_PLUGIN_DIR=<dir>` (CMake) or
  `--enable-plugins --with-plugin-dir=<dir>` (autotools). Default install
  dir is `<prefix>/hdf5/lib/plugin`.
- On this machine `/usr/local/netcdf-c/hdf5/lib/plugin` holds only
  NetCDF-C's own eight plugins (`lib__nch5{bzip2,zstd,shuffle,
  fletcher32,deflate,szip}.so`, `lib__ncz{hdf5filters,stdfilters}.so`),
  from netcdf-c `main` `5baa62e69` built against HDF5 2.1.1 (Debug) on
  2026-11-01. It has no `libh5lz4`, `libh5bz2`, `libh5lzf` or
  `libh5jpeg`. `/usr/local/hdf5/lib/plugin`, second on the default
  path, has no `lib__nch5*` or `libh5lz4`, but still has HDF Group
  `libh5bz2` (so ID 307 has two providers on the default path),
  `libh5jpeg` (linked to HDF5 1.14.6) and old 2025-02-06 `lib__ncz*`
  codec libraries. Do not use any `/usr/local` LZ4 build as a
  reference. The old ones took `cd_values[0]` as the acceleration
  (`e1` semantics). Build upstream hdf5_plugins `master` instead.

## Writing an HDF5 filter plugin

Pattern from `plugins/H5Zbzip2.c` (nep-compress `plugins/H5Zlz4.c` is
upstream hdf5_plugins verbatim; see `plugins/README.md`):

```c
const H5Z_class2_t H5Z_MYFILTER[1] = {{
    H5Z_CLASS_T_VERS, (H5Z_filter_t)MY_ID,
    1, 1,                 /* encoder_present, decoder_present */
    "myfilter",
    can_apply, set_local, /* may be NULL */
    filter_func,
}};
H5PL_type_t H5PLget_plugin_type(void) { return H5PL_TYPE_FILTER; }
const void *H5PLget_plugin_info(void) { return H5Z_MYFILTER; }

static size_t filter_func(unsigned flags, size_t cd_nelmts, const unsigned cd_values[],
                          size_t nbytes, size_t *buf_size, void **buf)
{
    if (flags & H5Z_FLAG_REVERSE) { /* decode */ } else { /* encode */ }
    H5free_memory(*buf); *buf = out; *buf_size = out_alloc_size;
    return out_data_len;          /* 0 means failure */
}
```

- Input length is `nbytes`, not `*buf_size` (NetCDF-C fixed this bug in
  its zstd plugin in 4.10.1).
- Buffers exchanged with HDF5 must use `H5allocate_memory`,
  `H5resize_memory`, `H5free_memory`, not `malloc`/`free`. Leaks can
  trigger `H5MM_final_sanity_check` assertions.
- `cd_values` are 32-bit and byte-swapped per element by HDF5. 64-bit
  params need `ncaux_h5filterspec_fix8()` (netcdf_aux.h) on both sides.
- `netcdf_filter_build.h` / `netcdf_filter_hdf5_build.h` include
  `<hdf5.h>` and `<H5PLextern.h>` only when `USE_HDF5` is defined; external
  plugins should include those headers directly. They define `DLLEXPORT`.
- Build as a shared/module library whose file name matches the platform
  pattern above (e.g. `lib*.so` on Linux), or HDF5 will not load it.
- For NCZarr use, a separate or combined library must export
  `NCZ_get_codec_info()` returning an `NCZ_codec_t` (see
  `docs/filters.md` Appendix E and `include/netcdf_filter_build.h`).

## Command-line tools

- `ncdump -hs file.nc` shows `_Storage`, `_ChunkSizes`, `_Shuffle`,
  `_DeflateLevel`, `_Fletcher32`, `_Filter`, `_Codecs`, `_Quantize*`
  without reading data, so it works even when a plugin is missing.
  `ncdump -F` prints `_Filter`/`_Codecs` instead of the specific attributes.
- `nccopy -d N [-s] in.nc out.nc` sets deflate level and shuffle.
- `nccopy -F "var,307,9" in out` applies a filter; `-F "*,32015,3"` all
  vars; `-F "v1&v2,307,9|4,32,32"` a chain on several vars; `-F none` or
  `-F "var,none"` suppresses filters. Quote specs; `*`, `&`, `|` are shell
  metacharacters. `-F` requires netCDF-4 output.
- `nccopy -c "var:N1,N2"` or `-c "dim/N,..."` sets chunking.
- CDL for `ncgen`: `var:_Filter = "307,9|4,32,32" ;` plus `_Storage` and
  `_ChunkSizes`. Typed constants in specs: `-17b`, `23ub`, `-25S`, `27US`,
  `93U`, `789f`, `1.5d`, `-9L`, `9UL` (64-bit forms take two params, LE).

## nep-compress conventions

- `nc_def_var_lz4` checks `nc_inq_filter_avail` first and returns
  `NC_ENOFILTER` if the plugin is missing, then `NC_EINVAL` for a level
  outside 1-9. It stores two parameters, `{0, 1 - level}`: default
  block size and encoder selector. `nc_inq_var_lz4` uses
  `nc_inq_var_filter_info(LZ4_ID)` count-first, finds LZ4 anywhere in
  the pipeline, and does not need the plugin. It accepts the
  one-parameter legacy form (stored value returned as the level) and
  the two-parameter form (level decoded from the encoder selector: 0/1
  → 1, negative `e` → `1 - e` if in 2-9, anything else `NC_EFILTER`);
  any other count is `NC_EFILTER`.
- `nc_def_var_jpegls` checks `nc_inq_filter_avail` first
  (`NC_ENOFILTER`), then type (`NC_EBADTYPE` unless NC_BYTE/NC_UBYTE/
  NC_SHORT/NC_USHORT), rank (2 or 3), `near` (0-127 / 0-255) and chunk
  shape (`NC_EINVAL`). `nc_inq_var_jpegls` uses
  `nc_inq_var_filter_info(JPEGLS_ID)` count-first, does not need the
  plugin, and accepts only 0, 9 or 13 parameters — anything else is
  `NC_EFILTER`.
- Tests set `HDF5_PLUGIN_PATH` to one plugin directory per filter: the
  build-tree LZ4 directory for LZ4 tests, the external directory for
  BZIP2 tests (CTest does this per test). See `README.md` and
  `docs/compression.md`.
- `perf_compare_filters` is the single exception to the one-directory
  rule: it uses one ordered `HDF5_PLUGIN_PATH` with `build/plugin` first
  (so nep-compress's LZ4 wins) and checks the loaded LZ4 provider. It
  also needs a 32015 (zstd) plugin to read
  `test/data/gdas_sfc_sample.nc`. To write that sample, `nccopy` needs a
  two-step copy (`-d 0` decompress first, then `-F`), and shuffle must
  be given as filter `2` in the `-F` chain, e.g.
  `nccopy -F "tmp2m&prate_ave&soilw1&land,2|32015,19"`.
- When comparing filters (zlib, szip, zstd, bzip2, blosc, LZ4), check
  availability with `nc_inq_filter_avail` first and skip unavailable ones
  rather than relying on wrappers that may silently no-op.
- The minimum NetCDF-C is 4.10.0, and CMake ignores
  `NEPCOMPRESS_ZSTD_PLUGIN_DIR` below 4.10.1.
- In Spack, NetCDF-C's `prefix.plugins` holds exactly one 307 and one
  32015 provider, and the `nep-compress` recipe uses it for both.
- conda-forge HDF5's default plugin directory is
  `$PREFIX/lib/hdf5/plugin`, where libnetcdf installs its plugins.
- nep-compress's conda package installs `libh5lz4_nepcompress.so`
  there.
- `hdf5-external-filter-plugins-lz4` is a second 32004 provider, and
  the recipe forbids it.
