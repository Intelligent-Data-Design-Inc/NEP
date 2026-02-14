/**
 * @file dump_nc4_metadata.c
 * @brief Read a NetCDF-4 file and print all metadata including user-defined types
 *
 * This example demonstrates how to use the NetCDF-4 inquiry functions to discover
 * and print all metadata in a NetCDF-4 file without prior knowledge of its contents.
 * It reads a filename from the command line, opens the file, and prints:
 * - User-defined types (compound, enum, vlen, opaque)
 * - All dimensions (name and length, noting unlimited dimensions)
 * - All global attributes (name, type, and value)
 * - All variables (name, type, dimensions, and attributes)
 * - Groups (recursively)
 *
 * This extends the classic dump_classic_metadata example with NetCDF-4 features.
 *
 * **Learning Objectives:**
 * - Use nc_inq_typeids() to discover user-defined types
 * - Use nc_inq_user_type() to determine type class
 * - Use nc_inq_compound(), nc_inq_enum(), nc_inq_vlen(), nc_inq_opaque()
 * - Use nc_inq_grps() to discover groups
 * - Recursively traverse the group hierarchy
 *
 * **Prerequisites:** Basic C programming, familiarity with NetCDF-4 concepts
 *
 * **Compilation:**
 * @code
 * gcc -o dump_nc4_metadata dump_nc4_metadata.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./dump_nc4_metadata user_types.nc
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

/* Return a human-readable string for a NetCDF type. For user-defined
 * types, look up the name. */
static const char *
type_name_str(int ncid, nc_type xtype, char *buf, size_t buflen)
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
    default:
        /* User-defined type - look up the name. */
        if (nc_inq_type(ncid, xtype, buf, NULL) == NC_NOERR)
            return buf;
        return "unknown";
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
    case NC_STRING:
    {
        char **val = malloc(len * sizeof(char *));
        if (!val) return NC_ENOMEM;
        if ((retval = nc_get_att_string(ncid, varid, att_name, val)))
        {
            free(val);
            return retval;
        }
        for (i = 0; i < len; i++)
            printf("%s\"%s\"", i ? ", " : "", val[i] ? val[i] : "(null)");
        nc_free_string(len, val);
        free(val);
        break;
    }
    default:
        printf("(user-defined type)");
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
    char type_buf[NC_MAX_NAME + 1];

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
               type_name_str(ncid, xtype, type_buf, sizeof(type_buf)), len);
        if ((retval = print_att_value(ncid, varid, att_name, xtype, len)))
            return retval;
        printf("\n");
    }

    return 0;
}

/* Print user-defined types in a group. */
static int
print_user_types(int ncid, const char *indent)
{
    int retval;
    int ntypes;
    int *typeids = NULL;
    int t;

    if ((retval = nc_inq_typeids(ncid, &ntypes, NULL)))
        return retval;

    if (ntypes == 0)
        return 0;

    typeids = malloc(ntypes * sizeof(int));
    if (!typeids) return NC_ENOMEM;

    if ((retval = nc_inq_typeids(ncid, NULL, typeids)))
    {
        free(typeids);
        return retval;
    }

    printf("\n%sUser-Defined Types:\n", indent);

    for (t = 0; t < ntypes; t++)
    {
        char name[NC_MAX_NAME + 1];
        size_t size;
        nc_type base_type;
        size_t nfields;
        int type_class;
        char type_buf[NC_MAX_NAME + 1];

        if ((retval = nc_inq_user_type(ncid, typeids[t], name, &size,
                                        &base_type, &nfields, &type_class)))
        {
            free(typeids);
            return retval;
        }

        switch (type_class)
        {
        case NC_COMPOUND:
        {
            printf("%s  %s: compound, %zu bytes, %zu field(s)\n",
                   indent, name, size, nfields);
            size_t f;
            for (f = 0; f < nfields; f++)
            {
                char field_name[NC_MAX_NAME + 1];
                nc_type field_type;
                size_t field_offset;
                int field_ndims;

                if ((retval = nc_inq_compound_field(ncid, typeids[t], (int)f,
                                                     field_name, &field_offset,
                                                     &field_type, &field_ndims,
                                                     NULL)))
                {
                    free(typeids);
                    return retval;
                }
                printf("%s    field %zu: %s, type %s, offset %zu\n",
                       indent, f, field_name,
                       type_name_str(ncid, field_type, type_buf,
                                     sizeof(type_buf)),
                       field_offset);
            }
            break;
        }
        case NC_VLEN:
        {
            nc_type vlen_base;
            if ((retval = nc_inq_vlen(ncid, typeids[t], NULL, NULL,
                                       &vlen_base)))
            {
                free(typeids);
                return retval;
            }
            printf("%s  %s: vlen of %s\n", indent, name,
                   type_name_str(ncid, vlen_base, type_buf,
                                 sizeof(type_buf)));
            break;
        }
        case NC_ENUM:
        {
            nc_type enum_base;
            size_t num_members;
            if ((retval = nc_inq_enum(ncid, typeids[t], NULL, &enum_base,
                                       NULL, &num_members)))
            {
                free(typeids);
                return retval;
            }
            printf("%s  %s: enum of %s, %zu member(s)\n", indent, name,
                   type_name_str(ncid, enum_base, type_buf,
                                 sizeof(type_buf)),
                   num_members);
            size_t m;
            for (m = 0; m < num_members; m++)
            {
                char member_name[NC_MAX_NAME + 1];
                long long member_val;
                if ((retval = nc_inq_enum_member(ncid, typeids[t], (int)m,
                                                  member_name, &member_val)))
                {
                    free(typeids);
                    return retval;
                }
                printf("%s    %s = %lld\n", indent, member_name, member_val);
            }
            break;
        }
        case NC_OPAQUE:
            printf("%s  %s: opaque, %zu bytes\n", indent, name, size);
            break;
        default:
            printf("%s  %s: unknown type class %d\n", indent, name,
                   type_class);
            break;
        }
    }

    free(typeids);
    return 0;
}

/* Print metadata for a single group (and recurse into subgroups). */
static int
print_group(int ncid, const char *group_name, const char *indent)
{
    int retval;
    int ndims, nvars, ngatts, unlimdimid;
    int d, v;
    char type_buf[NC_MAX_NAME + 1];

    /* Get top-level counts. */
    if ((retval = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid)))
        return retval;

    printf("%sGroup: %s\n", indent, group_name);
    printf("%sNumber of dimensions: %d\n", indent, ndims);
    printf("%sNumber of variables: %d\n", indent, nvars);
    printf("%sNumber of global attributes: %d\n", indent, ngatts);
    if (unlimdimid >= 0)
        printf("%sUnlimited dimension id: %d\n", indent, unlimdimid);
    else
        printf("%sNo unlimited dimension\n", indent);

    /* Print user-defined types. */
    if ((retval = print_user_types(ncid, indent)))
        return retval;

    /* Print dimensions. */
    printf("\n%sDimensions:\n", indent);
    for (d = 0; d < ndims; d++)
    {
        char dim_name[NC_MAX_NAME + 1];
        size_t dim_len;

        if ((retval = nc_inq_dim(ncid, d, dim_name, &dim_len)))
            return retval;
        printf("%s  %s = %zu", indent, dim_name, dim_len);
        if (d == unlimdimid)
            printf(" (unlimited)");
        printf("\n");
    }

    /* Print global attributes. */
    if (ngatts > 0)
    {
        char att_indent[256];
        printf("\n%sGlobal Attributes:\n", indent);
        snprintf(att_indent, sizeof(att_indent), "%s  ", indent);
        if ((retval = print_attributes(ncid, NC_GLOBAL, ngatts, att_indent)))
            return retval;
    }

    /* Print variables. */
    printf("\n%sVariables:\n", indent);
    for (v = 0; v < nvars; v++)
    {
        char var_name[NC_MAX_NAME + 1];
        nc_type xtype;
        int var_ndims, var_natts;
        int var_dimids[NC_MAX_VAR_DIMS];
        char att_indent[256];

        if ((retval = nc_inq_var(ncid, v, var_name, &xtype, &var_ndims,
                                 var_dimids, &var_natts)))
            return retval;

        printf("%s  %s: type %s, %d dimension(s)", indent, var_name,
               type_name_str(ncid, xtype, type_buf, sizeof(type_buf)),
               var_ndims);

        /* Print dimension names for this variable. */
        if (var_ndims > 0)
        {
            printf(" (");
            for (d = 0; d < var_ndims; d++)
            {
                char dim_name[NC_MAX_NAME + 1];
                if ((retval = nc_inq_dimname(ncid, var_dimids[d], dim_name)))
                    return retval;
                printf("%s%s", d ? ", " : "", dim_name);
            }
            printf(")");
        }
        printf(", %d attribute(s)\n", var_natts);

        /* Print variable attributes. */
        if (var_natts > 0)
        {
            snprintf(att_indent, sizeof(att_indent), "%s    ", indent);
            if ((retval = print_attributes(ncid, v, var_natts, att_indent)))
                return retval;
        }
    }

    /* Recurse into subgroups. */
    int ngroups;
    if ((retval = nc_inq_grps(ncid, &ngroups, NULL)))
        return retval;

    if (ngroups > 0)
    {
        int *grpids = malloc(ngroups * sizeof(int));
        if (!grpids) return NC_ENOMEM;

        if ((retval = nc_inq_grps(ncid, NULL, grpids)))
        {
            free(grpids);
            return retval;
        }

        char sub_indent[256];
        snprintf(sub_indent, sizeof(sub_indent), "%s  ", indent);

        int g;
        for (g = 0; g < ngroups; g++)
        {
            char grp_name[NC_MAX_NAME + 1];
            if ((retval = nc_inq_grpname(grpids[g], grp_name)))
            {
                free(grpids);
                return retval;
            }
            printf("\n");
            if ((retval = print_group(grpids[g], grp_name, sub_indent)))
            {
                free(grpids);
                return retval;
            }
        }
        free(grpids);
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    int ncid, retval;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <netcdf_file>\n", argv[0]);
        return 1;
    }

    /* Open the file. */
    if ((retval = nc_open(argv[1], NC_NOWRITE, &ncid)))
        ERR(retval);

    printf("File: %s\n", argv[1]);

    /* Print the root group. */
    if ((retval = print_group(ncid, "/", "")))
        ERR(retval);

    /* Close the file. */
    if ((retval = nc_close(ncid)))
        ERR(retval);

    return 0;
}
