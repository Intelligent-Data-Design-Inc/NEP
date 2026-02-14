/**
 * @file dump_classic_metadata.c
 * @brief Read a NetCDF file and print all metadata (dimensions, variables, attributes)
 *
 * This example demonstrates how to use the NetCDF inquiry functions to discover
 * and print all metadata in a NetCDF file without prior knowledge of its contents.
 * It reads a filename from the command line, opens the file, and prints:
 * - All dimensions (name and length, noting unlimited dimensions)
 * - All global attributes (name, type, and value)
 * - All variables (name, type, dimensions, and attributes)
 *
 * This is a useful pattern for building tools that inspect arbitrary NetCDF files.
 *
 * **Learning Objectives:**
 * - Use nc_inq() to discover file structure
 * - Iterate over dimensions with nc_inq_dim()
 * - Iterate over variables with nc_inq_var()
 * - Iterate over attributes with nc_inq_att() and nc_inq_attname()
 * - Read attribute values of different types
 * - Handle unlimited dimensions
 *
 * **Prerequisites:** Basic C programming, familiarity with NetCDF concepts
 *
 * **Compilation:**
 * @code
 * gcc -o dump_classic_metadata dump_classic_metadata.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./dump_classic_metadata coord_vars.nc
 * @endcode
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); return ERRCODE;}

/* Return a human-readable string for a NetCDF type. */
static const char *
type_name(nc_type xtype)
{
    switch (xtype)
    {
    case NC_BYTE:   return "byte";
    case NC_CHAR:   return "char";
    case NC_SHORT:  return "short";
    case NC_INT:    return "int";
    case NC_FLOAT:  return "float";
    case NC_DOUBLE: return "double";
    case NC_UBYTE:  return "ubyte";
    case NC_USHORT: return "ushort";
    case NC_UINT:   return "uint";
    case NC_INT64:  return "int64";
    case NC_UINT64: return "uint64";
    case NC_STRING: return "string";
    default:        return "unknown";
    }
}

/* Print the value(s) of an attribute. */
static int
print_att_value(int ncid, int varid, const char *att_name,
                nc_type xtype, size_t len)
{
    int retval;
    size_t i;

    switch (xtype)
    {
    case NC_CHAR:
    {
        char *val = malloc(len + 1);
        if (!val) return NC_ENOMEM;
        if ((retval = nc_get_att_text(ncid, varid, att_name, val)))
        {
            free(val);
            return retval;
        }
        val[len] = '\0';
        printf("\"%s\"", val);
        free(val);
        break;
    }
    case NC_BYTE:
    {
        signed char *val = malloc(len * sizeof(signed char));
        if (!val) return NC_ENOMEM;
        if ((retval = nc_get_att_schar(ncid, varid, att_name, val)))
        {
            free(val);
            return retval;
        }
        for (i = 0; i < len; i++)
            printf("%s%d", i ? ", " : "", val[i]);
        free(val);
        break;
    }
    case NC_SHORT:
    {
        short *val = malloc(len * sizeof(short));
        if (!val) return NC_ENOMEM;
        if ((retval = nc_get_att_short(ncid, varid, att_name, val)))
        {
            free(val);
            return retval;
        }
        for (i = 0; i < len; i++)
            printf("%s%d", i ? ", " : "", val[i]);
        free(val);
        break;
    }
    case NC_INT:
    {
        int *val = malloc(len * sizeof(int));
        if (!val) return NC_ENOMEM;
        if ((retval = nc_get_att_int(ncid, varid, att_name, val)))
        {
            free(val);
            return retval;
        }
        for (i = 0; i < len; i++)
            printf("%s%d", i ? ", " : "", val[i]);
        free(val);
        break;
    }
    case NC_FLOAT:
    {
        float *val = malloc(len * sizeof(float));
        if (!val) return NC_ENOMEM;
        if ((retval = nc_get_att_float(ncid, varid, att_name, val)))
        {
            free(val);
            return retval;
        }
        for (i = 0; i < len; i++)
            printf("%s%g", i ? ", " : "", val[i]);
        free(val);
        break;
    }
    case NC_DOUBLE:
    {
        double *val = malloc(len * sizeof(double));
        if (!val) return NC_ENOMEM;
        if ((retval = nc_get_att_double(ncid, varid, att_name, val)))
        {
            free(val);
            return retval;
        }
        for (i = 0; i < len; i++)
            printf("%s%g", i ? ", " : "", val[i]);
        free(val);
        break;
    }
    default:
        printf("(unsupported type)");
        break;
    }

    return 0;
}

/* Print all attributes for a given variable (or NC_GLOBAL). */
static int
print_attributes(int ncid, int varid, int natts, const char *indent)
{
    int retval;
    int a;

    for (a = 0; a < natts; a++)
    {
        char att_name[NC_MAX_NAME + 1];
        nc_type xtype;
        size_t len;

        if ((retval = nc_inq_attname(ncid, varid, a, att_name)))
            return retval;
        if ((retval = nc_inq_att(ncid, varid, att_name, &xtype, &len)))
            return retval;

        printf("%s%s: type %s, length %zu, value: ", indent, att_name,
               type_name(xtype), len);
        if ((retval = print_att_value(ncid, varid, att_name, xtype, len)))
            return retval;
        printf("\n");
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    int ncid, retval;
    int ndims, nvars, ngatts, unlimdimid;
    int d, v;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <netcdf_file>\n", argv[0]);
        return 1;
    }

    /* Open the file. */
    if ((retval = nc_open(argv[1], NC_NOWRITE, &ncid)))
        ERR(retval);

    /* Get top-level counts. */
    if ((retval = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid)))
        ERR(retval);

    printf("File: %s\n", argv[1]);
    printf("Number of dimensions: %d\n", ndims);
    printf("Number of variables: %d\n", nvars);
    printf("Number of global attributes: %d\n", ngatts);
    if (unlimdimid >= 0)
        printf("Unlimited dimension id: %d\n", unlimdimid);
    else
        printf("No unlimited dimension\n");

    /* Print dimensions. */
    printf("\nDimensions:\n");
    for (d = 0; d < ndims; d++)
    {
        char dim_name[NC_MAX_NAME + 1];
        size_t dim_len;

        if ((retval = nc_inq_dim(ncid, d, dim_name, &dim_len)))
            ERR(retval);
        printf("  %s = %zu", dim_name, dim_len);
        if (d == unlimdimid)
            printf(" (unlimited)");
        printf("\n");
    }

    /* Print global attributes. */
    if (ngatts > 0)
    {
        printf("\nGlobal Attributes:\n");
        if ((retval = print_attributes(ncid, NC_GLOBAL, ngatts, "  ")))
            ERR(retval);
    }

    /* Print variables. */
    printf("\nVariables:\n");
    for (v = 0; v < nvars; v++)
    {
        char var_name[NC_MAX_NAME + 1];
        nc_type xtype;
        int var_ndims, var_natts;
        int var_dimids[NC_MAX_VAR_DIMS];

        if ((retval = nc_inq_var(ncid, v, var_name, &xtype, &var_ndims,
                                 var_dimids, &var_natts)))
            ERR(retval);

        printf("  %s: type %s, %d dimension(s)", var_name,
               type_name(xtype), var_ndims);

        /* Print dimension names for this variable. */
        if (var_ndims > 0)
        {
            printf(" (");
            for (d = 0; d < var_ndims; d++)
            {
                char dim_name[NC_MAX_NAME + 1];
                if ((retval = nc_inq_dimname(ncid, var_dimids[d], dim_name)))
                    ERR(retval);
                printf("%s%s", d ? ", " : "", dim_name);
            }
            printf(")");
        }
        printf(", %d attribute(s)\n", var_natts);

        /* Print variable attributes. */
        if (var_natts > 0)
        {
            if ((retval = print_attributes(ncid, v, var_natts, "    ")))
                ERR(retval);
        }
    }

    /* Close the file. */
    if ((retval = nc_close(ncid)))
        ERR(retval);

    return 0;
}
