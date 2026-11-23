# NEXTCDF-4 Fortran Binding {#nextcdf4_fortran}

Author: Edward Hartnett, Intelligent Data Design, Inc.

Starting with v4.3.0, `use nep_nextcdf4` supplies explicit backend initialization,
mode/type constants and binary16/bfloat16 bit-buffer I/O. NEXTCDF-4 remains
experimental. This module is separate from NetCDF-Fortran's `netcdf` module and
from the retired NEP compression module `nep`; compression users still use
`nep_compress` from nep-compress.

## Build and link

Enable both `NEP_ENABLE_NEXTCDF4=ON` and `NEP_ENABLE_FORTRAN=ON`. This combination
requires NetCDF-Fortran >=4.6.2, a compatible `netcdf.mod` and Fortran compiler,
in addition to NEXTCDF-4's HDF5 >=2.1.1 and 64-slot netcdf-c installation.
The selected compiler's default integer kind must match `c_int`. Missing or
incompatible NetCDF-Fortran is a configure error for this combination, not a
silently skipped binding. Reader-only builds retain their existing optional
NetCDF-Fortran discovery behavior.

For custom installations, include **both** C and Fortran pkg-config directories
in `PKG_CONFIG_PATH`, and select their common HDF5/netcdf-c stack:

```sh
export PKG_CONFIG_PATH=/usr/local/netcdf-c/lib/pkgconfig:/usr/local/netcdf-fortran/lib/pkgconfig
cmake -S . -B build -DNEP_ENABLE_NEXTCDF4=ON -DNEP_ENABLE_FORTRAN=ON
cmake --build build --parallel 8
ctest --test-dir build -L nextcdf4_fortran --output-on-failure
cmake --install build --prefix /path/to/nep
```

The installed artifacts are `${libdir}/libnepnextcdf4f.so` and
`${includedir}/nep/nep_nextcdf4.mod`. Fortran `.mod` files are compiler-specific;
use a compiler compatible with the one that built both modules. Neither new
artifact is installed when either option is OFF.

A downstream CMake program needs only the installed NEP prefix, its dependency
pkg-config paths, and the exported target:

```cmake
project(half_example LANGUAGES Fortran)
find_package(NEP CONFIG REQUIRED)
add_executable(half_example half_example.F90)
target_link_libraries(half_example PRIVATE NEP::nepnextcdf4f)
```

For manual linking, using `NEP_PREFIX` for the installation prefix and `lib` for
its configured library directory:

```sh
gfortran -I"$NEP_PREFIX/include/nep" $(nf-config --fflags) half_example.F90 \
  -L"$NEP_PREFIX/lib" -Wl,-rpath,"$NEP_PREFIX/lib" -lnepnextcdf4f $(nf-config --flibs) -o half_example
```

Keep `libncnextcdf4`/`libnep` and their dependencies on the runtime loader path.
No external HDF5 filter plugin is needed for this example or the binding tests.

## Public procedures

All procedures are integer functions returning NetCDF status codes.
`nf90_nextcdf4_initialize()` registers NEXTCDF-4 and returns `nf90_noerr` on
success or `nf90_einternal` on registration failure. Call it before standard
`nf90_create`/`nf90_open` with the `nf90_nextcdf4` flag. There is no finalize
wrapper: the process-wide dispatch registration must remain valid for open files.

The four I/O generics are:

```fortran
status = nf90_put_var_float16(ncid, varid, values, start, count, stride)
status = nf90_get_var_float16(ncid, varid, values, start, count, stride)
status = nf90_put_var_bfloat16(ncid, varid, values, start, count, stride)
status = nf90_get_var_bfloat16(ncid, varid, values, start, count, stride)
```

- `ncid`, `varid`: default integers, `intent(in)`. File/group IDs are unchanged
  from NetCDF-Fortran; variable IDs are **one-based**.
- `values`: `integer(c_int16_t)`, scalar or rank 1–7. Put is `intent(in)`; get is
  `intent(inout)` so untouched elements remain defined. The buffer holds exact
  **bit encodings**, not numerically converted signed integers and not a native
  Fortran half-precision real kind. Use `int(z'BC00',c_int16_t)` for a pattern
  with the high bit set, not a numeric conversion from an out-of-range integer.
- `start(:)`, `count(:)`, `stride(:)`: optional default-integer vectors, in
  **Fortran dimension order**. Starts are one-based and strides positive; omitted
  starts/strides are one. Counts are nonnegative, default to the memory shape,
  and cannot exceed it. Internally selection vectors are reversed for C.

Array rank must match the file variable rank; scalar specifics require scalar
variables. With no selection arguments, the array shape must equal the entire
variable's current dimension lengths. Use explicit selection to append unlimited
dimensions. A partial selection uses the leading `count` rectangle of memory;
noncontiguous actual arguments are supported through owned packed temporaries.
Strides apply to the file, not to the memory rectangle. Unselected get elements
are preserved. Zero counts succeed after ID/type/selection validation without
accessing a buffer. Vectors on scalar calls must be empty; omitted vectors read
or write the whole scalar, and explicit empty vectors allow the scalar `var1`
path. Start-only array calls with unit memory extents use `var1`; other contiguous
selections use `vara`, strided selections use `vars`, and whole-variable calls
use `var`. These implementation choices do not change Fortran indexing.

The matching half variable type uses raw-bit I/O. `NC_FLOAT`/`NC_DOUBLE` variables
use the corresponding C small-float wrapper's numeric conversion; other variable
types return `nf90_ebadtype` (`nf90_echar` for character data). Rounding is NEP
round-to-nearest-even, with no new Fortran conversion engine. To numerically
convert a Fortran real32/real64 array to a half variable, use standard
`nf90_put_var`; standard `nf90_get_var` reads a half variable into real32/real64.

Malformed wrapper shapes/vectors and size-product overflow return `nf90_einval`;
allocation failure returns `nf90_enomem`. ID/type/bounds failures retain NetCDF
errors. A range-warning read (`nf90_erange`) still copies completed results back;
a structural/I/O failure does not scatter a temporary get buffer. Writes retain
C's completed-write range semantics and are not rolled back on a range warning.

## Constants and representation limits

| Fortran constant | Value | Meaning |
|---|---:|---|
| `nf90_nextcdf4` | `ior(64,ishft(9,19))` | UDF slot 9 in the required 64-slot scheme |
| `nf90_netcdf4_model` | 67108864 | NetCDF-4 enhanced-model compatibility flag |
| `nf90_float16` / `nf90_bfloat16` | 17 / 18 | IEEE binary16 / bfloat16 |
| `nf90_float8_e4m3` / `nf90_float8_e5m2` | 19 / 20 | FP8 |
| `nf90_float6_e2m3` / `nf90_float6_e3m2` | 21 / 22 | FP6 |
| `nf90_float4_e2m1` | 23 | FP4 |
| `nf90_complex` / `nf90_doublecomplex` | 24 / 25 | Complex atomic types |
| `nf90_bitfield8` / `nf90_bitfield16` / `nf90_bitfield32` / `nf90_bitfield64` | 26 / 27 / 28 / 29 | Bitfield atomic types |
| `nf90_std_ref` | 30 | Unified HDF5 reference type; constant only |

Use `ior(nf90_nextcdf4,nf90_netcdf4_model)` or
`ior(nf90_nextcdf4,nf90_classic_model)` for compatibility mode. These modes reject
NEXTCDF-4-specific types, as in C. The two model flags are mutually exclusive.
Build-time C assertions and a mixed-language test check constant parity.

Constants do **not** imply a supported Fortran value interface for FP8/FP6/FP4,
complex, bitfield or reference types. No reference token representation, reference
I/O, `nc_ref_*`/`nc_deref_*` wrappers or reference cleanup is supplied in v4.3.0.
Full compound/vlen/NC_STRING value interfaces and recursive ownership are also
beyond this binding's scope; the C implementations are not thereby broken or
disabled. Existing NetCDF-Fortran enum/compound metadata and ordinary numeric and
`NC_CHAR` character I/O are covered. `NC_CHAR` is not `NC_STRING`.
There are no F77, `map`/`varm`, or half attribute wrappers.

## Complete example

This example writes raw binary16 encodings of +1 and -1, closes the file, and
checks them after reopening. The bfloat16 encodings would instead be `3F80` and
`BF80`, with the bfloat16 type and generic names.

```fortran
program half_example
  use iso_c_binding
  use netcdf
  use nep_nextcdf4
  implicit none
  integer :: ncid, dimid, varid
  integer(c_int16_t) :: bits(2), result(2)
  bits = [int(z'3C00',c_int16_t), int(z'BC00',c_int16_t)]
  call check(nf90_nextcdf4_initialize())
  call check(nf90_create('half.nc',nf90_nextcdf4,ncid))
  call check(nf90_def_dim(ncid,'x',2,dimid))
  call check(nf90_def_var(ncid,'half',nf90_float16,[dimid],varid))
  call check(nf90_enddef(ncid))
  call check(nf90_put_var_float16(ncid,varid,bits))
  call check(nf90_close(ncid))
  call check(nf90_open('half.nc',nf90_nextcdf4,ncid))
  call check(nf90_get_var_float16(ncid,varid,result))
  if (any(bits /= result)) stop 2
  call check(nf90_close(ncid))
contains
  subroutine check(status)
    integer, intent(in) :: status
    if (status /= nf90_noerr) then
      print *, trim(nf90_strerror(status))
      stop 1
    endif
  end subroutine
end program
```

The `nextcdf4_fortran` CTest label runs focused type/mode/I/O tests and a separate
installed consumer that uses only the exported target. Normal NEXTCDF-4 CI builds
NetCDF-Fortran against the same pinned C/HDF5 stack and runs these tests; the
existing C-only sanitizer full suite is unchanged. Spack uses its existing
`+fortran+nextcdf4` variants; conda currently disables NEXTCDF-4.
