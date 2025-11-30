/**
 * @file @internal This file handles the variable functions for the
 * CDF dispatch layer.
 *
 * @author Edward Hartnett
 * @date 2025-11-23
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <nc4internal.h>
#include "cdfdispatch.h"
#include "nc4dispatch.h"
#include <cdf.h>
#include <stdio.h>  /* For fprintf */

/* Define NC_ECDF error code if not already defined */
#ifndef NC_ECDF
#define NC_ECDF (-1000)
#endif

/* Helper function to get size of netCDF type */
static size_t
get_nc4type_size(nc_type type)
{
    switch (type) {
        case NC_BYTE:
        case NC_CHAR:
        case NC_UBYTE:
            return 1;
        case NC_SHORT:
        case NC_USHORT:
            return 2;
        case NC_INT:
        case NC_UINT:
        case NC_FLOAT:
            return 4;
        case NC_DOUBLE:
        case NC_INT64:
        case NC_UINT64:
            return 8;
        default:
            return 0;
    }
}

/**
 * Read an array of values. This is called by nc_get_vara() for
 * netCDF-4 files, as well as all the other nc_get_vara_*
 * functions. CDF files are handled as a special case.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param startp Array of start indices.
 * @param countp Array of counts.
 * @param ip pointer that gets the data.
 * @param memtype The type of these data after it is read into memory.
 *
 * @return ::NC_NOERR for success.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EINVAL Invalid input.
 * @return ::NC_ECDF CDF library error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
int
NC_CDF_get_vara(int ncid, int varid, const size_t *startp,
                 const size_t *countp, void *ip, int memtype)
{
    NC_VAR_CDF_INFO_T *cdf_var;
    NC_VAR_INFO_T *var;
    NC_GRP_INFO_T *grp;
    NC_FILE_INFO_T *h5;
    CDFid cdfid;
    long *indices = NULL;
    size_t nelem = 1;
    int retval = NC_NOERR;
    size_t d;

    /* Debug output - uncomment if needed */
    /* fprintf(stderr, "%s: ncid 0x%x varid %d memtype %d\n", __func__, ncid, varid, memtype); */

    /* Input validation */
    if (!startp || !countp || !ip)
        return NC_EINVAL;

    /* Get file and group info */
    if ((retval = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return retval;
    if (!var || !var->hdr.name || !var->format_var_info)
        return NC_EBADID;

    /* Get CDF-specific variable info */
    cdf_var = (NC_VAR_CDF_INFO_T *)var->format_var_info;
    cdfid = ((NC_CDF_FILE_INFO_T *)h5->format_file_info)->id;

    /* For record variables, the first dimension is the record number */
    long recNum = 0;
    if (var->ndims > 0) {
        recNum = (long)startp[0];
        /* Suppress unused variable warning */
        (void)recNum;
    }
    size_t dimOffset = (var->ndims > 0) ? 1 : 0;

    /* Allocate space for indices */
    if (var->ndims > dimOffset) {
        if (!(indices = (long *)calloc(var->ndims - dimOffset, sizeof(long))))
            return NC_ENOMEM;
    }

    /* Convert start indices to CDF format and calculate total elements */
    for (d = dimOffset; d < var->ndims; d++) {
        if (indices)
	{
	    indices[d - dimOffset] = (long)startp[d];
	    printf("indices[%ld] %ld\n", d - dimOffset, indices[d - dimOffset]);
	}
        nelem *= countp[d];
    }

    /* If memtype was not specified, use variable's type */
    if (memtype == NC_NAT)
        memtype = var->type_info->hdr.id;

    /* For scalar variables */
    if (var->ndims == 0) {
        CDFstatus status = CDFgetzVarData(cdfid, cdf_var->sdsid, 0, NULL, ip);
        if (status != CDF_OK)
            retval = NC_ECDF;
    }
    /* For array variables */
    else {
        /* For each element in the requested hyperslab */
        for (size_t i = 0; i < nelem; i++) {
            /* Calculate the current position in the output buffer */
            void *out_ptr = (char *)ip + (i * get_nc4type_size(memtype));
            
            /* Read the value from CDF */
            CDFstatus status = CDFgetzVarData(cdfid, cdf_var->sdsid, recNum, 
                                             indices, out_ptr);
            if (status != CDF_OK) {
		printf("Error in cdfvar.c, could not CDFgetzVarData() sdsid %d recNum %ld\n", cdf_var->sdsid, recNum);
                retval = NC_ECDF;
                break;
            }

            /* Update indices for next element (row-major order) */
            if (indices) {
                int dim = var->ndims - 1;
                while ((size_t)dim >= dimOffset) {
                    indices[dim - dimOffset]++;
                    if (indices[dim - dimOffset] < (long)(startp[dim] + countp[dim]))
                        break;
                    indices[dim - dimOffset] = (long)startp[dim];
                    dim--;
                }
                /* If we've processed all records, break */
		if ((size_t)dim < dimOffset && dimOffset > 0)
                    break;
            }
        }
    }

    /* Clean up */
    if (indices) free(indices);

    /* Handle type conversion if needed */
    if (retval == NC_NOERR && var->type_info->hdr.id != memtype) {
        int range_error = 0;
        void *tmp_data = malloc(var->type_info->size * nelem);
        if (!tmp_data) return NC_ENOMEM;
        
        /* Convert from CDF type to requested type */
        if ((retval = nc4_convert_type(ip, tmp_data, memtype, 
                                      var->type_info->hdr.id, nelem,
                                      &range_error, NULL, 0, 
                                      NC_NOQUANTIZE, 0))) {
            free(tmp_data);
            return retval;
        }
        
        /* Copy back to user's buffer */
        memcpy(ip, tmp_data, get_nc4type_size(memtype) * nelem);
        free(tmp_data);
        
        if (range_error)
            return NC_ERANGE;
    }

    return retval;
}
