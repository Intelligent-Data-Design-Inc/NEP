# Makefile.am for performance examples
# Edward Hartnett, Intelligent Data Design, Inc.

AUTOMAKE_OPTIONS = parallel-tests

# Set library path for tests so shared libs (netcdf, hdf5) are found at runtime
AM_TESTS_ENVIRONMENT = LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH"

# Performance examples run as tests only when --enable-benchmarks is set (slow; not for regular CI)
if ENABLE_BENCHMARKS
check_PROGRAMS = cache_tuning chunking deflate fill_values endianness zstandard szip lz4 bzip2 lossless quantize

cache_tuning_SOURCES = cache_tuning.c
cache_tuning_CPPFLAGS = -DENABLE_BENCHMARKS=1
cache_tuning_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

chunking_SOURCES = chunking.c
chunking_CPPFLAGS = -DENABLE_BENCHMARKS=1
chunking_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

deflate_SOURCES = deflate.c
deflate_CPPFLAGS = -DENABLE_BENCHMARKS=1
deflate_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

fill_values_SOURCES = fill_values.c
fill_values_CPPFLAGS = -DENABLE_BENCHMARKS=1
fill_values_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

endianness_SOURCES = endianness.c
endianness_CPPFLAGS = -DENABLE_BENCHMARKS=1
endianness_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

zstandard_SOURCES = zstandard.c
zstandard_CPPFLAGS = -DENABLE_BENCHMARKS=1
zstandard_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

szip_SOURCES = szip.c
szip_CPPFLAGS = -DENABLE_BENCHMARKS=1
szip_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lsz -lm

lz4_SOURCES = lz4.c
lz4_CPPFLAGS = -DENABLE_BENCHMARKS=1
lz4_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

bzip2_SOURCES = bzip2.c
bzip2_CPPFLAGS = -DENABLE_BENCHMARKS=1
bzip2_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

lossless_SOURCES = lossless.c
lossless_CPPFLAGS = -DENABLE_BENCHMARKS=1
lossless_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lsz -lnep -lm

quantize_SOURCES = quantize.c
quantize_CPPFLAGS = -DENABLE_BENCHMARKS=1
quantize_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lsz -lnep -lm

TESTS = cache_tuning chunking deflate fill_values endianness zstandard szip lz4 bzip2 lossless quantize
endif

# Include test scripts in distribution
EXTRA_DIST =

# Ensure built binaries are executable before test scripts run them
check-local:
	chmod +x $(check_PROGRAMS)

# Clean up NetCDF files created by examples
CLEANFILES = *.nc
