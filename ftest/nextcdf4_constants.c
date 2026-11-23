/* Author: Edward Hartnett, Intelligent Data Design, Inc. */
#include "nep.h"
int nep_test_constants(const int *p)
{
    const int expected[] = {NC_NEXTCDF4, NC_NETCDF4_MODEL, NC_FLOAT16, NC_BFLOAT16,
        NC_FLOAT8_E4M3, NC_FLOAT8_E5M2, NC_FLOAT6_E2M3, NC_FLOAT6_E3M2, NC_FLOAT4_E2M1,
        NC_COMPLEX, NC_DOUBLECOMPLEX, NC_BITFIELD8, NC_BITFIELD16, NC_BITFIELD32,
        NC_BITFIELD64, NC_STD_REF};
    for (int i = 0; i < 16; i++)
        if (p[i] != expected[i]) return 1;
    return 0;
}
