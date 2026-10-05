/* NC_UDF(n) for the Fortran tests: the nf90_open() mode flag for UDF
 * slot n, matching NC_UDF(n) in netcdf.h / nep.h. CMake defines
 * NEP_NC_UDF_SLOT_FIELD when netCDF-C uses the slot-number encoding
 * (Unidata/netcdf-c#3442); older releases use one mode bit per slot
 * (valid here for slots 3-9). */
#ifdef NEP_NC_UDF_SLOT_FIELD
#define NC_UDF(n) ior(64, ishft(n, 19))
#else
#define NC_UDF(n) ishft(524288, n - 3)
#endif
