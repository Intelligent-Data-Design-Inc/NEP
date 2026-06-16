# OPeNDAP Remote Data Access Examples

These examples demonstrate how to access remote scientific datasets via OPeNDAP using the NetCDF-C and NetCDF-Fortran libraries.

## What is OPeNDAP?

OPeNDAP (Open-source Project for a Network Data Access Protocol) provides network-transparent data access for scientific datasets over HTTP. It allows you to:

- Access remote datasets without downloading entire files
- Subset data on the server (reducing bandwidth)
- Use standard NetCDF API calls with OPeNDAP URLs

For complete documentation, see Chapter 14 of the NetCDF Developer's Handbook.

## Examples

### C Examples

- **opendap_simple.c** — Basic open/read/close of a remote dataset
- **opendap_constraint.c** — Using constraint expressions for server-side subsetting
- **opendap_subset.c** — Client-side subsetting with start/count arrays

### Fortran Examples

- **f_opendap_simple.f90** — Fortran equivalent of opendap_simple.c
- **f_opendap_constraint.f90** — Fortran equivalent of opendap_constraint.c
- **f_opendap_subset.f90** — Fortran equivalent of opendap_subset.c

## Building the Examples

### With CMake

```bash
cd /path/to/nep
mkdir build && cd build
cmake -DENABLE_OPENDAP_EXAMPLES=ON ..
make opendap_examples
```

### With Autotools

```bash
cd /path/to/nep
./configure --enable-opendap-examples
make
cd examples/opendap
```

## Running the Examples

The examples use the public test server at test.opendap.org. An internet connection is required.

```bash
./opendap_simple
./opendap_constraint
./opendap_subset
```

## OPeNDAP URL Format

Basic URL:
```
http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz
```

With constraint expression:
```
http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz?sst[0:9][0:88][0:179]
```

Service endpoints (append to URL):
- `.dds` — Dataset Descriptor Structure (variable shapes)
- `.das` — Data Attribute Structure (metadata)
- `.dmr.xml` — DAP4 Dataset Metadata Response
- `.info` — Combined HTML view
- `.ascii` — ASCII data representation

## Constraint Expressions

Syntax: `?variable[start:stop]` or `?variable[start:stride:stop]`

Examples:
- `?sst[0:9]` — First 10 elements of first dimension
- `?sst[0:2:100]` — Every other element from 0 to 100
- `?sst[0:1][20:30][40:50]` — 3D subset

## Troubleshooting

### URL Not Recognized

Check if your NetCDF library has DAP support:
```bash
nc-config --has-dap
```

If this returns "no", rebuild NetCDF with `--enable-dap`.

### Network Timeouts

Large requests may timeout on slow networks. Use constraint expressions to request smaller subsets.

### Test Server Unavailable

The test.opendap.org server may occasionally be down. Try again later or use alternative public OPeNDAP servers.

## References

- NetCDF Developer's Handbook, Chapter 14: Remote Data Access with OPeNDAP
- OPeNDAP Website: https://www.opendap.org/
- Hyrax Server Documentation: https://opendap.github.io/hyrax/
