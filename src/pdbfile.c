/**
 * @file pdbfile.c
 * @brief Legacy PDB User-Defined Format (UDF) dispatch layer.
 *
 * V3.3.0 Sprint 2: read-only fixed-column parser for legacy PDB (Protein
 * Data Bank) files. Exposes atom coordinates, per-atom identity fields,
 * HEADER/TITLE/COMPND/SOURCE attributes, and CRYST1 unit-cell attributes
 * through the netCDF-4 data model using the UDF7 dispatch table.
 *
 * Known limitations carried forward to later sprints:
 * - SEQRES-derived sequence data is not yet exposed.
 * - Hybrid-36 encoded serial/residue numbers are not supported.
 * - Multi-model files are parsed but not yet covered by tests.
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nep_nc4.h"
#include "pdbdispatch.h"

extern int nc4_var_list_add(NC_GRP_INFO_T *grp, const char *name, int ndims,
                            NC_VAR_INFO_T **var);

/** Maximum length of a legacy PDB text line. */
#define PDB_MAX_LINE 1024

/** Width of the atom_site_label_atom_id string field. */
#define PDB_NAME_LEN 4

/** Width of the atom_site_label_comp_id string field. */
#define PDB_RESNAME_LEN 3

/** Width of the atom_site_auth_asym_id string field. */
#define PDB_CHAINID_LEN 1

/** Width of the atom_site_type_symbol string field. */
#define PDB_ELEMENT_LEN 2

/** Width of the atom_site_group_PDB string field. */
#define PDB_GROUP_LEN 6

/**
 * @internal Right-trim whitespace in place.
 *
 * @param s String to trim.
 */
static void
pdb_rtrim(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
}

/**
 * @internal Trim leading and trailing whitespace in place.
 *
 * @param s String to trim.
 */
static void
pdb_trim(char *s)
{
    size_t len = strlen(s);
    size_t start = 0;

    while (start < len && isspace((unsigned char)s[start]))
        start++;

    while (len > start && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';

    if (start > 0)
        memmove(s, s + start, len - start + 1);
}

/**
 * @internal Copy a fixed-column field from a PDB line.
 *
 * Columns are 1-indexed and inclusive, matching the wwPDB format
 * specification. The destination is null-terminated and right-trimmed.
 *
 * @param line Input PDB line.
 * @param linelen strlen(line).
 * @param start First column (1-indexed).
 * @param end Last column (1-indexed).
 * @param dest Output buffer.
 * @param destlen Size of output buffer.
 */
static void
pdb_copy_field(const char *line, size_t linelen, int start, int end,
               char *dest, size_t destlen)
{
    int s = start - 1;
    int e = end - 1;
    int len;

    dest[0] = '\0';

    if (s < 0 || (size_t)s >= linelen)
        return;
    if (e < s)
        return;
    if ((size_t)e >= (int)linelen)
        e = (int)linelen - 1;

    len = e - s + 1;
    if (len <= 0)
        return;
    if ((size_t)len >= destlen)
        len = (int)destlen - 1;

    memcpy(dest, line + s, len);
    dest[len] = '\0';
    pdb_rtrim(dest);
}

/**
 * @internal Parse a fixed-column numeric field as float.
 *
 * @param line Input PDB line.
 * @param linelen strlen(line).
 * @param start First column.
 * @param end Last column.
 * @param value Pointer to float receiving the result.
 *
 * @return 0 on success, -1 if the field is empty or malformed.
 */
static int
pdb_parse_float(const char *line, size_t linelen, int start, int end,
              float *value)
{
    char buf[32];
    char *endptr;

    pdb_copy_field(line, linelen, start, end, buf, sizeof(buf));
    if (buf[0] == '\0')
        return -1;

    *value = (float)strtod(buf, &endptr);
    if (endptr == buf || *endptr != '\0')
        return -1;

    return 0;
}

/**
 * @internal Parse a fixed-column numeric field as int.
 *
 * @param line Input PDB line.
 * @param linelen strlen(line).
 * @param start First column.
 * @param end Last column.
 * @param value Pointer to int receiving the result.
 *
 * @return 0 on success, -1 if the field is empty or malformed.
 */
static int
pdb_parse_int(const char *line, size_t linelen, int start, int end,
              int *value)
{
    char buf[32];
    char *endptr;

    pdb_copy_field(line, linelen, start, end, buf, sizeof(buf));
    if (buf[0] == '\0')
        return -1;

    *value = (int)strtol(buf, &endptr, 10);
    if (endptr == buf || *endptr != '\0')
        return -1;

    return 0;
}

/**
 * @internal Append a trimmed text fragment to a growing string.
 *
 * Used to concatenate continuation lines for TITLE/COMPND/SOURCE.
 *
 * @param dest Pointer to the current string (may be reallocated).
 * @param fragment Text fragment to append.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
pdb_append_text(char **dest, const char *fragment)
{
    size_t old_len = *dest ? strlen(*dest) : 0;
    size_t frag_len = strlen(fragment);
    size_t add_len = frag_len + (old_len > 0 ? 1 : 0);
    char *new_data;

    if (frag_len == 0)
        return NC_NOERR;

    new_data = (char *)realloc(*dest, old_len + add_len + 1);
    if (!new_data)
        return NC_ENOMEM;

    if (old_len > 0)
    {
        new_data[old_len] = ' ';
        memcpy(new_data + old_len + 1, fragment, frag_len);
    }
    else
    {
        memcpy(new_data, fragment, frag_len);
    }

    new_data[old_len + add_len] = '\0';
    *dest = new_data;

    return NC_NOERR;
}

/**
 * @internal Free all memory owned by a NC_PDB_FILE_INFO_T struct.
 *
 * @param pdb_file PDB file info to free.
 */
static void
pdb_free_file_info(NC_PDB_FILE_INFO_T *pdb_file)
{
    if (!pdb_file)
        return;

    free(pdb_file->path);
    free(pdb_file->x);
    free(pdb_file->y);
    free(pdb_file->z);
    free(pdb_file->serial);
    free(pdb_file->name);
    free(pdb_file->res_name);
    free(pdb_file->chain_id);
    free(pdb_file->res_seq);
    free(pdb_file->occupancy);
    free(pdb_file->temp_factor);
    free(pdb_file->element);
    free(pdb_file->group);
    free(pdb_file->id_code);
    free(pdb_file->classification);
    free(pdb_file->dep_date);
    free(pdb_file->title);
    free(pdb_file->compnd);
    free(pdb_file->source);
    free(pdb_file->cell_a);
    free(pdb_file->cell_b);
    free(pdb_file->cell_c);
    free(pdb_file->cell_alpha);
    free(pdb_file->cell_beta);
    free(pdb_file->cell_gamma);
    free(pdb_file->space_group);
    free(pdb_file->symmetry_z);

    free(pdb_file);
}

/**
 * @internal Create a netCDF-4 type description for a variable.
 *
 * Mirrors the helper used by other NEP UDF handlers.
 *
 * @param xtype NetCDF type.
 * @param endianness Byte order.
 * @param type_size Size in bytes.
 * @param type_name Human-readable type name.
 * @param typep Pointer receiving the new type struct.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
pdb_set_var_type(nc_type xtype, int endianness, size_t type_size,
                 char *type_name, NC_TYPE_INFO_T **typep)
{
    NC_TYPE_INFO_T *type;

    assert(typep);

    if (!(type = calloc(1, sizeof(NC_TYPE_INFO_T))))
        return NC_ENOMEM;
    if (!(type->hdr.name = strdup(type_name)))
    {
        free(type);
        return NC_ENOMEM;
    }
    type->hdr.sort = NCTYP;

    if (xtype == NC_FLOAT)
        type->nc_type_class = NC_FLOAT;
    else if (xtype == NC_DOUBLE)
        type->nc_type_class = NC_DOUBLE;
    else if (xtype == NC_CHAR)
        type->nc_type_class = NC_STRING;
    else
        type->nc_type_class = NC_INT;

    type->endianness = endianness;
    type->size = type_size;
    type->hdr.id = (size_t)xtype;

    *typep = type;
    return NC_NOERR;
}

/**
 * @internal Add a fully configured variable to a netCDF-4 group.
 *
 * @param grp Containing group.
 * @param name Variable name.
 * @param ndims Rank.
 * @param xtype NetCDF type.
 * @param endianness Byte order.
 * @param type_size Size in bytes of one element.
 * @param type_name Human-readable type name.
 * @param fill_value Optional fill value (may be NULL).
 * @param contiguous Non-zero for contiguous storage.
 * @param chunksizes Optional chunk sizes (may be NULL).
 * @param format_var_info Format-specific per-variable metadata (may be NULL).
 * @param var Output variable pointer.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
pdb_var_list_add_full(NC_GRP_INFO_T *grp, const char *name, int ndims,
                    nc_type xtype, int endianness, size_t type_size,
                    char *type_name, void *fill_value, int contiguous,
                    size_t *chunksizes, void *format_var_info,
                    NC_VAR_INFO_T **var)
{
    int d, retval;

    if ((retval = nc4_var_list_add(grp, name, ndims, var)))
        return retval;
    (*var)->created = NC_TRUE;
    (*var)->written_to = NC_TRUE;
    (*var)->format_var_info = format_var_info;
    (*var)->atts_read = 1;

    if ((retval = pdb_set_var_type(xtype, endianness, type_size, type_name,
                                   &(*var)->type_info)))
        return retval;

    (*var)->endianness = (*var)->type_info->endianness;
    (*var)->type_info->rc++;

    if (fill_value)
    {
        if (!((*var)->fill_value = malloc(type_size)))
            return NC_ENOMEM;
        memcpy((*var)->fill_value, fill_value, type_size);
    }

    (*var)->storage = contiguous ? NC_CONTIGUOUS : NC_CHUNKED;

    if (chunksizes)
    {
        if (!((*var)->chunksizes = malloc((size_t)ndims * sizeof(size_t))))
            return NC_ENOMEM;
        for (d = 0; d < ndims; d++)
            (*var)->chunksizes[d] = chunksizes[d];
    }

    return NC_NOERR;
}

/**
 * @internal Add a fixed-length string variable to the root group.
 *
 * Creates the string-length dimension and assigns dimension IDs.
 *
 * @param grp Root group.
 * @param name Variable name.
 * @param atom_dimid Dimension ID for the atom dimension.
 * @param str_len String length.
 * @param var Output variable pointer.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
pdb_add_string_var(NC_GRP_INFO_T *grp, const char *name, int atom_dimid,
                 size_t str_len, NC_VAR_INFO_T **var)
{
    NC_DIM_INFO_T *str_dim;
    char dimname[NC_MAX_NAME + 1];
    int dimids[2];
    int retval;

    snprintf(dimname, NC_MAX_NAME, "%s_len", name);
    if ((retval = nc4_dim_list_add(grp, dimname, str_len, -1, &str_dim)))
        return retval;

    dimids[0] = atom_dimid;
    dimids[1] = (int)str_dim->hdr.id;

    if ((retval = pdb_var_list_add_full(grp, name, 2, NC_CHAR,
                                        NC_ENDIAN_NATIVE, 1, "char",
                                        NULL, 1, NULL, NULL, var)))
        return retval;

    (*var)->dimids[0] = dimids[0];
    (*var)->dimids[1] = dimids[1];

    return NC_NOERR;
}

/**
 * @internal Add a CHAR global attribute to a group.
 *
 * Empty values are skipped, so only records actually present create
 * attributes.
 *
 * @param grp Group receiving the attribute.
 * @param name Attribute name.
 * @param value Attribute string value.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
pdb_add_char_att(NC_GRP_INFO_T *grp, const char *name, const char *value)
{
    NC_ATT_INFO_T *att;
    size_t vlen;
    char *data;
    int retval;

    if (!value || value[0] == '\0')
        return NC_NOERR;

    vlen = strlen(value);

    if ((retval = nc4_att_list_add(grp->att, name, &att)))
        return retval;

    att->nc_typeid = NC_CHAR;
    att->len = vlen;

    if (!(data = malloc(vlen + 1)))
        return NC_ENOMEM;
    memcpy(data, value, vlen + 1);

    att->data = data;
    att->dirty = NC_TRUE;

    return NC_NOERR;
}

/**
 * @internal Parse a legacy PDB file into an in-memory structure.
 *
 * Two-pass parser: first pass counts MODEL blocks and the number of
 * ATOM/HETATM records in the first model, second pass allocates arrays
 * and populates them.
 *
 * @param path Path to the PDB file.
 * @param retvalp Receives error code if parsing fails.
 *
 * @return Pointer to parsed file info, or NULL on error.
 */
static NC_PDB_FILE_INFO_T *
pdb_parse_file(const char *path, int *retvalp)
{
    FILE *f;
    NC_PDB_FILE_INFO_T *pdb_file = NULL;
    char line[PDB_MAX_LINE];
    int nmodels = 0;
    int in_model = 0;
    size_t first_model_atoms = 0;
    int model_idx = 0;
    int atom_idx = 0;
    size_t linelen;

    *retvalp = NC_NOERR;

    f = fopen(path, "r");
    if (!f)
    {
        *retvalp = NC_EINVAL;
        return NULL;
    }

    /* First pass: count models and atoms. */
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "MODEL ", 6) == 0)
        {
            nmodels++;
            in_model = 1;
            if (nmodels == 1)
                first_model_atoms = 0;
        }
        else if (strncmp(line, "ENDMDL", 6) == 0)
        {
            in_model = 0;
        }
        else if (strncmp(line, "ATOM  ", 6) == 0 ||
                 strncmp(line, "HETATM", 6) == 0)
        {
            if (nmodels == 0 || (nmodels == 1 && in_model))
                first_model_atoms++;
        }
    }

    if (nmodels == 0)
        nmodels = 1;

    if (first_model_atoms == 0)
    {
        *retvalp = NC_EINVAL;
        fclose(f);
        return NULL;
    }

    pdb_file = (NC_PDB_FILE_INFO_T *)calloc(1, sizeof(NC_PDB_FILE_INFO_T));
    if (!pdb_file)
    {
        *retvalp = NC_ENOMEM;
        fclose(f);
        return NULL;
    }

    pdb_file->path = strdup(path);
    if (!pdb_file->path)
    {
        *retvalp = NC_ENOMEM;
        goto fail;
    }

    pdb_file->nmodels = nmodels;
    pdb_file->natoms = first_model_atoms;

    /* Allocate coordinate arrays [model][atom]. */
    pdb_file->x = (float *)calloc((size_t)nmodels * first_model_atoms, sizeof(float));
    pdb_file->y = (float *)calloc((size_t)nmodels * first_model_atoms, sizeof(float));
    pdb_file->z = (float *)calloc((size_t)nmodels * first_model_atoms, sizeof(float));

    /* Allocate per-atom identity arrays [atom]. */
    pdb_file->serial = (int *)calloc(first_model_atoms, sizeof(int));
    pdb_file->name = (char *)calloc(first_model_atoms * (PDB_NAME_LEN + 1), sizeof(char));
    pdb_file->res_name = (char *)calloc(first_model_atoms * (PDB_RESNAME_LEN + 1), sizeof(char));
    pdb_file->chain_id = (char *)calloc(first_model_atoms * (PDB_CHAINID_LEN + 1), sizeof(char));
    pdb_file->res_seq = (int *)calloc(first_model_atoms, sizeof(int));
    pdb_file->occupancy = (float *)calloc(first_model_atoms, sizeof(float));
    pdb_file->temp_factor = (float *)calloc(first_model_atoms, sizeof(float));
    pdb_file->element = (char *)calloc(first_model_atoms * (PDB_ELEMENT_LEN + 1), sizeof(char));
    pdb_file->group = (char *)calloc(first_model_atoms * (PDB_GROUP_LEN + 1), sizeof(char));

    if (!pdb_file->x || !pdb_file->y || !pdb_file->z ||
        !pdb_file->serial || !pdb_file->name || !pdb_file->res_name ||
        !pdb_file->chain_id || !pdb_file->res_seq ||
        !pdb_file->occupancy || !pdb_file->temp_factor ||
        !pdb_file->element || !pdb_file->group)
    {
        *retvalp = NC_ENOMEM;
        goto fail;
    }

    /* Second pass: populate arrays and global attributes. */
    rewind(f);
    model_idx = 0;
    atom_idx = 0;
    while (fgets(line, sizeof(line), f))
    {
        linelen = strlen(line);
        if (linelen > 0 && line[linelen - 1] == '\n')
            line[--linelen] = '\0';

        if (strncmp(line, "MODEL ", 6) == 0)
        {
            model_idx++;
            atom_idx = 0;
            continue;
        }
        else if (strncmp(line, "ENDMDL", 6) == 0)
        {
            continue;
        }
        else if (strncmp(line, "ATOM  ", 6) == 0 ||
                 strncmp(line, "HETATM", 6) == 0)
        {
            int is_atom = (strncmp(line, "ATOM  ", 6) == 0);
            int flat_idx;
            char name_trim[16];
            char elem_buf[PDB_ELEMENT_LEN + 1];

            if (atom_idx >= (int)first_model_atoms ||
                (nmodels > 1 && model_idx == 0))
            {
                /* Ignore atoms outside the first model or before the first
                 * MODEL record. */
                continue;
            }

            flat_idx = (model_idx - (nmodels > 1 ? 1 : 0)) * (int)first_model_atoms + atom_idx;

            if (model_idx == 0 || (nmodels > 1 && model_idx == 1))
            {
                /* Identity fields come from the first model only. */
                pdb_parse_int(line, linelen, 7, 11,
                              &pdb_file->serial[atom_idx]);
                pdb_copy_field(line, linelen, 13, 16,
                               &pdb_file->name[atom_idx * (PDB_NAME_LEN + 1)],
                               PDB_NAME_LEN + 1);
                pdb_copy_field(line, linelen, 18, 20,
                               &pdb_file->res_name[atom_idx * (PDB_RESNAME_LEN + 1)],
                               PDB_RESNAME_LEN + 1);
                pdb_copy_field(line, linelen, 22, 22,
                               &pdb_file->chain_id[atom_idx * (PDB_CHAINID_LEN + 1)],
                               PDB_CHAINID_LEN + 1);
                pdb_parse_int(line, linelen, 23, 26,
                              &pdb_file->res_seq[atom_idx]);

                /* Element: field 77-78; infer from atom name if blank. */
                pdb_copy_field(line, linelen, 77, 78, elem_buf,
                               sizeof(elem_buf));
                if (elem_buf[0] == '\0')
                {
                    strcpy(name_trim,
                           &pdb_file->name[atom_idx * (PDB_NAME_LEN + 1)]);
                    pdb_trim(name_trim);
                    if (name_trim[0] && isalpha((unsigned char)name_trim[0]))
                    {
                        elem_buf[0] = name_trim[0];
                        elem_buf[1] = (name_trim[1] && isalpha((unsigned char)name_trim[1]))
                                          ? name_trim[1]
                                          : '\0';
                        elem_buf[2] = '\0';
                    }
                }
                strcpy(&pdb_file->element[atom_idx * (PDB_ELEMENT_LEN + 1)],
                       elem_buf);

                /* ATOM/HETATM discriminator. */
                strcpy(&pdb_file->group[atom_idx * (PDB_GROUP_LEN + 1)],
                       is_atom ? "ATOM" : "HETATM");
            }

            /* Numeric fields are parsed for every model. */
            pdb_parse_float(line, linelen, 31, 38,
                            &pdb_file->x[flat_idx]);
            pdb_parse_float(line, linelen, 39, 46,
                            &pdb_file->y[flat_idx]);
            pdb_parse_float(line, linelen, 47, 54,
                            &pdb_file->z[flat_idx]);
            pdb_parse_float(line, linelen, 55, 60,
                            &pdb_file->occupancy[atom_idx]);
            pdb_parse_float(line, linelen, 61, 66,
                            &pdb_file->temp_factor[atom_idx]);

            /* When no MODEL records exist, identity fields are filled in
             * the same single pass. */
            if (nmodels == 1)
                atom_idx++;
            else if (model_idx > 0)
                atom_idx++;

            continue;
        }
        else if (strncmp(line, "HEADER", 6) == 0)
        {
            char buf[64];
            pdb_copy_field(line, linelen, 63, 66, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->id_code = strdup(buf);

            pdb_copy_field(line, linelen, 11, 50, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->classification = strdup(buf);

            pdb_copy_field(line, linelen, 51, 59, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->dep_date = strdup(buf);

            continue;
        }
        else if (strncmp(line, "TITLE ", 6) == 0)
        {
            char buf[256];
            pdb_copy_field(line, linelen, 11, 80, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
            {
                *retvalp = pdb_append_text(&pdb_file->title, buf);
                if (*retvalp != NC_NOERR)
                    goto fail;
            }
            continue;
        }
        else if (strncmp(line, "COMPND", 6) == 0)
        {
            char buf[256];
            pdb_copy_field(line, linelen, 11, 80, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
            {
                *retvalp = pdb_append_text(&pdb_file->compnd, buf);
                if (*retvalp != NC_NOERR)
                    goto fail;
            }
            continue;
        }
        else if (strncmp(line, "SOURCE", 6) == 0)
        {
            char buf[256];
            pdb_copy_field(line, linelen, 11, 80, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
            {
                *retvalp = pdb_append_text(&pdb_file->source, buf);
                if (*retvalp != NC_NOERR)
                    goto fail;
            }
            continue;
        }
        else if (strncmp(line, "CRYST1", 6) == 0)
        {
            char buf[64];
            pdb_file->has_cryst1 = 1;

            pdb_copy_field(line, linelen, 7, 15, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->cell_a = strdup(buf);

            pdb_copy_field(line, linelen, 16, 24, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->cell_b = strdup(buf);

            pdb_copy_field(line, linelen, 25, 33, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->cell_c = strdup(buf);

            pdb_copy_field(line, linelen, 34, 40, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->cell_alpha = strdup(buf);

            pdb_copy_field(line, linelen, 41, 47, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->cell_beta = strdup(buf);

            pdb_copy_field(line, linelen, 48, 54, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->cell_gamma = strdup(buf);

            pdb_copy_field(line, linelen, 56, 66, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->space_group = strdup(buf);

            pdb_copy_field(line, linelen, 67, 70, buf, sizeof(buf));
            pdb_trim(buf);
            if (buf[0] != '\0')
                pdb_file->symmetry_z = strdup(buf);

            continue;
        }
    }

    fclose(f);
    return pdb_file;

fail:
    fclose(f);
    pdb_free_file_info(pdb_file);
    return NULL;
}

/**
 * @internal Build the netCDF-4 metadata model for a parsed PDB file.
 *
 * Creates dimensions, variables, and global attributes matching the schema
 * in docs/plan/v3.3.0-sprint2-pdb-dispatch-layer.md.
 *
 * @param h5 NetCDF-4 file info struct.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
pdb_build_metadata(NC_FILE_INFO_T *h5)
{
    NC_PDB_FILE_INFO_T *pdb_file;
    NC_GRP_INFO_T *grp;
    NC_DIM_INFO_T *model_dim;
    NC_DIM_INFO_T *atom_dim;
    int model_dimid;
    int atom_dimid;
    NC_VAR_INFO_T *var;
    int dimids[2];
    int retval;

    pdb_file = (NC_PDB_FILE_INFO_T *)h5->format_file_info;
    grp = h5->root_grp;

    /* Dimensions. */
    if ((retval = nc4_dim_list_add(grp, "model", (size_t)pdb_file->nmodels,
                                   -1, &model_dim)))
        return retval;
    model_dimid = (int)model_dim->hdr.id;

    if ((retval = nc4_dim_list_add(grp, "atom", pdb_file->natoms, -1,
                                   &atom_dim)))
        return retval;
    atom_dimid = (int)atom_dim->hdr.id;

    /* Coordinate variables: [model][atom]. */
    dimids[0] = model_dimid;
    dimids[1] = atom_dimid;

    if ((retval = pdb_var_list_add_full(grp, "atom_site_Cartn_x", 2,
                                        NC_FLOAT, NC_ENDIAN_NATIVE,
                                        sizeof(float), "float",
                                        NULL, 1, NULL, NULL, &var)))
        return retval;
    var->dimids[0] = dimids[0];
    var->dimids[1] = dimids[1];

    if ((retval = pdb_var_list_add_full(grp, "atom_site_Cartn_y", 2,
                                        NC_FLOAT, NC_ENDIAN_NATIVE,
                                        sizeof(float), "float",
                                        NULL, 1, NULL, NULL, &var)))
        return retval;
    var->dimids[0] = dimids[0];
    var->dimids[1] = dimids[1];

    if ((retval = pdb_var_list_add_full(grp, "atom_site_Cartn_z", 2,
                                        NC_FLOAT, NC_ENDIAN_NATIVE,
                                        sizeof(float), "float",
                                        NULL, 1, NULL, NULL, &var)))
        return retval;
    var->dimids[0] = dimids[0];
    var->dimids[1] = dimids[1];

    /* Per-atom identity and numeric variables: [atom]. */
    if ((retval = pdb_var_list_add_full(grp, "atom_site_id", 1, NC_INT,
                                        NC_ENDIAN_NATIVE, sizeof(int), "int",
                                        NULL, 1, NULL, NULL, &var)))
        return retval;
    var->dimids[0] = atom_dimid;

    if ((retval = pdb_add_string_var(grp, "atom_site_label_atom_id",
                                     atom_dimid, PDB_NAME_LEN, &var)))
        return retval;

    if ((retval = pdb_add_string_var(grp, "atom_site_label_comp_id",
                                     atom_dimid, PDB_RESNAME_LEN, &var)))
        return retval;

    if ((retval = pdb_add_string_var(grp, "atom_site_auth_asym_id",
                                     atom_dimid, PDB_CHAINID_LEN, &var)))
        return retval;

    if ((retval = pdb_var_list_add_full(grp, "atom_site_auth_seq_id", 1,
                                        NC_INT, NC_ENDIAN_NATIVE,
                                        sizeof(int), "int",
                                        NULL, 1, NULL, NULL, &var)))
        return retval;
    var->dimids[0] = atom_dimid;

    if ((retval = pdb_var_list_add_full(grp, "atom_site_occupancy", 1,
                                        NC_FLOAT, NC_ENDIAN_NATIVE,
                                        sizeof(float), "float",
                                        NULL, 1, NULL, NULL, &var)))
        return retval;
    var->dimids[0] = atom_dimid;

    if ((retval = pdb_var_list_add_full(grp, "atom_site_B_iso_or_equiv", 1,
                                        NC_FLOAT, NC_ENDIAN_NATIVE,
                                        sizeof(float), "float",
                                        NULL, 1, NULL, NULL, &var)))
        return retval;
    var->dimids[0] = atom_dimid;

    if ((retval = pdb_add_string_var(grp, "atom_site_type_symbol",
                                     atom_dimid, PDB_ELEMENT_LEN, &var)))
        return retval;

    if ((retval = pdb_add_string_var(grp, "atom_site_group_PDB",
                                     atom_dimid, PDB_GROUP_LEN, &var)))
        return retval;

    /* Global attributes. */
    if ((retval = pdb_add_char_att(grp, "idCode", pdb_file->id_code)))
        return retval;
    if ((retval = pdb_add_char_att(grp, "classification", pdb_file->classification)))
        return retval;
    if ((retval = pdb_add_char_att(grp, "depDate", pdb_file->dep_date)))
        return retval;
    if ((retval = pdb_add_char_att(grp, "title", pdb_file->title)))
        return retval;
    if ((retval = pdb_add_char_att(grp, "compnd", pdb_file->compnd)))
        return retval;
    if ((retval = pdb_add_char_att(grp, "source", pdb_file->source)))
        return retval;

    if (pdb_file->has_cryst1)
    {
        if ((retval = pdb_add_char_att(grp, "cell_length_a", pdb_file->cell_a)))
            return retval;
        if ((retval = pdb_add_char_att(grp, "cell_length_b", pdb_file->cell_b)))
            return retval;
        if ((retval = pdb_add_char_att(grp, "cell_length_c", pdb_file->cell_c)))
            return retval;
        if ((retval = pdb_add_char_att(grp, "cell_angle_alpha", pdb_file->cell_alpha)))
            return retval;
        if ((retval = pdb_add_char_att(grp, "cell_angle_beta", pdb_file->cell_beta)))
            return retval;
        if ((retval = pdb_add_char_att(grp, "cell_angle_gamma", pdb_file->cell_gamma)))
            return retval;
        if ((retval = pdb_add_char_att(grp, "space_group_name_H-M", pdb_file->space_group)))
            return retval;
        if ((retval = pdb_add_char_att(grp, "symmetry_Z", pdb_file->symmetry_z)))
            return retval;
    }

    return NC_NOERR;
}

/**
 * @internal Open a legacy PDB file for read-only access.
 *
 * V3.3.0 Sprint 2: parses the fixed-column PDB records, validates that
 * at least one ATOM/HETATM record exists, and builds the netCDF-4
 * in-memory metadata model.
 *
 * @param path Path to the PDB file.
 * @param mode Open mode flags.
 * @param basepe Ignored.
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Pointer to dispatch table.
 * @param ncid NetCDF ID assigned to this file.
 *
 * @return NC_NOERR No error.
 * @return NC_EINVAL Invalid parameters or mode flags, or no atoms.
 * @return NC_EPERM Write mode requested.
 * @return NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
int
NC_PDB_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
            void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC *nc;
    NC_FILE_INFO_T *h5;
    NC_PDB_FILE_INFO_T *pdb_file;
    int retval;

    assert(basepe || !basepe);
    assert(chunksizehintp || !chunksizehintp);
    assert(parameters || !parameters);
    assert(dispatch);

    if (!path)
        return NC_EINVAL;

    /* Only read-only access is supported. */
    if (mode & NC_WRITE)
        return NC_EPERM;

    /* Find pointer to NC. */
    if ((retval = NC_check_id(ncid, &nc)))
        return retval;

    /* Add necessary structs to hold netcdf-4 file data. */
    if ((retval = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
        return retval;
    assert(h5 && h5->root_grp);
    h5->no_write = NC_TRUE;
    h5->root_grp->atts_read = 1;

    /* Parse the PDB file into an in-memory representation. */
    if (!(pdb_file = pdb_parse_file(path, &retval)))
    {
        nc4_file_list_del(ncid);
        return retval;
    }

    h5->format_file_info = pdb_file;

    /* Build the netCDF-4 metadata model. */
    if ((retval = pdb_build_metadata(h5)))
    {
        pdb_free_file_info(pdb_file);
        h5->format_file_info = NULL;
        nc4_file_list_del(ncid);
        return retval;
    }

    return NC_NOERR;
}

/**
 * @internal Close a legacy PDB file.
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_PDB_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_PDB_FILE_INFO_T *pdb_file;
    int retval;

    assert(ignore || !ignore);

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get PDB-specific info. */
    pdb_file = (NC_PDB_FILE_INFO_T *)h5->format_file_info;
    pdb_free_file_info(pdb_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Abort opening a legacy PDB file.
 *
 * @param ncid NetCDF ID.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_PDB_abort(int ncid)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_PDB_FILE_INFO_T *pdb_file;
    int retval;

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get PDB-specific info. */
    pdb_file = (NC_PDB_FILE_INFO_T *)h5->format_file_info;
    pdb_free_file_info(pdb_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Inquire the format of a legacy PDB file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_PDB_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * @internal Inquire the extended format of a legacy PDB file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 * @param modep Pointer that gets mode flags.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_PDB_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_PDB;
    if (modep)
        *modep = NC_NOWRITE;
    return NC_NOERR;
}

/**
 * @internal Return the size in bytes of a netCDF numeric/char type.
 *
 * @param xtype NetCDF type.
 *
 * @return Element size in bytes.
 */
static size_t
pdb_type_size(nc_type xtype)
{
    switch (xtype)
    {
    case NC_INT:    return sizeof(int);
    case NC_FLOAT:  return sizeof(float);
    case NC_CHAR:   return 1;
    default:        return 1;
    }
}

/**
 * @internal Read a hyperslab from a 2D [model][atom] float source array.
 *
 * @param src Source array.
 * @param natoms Atom dimension length (fastest varying).
 * @param start Start indices.
 * @param count Counts.
 * @param value Output buffer.
 * @param value_type NetCDF type of output values.
 *
 * @return NC_NOERR No error.
 * @return NC_EINVAL Invalid parameters.
 * @return NC_EBADTYPE Unsupported type conversion.
 */
static int
pdb_read_coord_var(const float *src, size_t natoms,
                   const size_t *start, const size_t *count,
                   void *value, nc_type value_type)
{
    size_t m, a;
    size_t nelems = count[0] * count[1];
    size_t tmp_size = nelems * sizeof(float);
    float *tmp = (float *)malloc(tmp_size);

    if (!tmp)
        return NC_ENOMEM;

    for (m = 0; m < count[0]; m++)
    {
        size_t model_off = (start[0] + m) * natoms;
        for (a = 0; a < count[1]; a++)
            tmp[m * count[1] + a] = src[model_off + start[1] + a];
    }

    if (value_type == NC_FLOAT)
    {
        memcpy(value, tmp, tmp_size);
    }
    else
    {
        int retval = nc4_convert_type(tmp, value, NC_FLOAT, value_type,
                                      nelems, NULL, NULL, 0, 0, 0);
        free(tmp);
        return retval;
    }

    free(tmp);
    return NC_NOERR;
}

/**
 * @internal Read a hyperslab from a 1D per-atom numeric source array.
 *
 * @param src Source array.
 * @param start Start index.
 * @param count Count.
 * @param value Output buffer.
 * @param src_type NetCDF type of source elements.
 * @param value_type NetCDF type of output values.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 * @return NC_EBADTYPE Unsupported type conversion.
 */
static int
pdb_read_1d_var(const void *src, size_t start, size_t count,
                void *value, nc_type src_type, nc_type value_type)
{
    size_t src_size = pdb_type_size(src_type);
    void *tmp = malloc(count * src_size);

    if (!tmp)
        return NC_ENOMEM;

    memcpy(tmp, (const char *)src + start * src_size, count * src_size);

    if (value_type == src_type)
    {
        memcpy(value, tmp, count * src_size);
    }
    else
    {
        int retval = nc4_convert_type(tmp, value, src_type, value_type,
                                      count, NULL, NULL, 0, 0, 0);
        free(tmp);
        return retval;
    }

    free(tmp);
    return NC_NOERR;
}

/**
 * @internal Read a hyperslab from a 2D per-atom fixed-length string array.
 *
 * @param src Source char array [atom][row_len].
 * @param row_len Maximum string length per row.
 * @param start Start indices.
 * @param count Counts.
 * @param value Output buffer.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
pdb_read_string_var(const char *src, size_t row_len,
                    const size_t *start, const size_t *count,
                    void *value)
{
    size_t a, c;
    size_t nelems = count[0] * count[1];
    char *tmp = (char *)malloc(nelems);

    if (!tmp)
        return NC_ENOMEM;

    for (a = 0; a < count[0]; a++)
    {
        const char *row = src + (start[0] + a) * row_len;
        for (c = 0; c < count[1]; c++)
            tmp[a * count[1] + c] = row[start[1] + c];
    }

    memcpy(value, tmp, nelems);
    free(tmp);
    return NC_NOERR;
}

/**
 * @internal Read a hyperslab of data from a legacy PDB variable.
 *
 * All variables are backed by in-memory arrays created when the file is
 * opened. String variables are copied as raw bytes; numeric variables use
 * nc4_convert_type when the requested memory type differs from the
 * variable type.
 *
 * @param ncid NetCDF ID.
 * @param varid Variable ID.
 * @param start Start indices (0-based).
 * @param count Counts.
 * @param value Output buffer.
 * @param memtype Requested memory type.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid or varid.
 * @return NC_EINVAL Invalid parameters.
 * @return NC_EBADTYPE Unsupported type conversion.
 * @author Edward Hartnett
 */
int
NC_PDB_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                void *value, nc_type memtype)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NC_PDB_FILE_INFO_T *pdb_file;
    nc_type xtype;
    int retval;

    if (!start || !count || !value)
        return NC_EINVAL;

    if ((retval = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return retval;

    pdb_file = (NC_PDB_FILE_INFO_T *)h5->format_file_info;
    if (!pdb_file)
        return NC_EBADID;

    xtype = (nc_type)var->type_info->hdr.id;
    if (memtype == NC_NAT)
        memtype = xtype;

    if (strcmp(var->hdr.name, "atom_site_Cartn_x") == 0)
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return pdb_read_coord_var(pdb_file->x, pdb_file->natoms,
                                  start, count, value, memtype);
    }
    else if (strcmp(var->hdr.name, "atom_site_Cartn_y") == 0)
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return pdb_read_coord_var(pdb_file->y, pdb_file->natoms,
                                  start, count, value, memtype);
    }
    else if (strcmp(var->hdr.name, "atom_site_Cartn_z") == 0)
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return pdb_read_coord_var(pdb_file->z, pdb_file->natoms,
                                  start, count, value, memtype);
    }
    else if (strcmp(var->hdr.name, "atom_site_id") == 0)
    {
        if (var->ndims != 1)
            return NC_EINVAL;
        return pdb_read_1d_var(pdb_file->serial, start[0], count[0],
                               value, NC_INT, memtype);
    }
    else if (strcmp(var->hdr.name, "atom_site_auth_seq_id") == 0)
    {
        if (var->ndims != 1)
            return NC_EINVAL;
        return pdb_read_1d_var(pdb_file->res_seq, start[0], count[0],
                               value, NC_INT, memtype);
    }
    else if (strcmp(var->hdr.name, "atom_site_occupancy") == 0)
    {
        if (var->ndims != 1)
            return NC_EINVAL;
        return pdb_read_1d_var(pdb_file->occupancy, start[0], count[0],
                               value, NC_FLOAT, memtype);
    }
    else if (strcmp(var->hdr.name, "atom_site_B_iso_or_equiv") == 0)
    {
        if (var->ndims != 1)
            return NC_EINVAL;
        return pdb_read_1d_var(pdb_file->temp_factor, start[0], count[0],
                               value, NC_FLOAT, memtype);
    }
    else if (strcmp(var->hdr.name, "atom_site_label_atom_id") == 0)
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return pdb_read_string_var(pdb_file->name, PDB_NAME_LEN + 1,
                                   start, count, value);
    }
    else if (strcmp(var->hdr.name, "atom_site_label_comp_id") == 0)
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return pdb_read_string_var(pdb_file->res_name, PDB_RESNAME_LEN + 1,
                                   start, count, value);
    }
    else if (strcmp(var->hdr.name, "atom_site_auth_asym_id") == 0)
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return pdb_read_string_var(pdb_file->chain_id, PDB_CHAINID_LEN + 1,
                                   start, count, value);
    }
    else if (strcmp(var->hdr.name, "atom_site_type_symbol") == 0)
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return pdb_read_string_var(pdb_file->element, PDB_ELEMENT_LEN + 1,
                                   start, count, value);
    }
    else if (strcmp(var->hdr.name, "atom_site_group_PDB") == 0)
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return pdb_read_string_var(pdb_file->group, PDB_GROUP_LEN + 1,
                                   start, count, value);
    }

    return NC_EINVAL;
}
