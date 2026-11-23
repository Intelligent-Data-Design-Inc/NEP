/**
 * @file
 * @brief Private byte-buffer bridges for the NEXTCDF-4 Fortran module.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stddef.h>
#include <stdint.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

_Static_assert(NC_NEXTCDF4 == (64 | (9 << 19)), "UDF flag encoding changed");
_Static_assert(NC_NETCDF4_MODEL == 67108864, "model flag changed");
_Static_assert(sizeof(ptrdiff_t) == sizeof(intptr_t), "Fortran stride kind mismatch");
#define CHECK_TYPE(name, value) _Static_assert(NC_##name == value, "type constant changed")
CHECK_TYPE(FLOAT16, 17);
CHECK_TYPE(BFLOAT16, 18);
CHECK_TYPE(FLOAT8_E4M3, 19);
CHECK_TYPE(FLOAT8_E5M2, 20);
CHECK_TYPE(FLOAT6_E2M3, 21);
CHECK_TYPE(FLOAT6_E3M2, 22);
CHECK_TYPE(FLOAT4_E2M1, 23);
CHECK_TYPE(COMPLEX, 24);
CHECK_TYPE(DOUBLECOMPLEX, 25);
CHECK_TYPE(BITFIELD8, 26);
CHECK_TYPE(BITFIELD16, 27);
CHECK_TYPE(BITFIELD32, 28);
CHECK_TYPE(BITFIELD64, 29);
CHECK_TYPE(STD_REF, 30);

#define BRIDGES(T) \
int nep_f_get_var_##T(int n, int v, void *p) \
{ return nc_get_var_##T(n, v, p); } \
int nep_f_put_var_##T(int n, int v, const void *p) \
{ return nc_put_var_##T(n, v, p); } \
int nep_f_get_var1_##T(int n, int v, const size_t *s, void *p) \
{ return nc_get_var1_##T(n, v, s, p); } \
int nep_f_put_var1_##T(int n, int v, const size_t *s, const void *p) \
{ return nc_put_var1_##T(n, v, s, p); } \
int nep_f_get_vara_##T(int n, int v, const size_t *s, const size_t *c, void *p) \
{ return nc_get_vara_##T(n, v, s, c, p); } \
int nep_f_put_vara_##T(int n, int v, const size_t *s, const size_t *c, const void *p) \
{ return nc_put_vara_##T(n, v, s, c, p); } \
int nep_f_get_vars_##T(int n, int v, const size_t *s, const size_t *c, const ptrdiff_t *t, void *p) \
{ return nc_get_vars_##T(n, v, s, c, t, p); } \
int nep_f_put_vars_##T(int n, int v, const size_t *s, const size_t *c, const ptrdiff_t *t, const void *p) \
{ return nc_put_vars_##T(n, v, s, c, t, p); }
BRIDGES(float16)
BRIDGES(bfloat16)

int nep_f_initialize(void)
{
    return NC_NEXTCDF4_initialize() ? NC_NOERR : NC_EINTERNAL;
}

#define TRANSFER(T) \
    if (method == 0) return write ? nep_f_put_var_##T(n,v,p) : nep_f_get_var_##T(n,v,p); \
    if (method == 1) return write ? nep_f_put_var1_##T(n,v,s,p) : nep_f_get_var1_##T(n,v,s,p); \
    if (method == 2) return write ? nep_f_put_vara_##T(n,v,s,c,p) : nep_f_get_vara_##T(n,v,s,c,p); \
    return write ? nep_f_put_vars_##T(n,v,s,c,t,p) : nep_f_get_vars_##T(n,v,s,c,t,p)

int nep_f_transfer(int n, int v, int type, int write, int method,
                   const size_t *s, const size_t *c, const ptrdiff_t *t, void *p)
{
    if (type == NC_FLOAT16) { TRANSFER(float16); }
    TRANSFER(bfloat16);
}
