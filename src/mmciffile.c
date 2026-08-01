/**
 * @file mmciffile.c
 * @brief PDBx/mmCIF User-Defined Format (UDF) dispatch layer.
 *
 * V3.4.0 Sprint 2: read-only STAR/CIF tokenizer and parser for PDBx/mmCIF
 * files. Parses the `_atom_site` loop (coordinates and per-atom identity
 * fields) and the single-row `_entry`, `_struct`, `_cell`, `_symmetry`,
 * and `_pdbx_database_status` categories into a netCDF-4 metadata model,
 * exposed through the UDF8 dispatch table.
 *
 * Known limitations carried forward to later sprints:
 * - `_entity_poly_seq`-derived sequence data is not yet exposed.
 * - `_atom_site_anisotrop` (ANISOU-equivalent) data is not parsed.
 * - Multi-line semicolon-delimited text field values are not supported.
 * - Multi-model (NMR ensemble) mmCIF files are parsed but not yet covered
 *   by tests.
 *
 * @author Edward Hartnett
 * @date 2026-08-01
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nep_nc4.h"
#include "mmcifdispatch.h"

extern int nc4_var_list_add(NC_GRP_INFO_T *grp, const char *name, int ndims,
                            NC_VAR_INFO_T **var);

/** Maximum length of an mmCIF text line this parser will read. */
#define MMCIF_MAX_LINE 2048

/** Maximum length of a single tokenized field (quoted or bare). */
#define MMCIF_TOK_LEN 128

/** Maximum number of items expected in a single `loop_` header. */
#define MMCIF_MAX_ITEMS 64

/** Maximum number of distinct `pdbx_PDB_model_num` values tracked. */
#define MMCIF_MAX_MODELS 256

/** One parsed `_atom_site` loop row, before final array assembly. */
typedef struct
{
    char group_pdb[MMCIF_GROUP_LEN + 1];
    int id;
    char type_symbol[MMCIF_TYPE_SYMBOL_LEN + 1];
    char label_atom_id[MMCIF_ATOM_ID_LEN + 1];
    char label_comp_id[MMCIF_COMP_ID_LEN + 1];
    char auth_asym_id[MMCIF_ASYM_ID_LEN + 1];
    int auth_seq_id;
    double cartn_x, cartn_y, cartn_z;
    double occupancy;
    double b_iso_or_equiv;
    char model_num[16];
} mmcif_atom_row_t;

/** Column indices of the `_atom_site` items this reader understands, or
 * -1 if the file's loop does not declare that item. */
typedef struct
{
    int group_pdb, id, type_symbol, label_atom_id, label_comp_id,
        auth_asym_id, auth_seq_id, cartn_x, cartn_y, cartn_z, occupancy,
        b_iso_or_equiv, model_num;
} mmcif_atom_site_cols_t;

/** Parser state machine modes. */
typedef enum
{
    MMCIF_MODE_NONE,
    MMCIF_MODE_LOOP_HEADER,
    MMCIF_MODE_LOOP_DATA
} mmcif_mode_t;

/**
 * @internal Right-trim whitespace and trailing newline/CR in place.
 *
 * @param s String to trim.
 */
static void
mmcif_rtrim(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (isspace((unsigned char)s[len - 1])))
        s[--len] = '\0';
}

/**
 * @internal Split an mmCIF line into whitespace-separated tokens, treating
 * single- or double-quoted substrings (which may contain embedded spaces)
 * as a single token. Quote characters are stripped from the token value.
 *
 * @param line Input line (already newline-stripped).
 * @param tokens Output array of token buffers.
 * @param max_tokens Number of buffers available in `tokens`.
 *
 * @return Number of tokens found (may be less than `max_tokens`).
 */
static int
mmcif_tokenize(const char *line, char tokens[][MMCIF_TOK_LEN], int max_tokens)
{
    const char *p = line;
    int ntok = 0;

    while (*p && ntok < max_tokens)
    {
        char quote = 0;
        const char *start;
        size_t len;

        while (isspace((unsigned char)*p))
            p++;
        if (!*p)
            break;

        if (*p == '\'' || *p == '"')
        {
            quote = *p;
            p++;
        }

        start = p;
        if (quote)
        {
            while (*p && *p != quote)
                p++;
        }
        else
        {
            while (*p && !isspace((unsigned char)*p))
                p++;
        }

        len = (size_t)(p - start);
        if (len >= MMCIF_TOK_LEN)
            len = MMCIF_TOK_LEN - 1;
        memcpy(tokens[ntok], start, len);
        tokens[ntok][len] = '\0';
        ntok++;

        if (quote && *p == quote)
            p++;
    }

    return ntok;
}

/**
 * @internal Return the item name suffix after the category dot, e.g.
 * "group_PDB" for "_atom_site.group_PDB".
 *
 * @param full_name Full `_category.item` token.
 *
 * @return Pointer into `full_name` just past the dot, or NULL if there is
 * no dot.
 */
static const char *
mmcif_item_suffix(const char *full_name)
{
    const char *dot = strchr(full_name, '.');
    return dot ? dot + 1 : NULL;
}

/**
 * @internal Map the parsed `_atom_site` loop header items to column
 * indices understood by this reader.
 *
 * @param item_names Array of item name suffixes (e.g. "group_PDB").
 * @param nitems Number of items in the loop header.
 * @param cols Output column mapping (fields set to -1 if not found).
 */
static void
mmcif_map_atom_site_cols(char item_names[][MMCIF_TOK_LEN], int nitems,
                         mmcif_atom_site_cols_t *cols)
{
    int i;

    cols->group_pdb = cols->id = cols->type_symbol = cols->label_atom_id =
        cols->label_comp_id = cols->auth_asym_id = cols->auth_seq_id =
            cols->cartn_x = cols->cartn_y = cols->cartn_z =
                cols->occupancy = cols->b_iso_or_equiv = cols->model_num = -1;

    for (i = 0; i < nitems; i++)
    {
        if (!strcmp(item_names[i], "group_PDB"))
            cols->group_pdb = i;
        else if (!strcmp(item_names[i], "id"))
            cols->id = i;
        else if (!strcmp(item_names[i], "type_symbol"))
            cols->type_symbol = i;
        else if (!strcmp(item_names[i], "label_atom_id"))
            cols->label_atom_id = i;
        else if (!strcmp(item_names[i], "label_comp_id"))
            cols->label_comp_id = i;
        else if (!strcmp(item_names[i], "auth_asym_id"))
            cols->auth_asym_id = i;
        else if (!strcmp(item_names[i], "auth_seq_id"))
            cols->auth_seq_id = i;
        else if (!strcmp(item_names[i], "Cartn_x"))
            cols->cartn_x = i;
        else if (!strcmp(item_names[i], "Cartn_y"))
            cols->cartn_y = i;
        else if (!strcmp(item_names[i], "Cartn_z"))
            cols->cartn_z = i;
        else if (!strcmp(item_names[i], "occupancy"))
            cols->occupancy = i;
        else if (!strcmp(item_names[i], "B_iso_or_equiv"))
            cols->b_iso_or_equiv = i;
        else if (!strcmp(item_names[i], "pdbx_PDB_model_num"))
            cols->model_num = i;
    }
}

/**
 * @internal Copy a token value into a fixed-size destination, mapping the
 * `?` (unknown) and `.` (not applicable) mmCIF placeholders to an empty
 * string (which callers subsequently map to the type's fill value).
 *
 * @param dest Destination buffer.
 * @param destlen Size of destination buffer.
 * @param tok Source token.
 */
static void
mmcif_copy_str(char *dest, size_t destlen, const char *tok)
{
    if (!tok || tok[0] == '\0' ||
        (tok[1] == '\0' && (tok[0] == '?' || tok[0] == '.')))
    {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, tok, destlen - 1);
    dest[destlen - 1] = '\0';
}

/**
 * @internal Parse a token as a double, mapping `?`/`.` to 0.0.
 *
 * @param tok Source token.
 *
 * @return Parsed value, or 0.0 for missing/malformed tokens.
 */
static double
mmcif_parse_double(const char *tok)
{
    char *endptr;
    double v;

    if (!tok || tok[0] == '\0' ||
        (tok[1] == '\0' && (tok[0] == '?' || tok[0] == '.')))
        return 0.0;

    v = strtod(tok, &endptr);
    if (endptr == tok)
        return 0.0;
    return v;
}

/**
 * @internal Parse a token as an int, mapping `?`/`.` to 0.
 *
 * @param tok Source token.
 *
 * @return Parsed value, or 0 for missing/malformed tokens.
 */
static int
mmcif_parse_int(const char *tok)
{
    char *endptr;
    long v;

    if (!tok || tok[0] == '\0' ||
        (tok[1] == '\0' && (tok[0] == '?' || tok[0] == '.')))
        return 0;

    v = strtol(tok, &endptr, 10);
    if (endptr == tok)
        return 0;
    return (int)v;
}

/**
 * @internal Assign a single-row category's item value to the matching
 * NC_MMCIF_FILE_INFO_T field, based on the full `_category.item` name.
 *
 * Only the categories/items in the Sprint 2 schema are recognized;
 * everything else is silently ignored.
 *
 * @param mmcif_file File info struct being populated.
 * @param full_name Full `_category.item` token.
 * @param value_tok Raw value token (quotes already stripped).
 */
static void
mmcif_assign_scalar(NC_MMCIF_FILE_INFO_T *mmcif_file, const char *full_name,
                    const char *value_tok)
{
    char buf[256];

    mmcif_copy_str(buf, sizeof(buf), value_tok);
    if (buf[0] == '\0')
        return;

    if (!strcmp(full_name, "_entry.id"))
        mmcif_file->entry_id = strdup(buf);
    else if (!strcmp(full_name, "_struct.title"))
        mmcif_file->struct_title = strdup(buf);
    else if (!strcmp(full_name, "_pdbx_database_status.recvd_initial_deposition_date"))
        mmcif_file->dep_date = strdup(buf);
    else if (!strcmp(full_name, "_cell.length_a"))
    {
        mmcif_file->cell_length_a = strdup(buf);
        mmcif_file->has_cell = 1;
    }
    else if (!strcmp(full_name, "_cell.length_b"))
        mmcif_file->cell_length_b = strdup(buf);
    else if (!strcmp(full_name, "_cell.length_c"))
        mmcif_file->cell_length_c = strdup(buf);
    else if (!strcmp(full_name, "_cell.angle_alpha"))
        mmcif_file->cell_angle_alpha = strdup(buf);
    else if (!strcmp(full_name, "_cell.angle_beta"))
        mmcif_file->cell_angle_beta = strdup(buf);
    else if (!strcmp(full_name, "_cell.angle_gamma"))
        mmcif_file->cell_angle_gamma = strdup(buf);
    else if (!strcmp(full_name, "_symmetry.space_group_name_H-M"))
    {
        mmcif_file->space_group_name = strdup(buf);
        mmcif_file->has_symmetry = 1;
    }
    else if (!strcmp(full_name, "_symmetry.Int_Tables_number"))
        mmcif_file->int_tables_number = strdup(buf);
}

/**
 * @internal Free all memory owned by a NC_MMCIF_FILE_INFO_T struct.
 *
 * @param mmcif_file mmCIF file info to free.
 */
static void
mmcif_free_file_info(NC_MMCIF_FILE_INFO_T *mmcif_file)
{
    if (!mmcif_file)
        return;

    free(mmcif_file->path);
    free(mmcif_file->x);
    free(mmcif_file->y);
    free(mmcif_file->z);
    free(mmcif_file->id);
    free(mmcif_file->label_atom_id);
    free(mmcif_file->label_comp_id);
    free(mmcif_file->auth_asym_id);
    free(mmcif_file->auth_seq_id);
    free(mmcif_file->occupancy);
    free(mmcif_file->b_iso_or_equiv);
    free(mmcif_file->type_symbol);
    free(mmcif_file->group_pdb);
    free(mmcif_file->entry_id);
    free(mmcif_file->struct_title);
    free(mmcif_file->dep_date);
    free(mmcif_file->cell_length_a);
    free(mmcif_file->cell_length_b);
    free(mmcif_file->cell_length_c);
    free(mmcif_file->cell_angle_alpha);
    free(mmcif_file->cell_angle_beta);
    free(mmcif_file->cell_angle_gamma);
    free(mmcif_file->space_group_name);
    free(mmcif_file->int_tables_number);

    free(mmcif_file);
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
mmcif_set_var_type(nc_type xtype, int endianness, size_t type_size,
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
 * @param var Output variable pointer.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
mmcif_var_list_add_full(NC_GRP_INFO_T *grp, const char *name, int ndims,
                        nc_type xtype, int endianness, size_t type_size,
                        char *type_name, NC_VAR_INFO_T **var)
{
    int retval;

    if ((retval = nc4_var_list_add(grp, name, ndims, var)))
        return retval;
    (*var)->created = NC_TRUE;
    (*var)->written_to = NC_TRUE;
    (*var)->atts_read = 1;

    if ((retval = mmcif_set_var_type(xtype, endianness, type_size, type_name,
                                     &(*var)->type_info)))
        return retval;

    (*var)->endianness = (*var)->type_info->endianness;
    (*var)->type_info->rc++;
    (*var)->storage = NC_CONTIGUOUS;

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
mmcif_add_string_var(NC_GRP_INFO_T *grp, const char *name, int atom_dimid,
                     size_t str_len, NC_VAR_INFO_T **var)
{
    NC_DIM_INFO_T *str_dim;
    char dimname[NC_MAX_NAME + 1];
    int retval;

    snprintf(dimname, NC_MAX_NAME, "%s_len", name);
    if ((retval = nc4_dim_list_add(grp, dimname, str_len, -1, &str_dim)))
        return retval;

    if ((retval = mmcif_var_list_add_full(grp, name, 2, NC_CHAR,
                                          NC_ENDIAN_NATIVE, 1, "char", var)))
        return retval;

    (*var)->dimids[0] = atom_dimid;
    (*var)->dimids[1] = (int)str_dim->hdr.id;

    return NC_NOERR;
}

/**
 * @internal Add a CHAR global attribute to a group.
 *
 * Empty values are skipped, so only categories/items actually present in
 * the file create attributes.
 *
 * @param grp Group receiving the attribute.
 * @param name Attribute name.
 * @param value Attribute string value.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
mmcif_add_char_att(NC_GRP_INFO_T *grp, const char *name, const char *value)
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
 * @internal Grow the dynamic `_atom_site` row array by one slot.
 *
 * @param rows Pointer to the row array (may be reallocated).
 * @param nrows Pointer to the current row count (incremented on success).
 * @param capacity Pointer to the current allocated capacity (updated).
 *
 * @return Pointer to the new row slot, or NULL on allocation failure.
 */
static mmcif_atom_row_t *
mmcif_grow_rows(mmcif_atom_row_t **rows, size_t *nrows, size_t *capacity)
{
    if (*nrows == *capacity)
    {
        size_t new_capacity = *capacity ? *capacity * 2 : 256;
        mmcif_atom_row_t *new_rows = (mmcif_atom_row_t *)realloc(
            *rows, new_capacity * sizeof(mmcif_atom_row_t));
        if (!new_rows)
            return NULL;
        *rows = new_rows;
        *capacity = new_capacity;
    }
    return &(*rows)[(*nrows)++];
}

/**
 * @internal Parse a PDBx/mmCIF file into an in-memory structure.
 *
 * Single-pass STAR/CIF tokenizer: recognizes `loop_` blocks and
 * single-row `_category.item value` key-value pairs. Only the
 * `_atom_site` loop and the `_entry`/`_struct`/`_cell`/`_symmetry`/
 * `_pdbx_database_status` single-row categories are extracted; all other
 * categories are ignored. Multi-line semicolon text fields are not
 * supported (documented limitation).
 *
 * @param path Path to the mmCIF file.
 * @param retvalp Receives error code if parsing fails.
 *
 * @return Pointer to parsed file info, or NULL on error.
 */
static NC_MMCIF_FILE_INFO_T *
mmcif_parse_file(const char *path, int *retvalp)
{
    FILE *f;
    char line[MMCIF_MAX_LINE];
    mmcif_mode_t mode = MMCIF_MODE_NONE;
    char item_names[MMCIF_MAX_ITEMS][MMCIF_TOK_LEN];
    int nitems = 0;
    int in_atom_site_loop = 0;
    mmcif_atom_site_cols_t cols;
    mmcif_atom_row_t *rows = NULL;
    size_t nrows = 0, capacity = 0;
    int seen_data_block = 0;
    NC_MMCIF_FILE_INFO_T *mmcif_file = NULL;
    char model_list[MMCIF_MAX_MODELS][16];
    int nmodel_values = 0;
    int model_counts[MMCIF_MAX_MODELS];
    size_t i;
    int m;

    *retvalp = NC_NOERR;
    memset(&cols, -1, sizeof(cols));
    memset(model_counts, 0, sizeof(model_counts));

    f = fopen(path, "r");
    if (!f)
    {
        *retvalp = NC_EINVAL;
        return NULL;
    }

    while (fgets(line, sizeof(line), f))
    {
        char tokens[3][MMCIF_TOK_LEN];
        int ntok;
        size_t linelen = strlen(line);

        if (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            line[--linelen] = '\0';
        if (linelen > 0 && line[linelen - 1] == '\r')
            line[--linelen] = '\0';
        mmcif_rtrim(line);

        if (line[0] == '\0' || line[0] == '#')
            continue;

        if (!strncmp(line, "data_", 5))
        {
            /* Only the first data block is parsed; later blocks (rare for
             * PDBx entries) are ignored. */
            if (seen_data_block)
                break;
            seen_data_block = 1;
            mode = MMCIF_MODE_NONE;
            continue;
        }

        if (!strncmp(line, "loop_", 5))
        {
            mode = MMCIF_MODE_LOOP_HEADER;
            nitems = 0;
            in_atom_site_loop = 0;
            continue;
        }

        if (mode == MMCIF_MODE_LOOP_HEADER)
        {
            if (line[0] == '_')
            {
                const char *suffix;

                ntok = mmcif_tokenize(line, tokens, 1);
                if (ntok < 1)
                    continue;
                suffix = mmcif_item_suffix(tokens[0]);
                if (suffix && nitems < MMCIF_MAX_ITEMS)
                {
                    strncpy(item_names[nitems], suffix, MMCIF_TOK_LEN - 1);
                    item_names[nitems][MMCIF_TOK_LEN - 1] = '\0';
                    nitems++;
                }
                continue;
            }

            /* First non-item line: header is complete, this line is
             * already the first data row. Determine whether this loop is
             * `_atom_site` by checking for two of its distinguishing
             * items among the collected header items. */
            {
                int has_group_pdb = 0, has_cartn_x = 0;
                int k;
                for (k = 0; k < nitems; k++)
                {
                    if (!strcmp(item_names[k], "group_PDB"))
                        has_group_pdb = 1;
                    if (!strcmp(item_names[k], "Cartn_x"))
                        has_cartn_x = 1;
                }
                in_atom_site_loop = (has_group_pdb && has_cartn_x);
            }
            if (in_atom_site_loop)
                mmcif_map_atom_site_cols(item_names, nitems, &cols);
            mode = MMCIF_MODE_LOOP_DATA;
            /* Fall through to process this line as a data row below. */
        }

        if (mode == MMCIF_MODE_LOOP_DATA)
        {
            if (line[0] == '_')
            {
                /* Loop implicitly ended; this is a new single key-value
                 * pair. (A `loop_` line is already handled above, before
                 * this check is reached.) */
                mode = MMCIF_MODE_NONE;
                in_atom_site_loop = 0;
            }
            else
            {
                if (in_atom_site_loop)
                {
                    char row_tokens[MMCIF_MAX_ITEMS][MMCIF_TOK_LEN];
                    int nrow_tok = mmcif_tokenize(line, row_tokens, nitems);
                    mmcif_atom_row_t *row;

                    if (nrow_tok < nitems)
                        continue; /* malformed/truncated row; skip */

                    if (!(row = mmcif_grow_rows(&rows, &nrows, &capacity)))
                    {
                        *retvalp = NC_ENOMEM;
                        goto fail;
                    }

                    mmcif_copy_str(row->group_pdb, sizeof(row->group_pdb),
                                  cols.group_pdb >= 0 ? row_tokens[cols.group_pdb] : "");
                    row->id = cols.id >= 0 ? mmcif_parse_int(row_tokens[cols.id]) : 0;
                    mmcif_copy_str(row->type_symbol, sizeof(row->type_symbol),
                                  cols.type_symbol >= 0 ? row_tokens[cols.type_symbol] : "");
                    mmcif_copy_str(row->label_atom_id, sizeof(row->label_atom_id),
                                  cols.label_atom_id >= 0 ? row_tokens[cols.label_atom_id] : "");
                    mmcif_copy_str(row->label_comp_id, sizeof(row->label_comp_id),
                                  cols.label_comp_id >= 0 ? row_tokens[cols.label_comp_id] : "");
                    mmcif_copy_str(row->auth_asym_id, sizeof(row->auth_asym_id),
                                  cols.auth_asym_id >= 0 ? row_tokens[cols.auth_asym_id] : "");
                    row->auth_seq_id = cols.auth_seq_id >= 0 ? mmcif_parse_int(row_tokens[cols.auth_seq_id]) : 0;
                    row->cartn_x = cols.cartn_x >= 0 ? mmcif_parse_double(row_tokens[cols.cartn_x]) : 0.0;
                    row->cartn_y = cols.cartn_y >= 0 ? mmcif_parse_double(row_tokens[cols.cartn_y]) : 0.0;
                    row->cartn_z = cols.cartn_z >= 0 ? mmcif_parse_double(row_tokens[cols.cartn_z]) : 0.0;
                    row->occupancy = cols.occupancy >= 0 ? mmcif_parse_double(row_tokens[cols.occupancy]) : 0.0;
                    row->b_iso_or_equiv = cols.b_iso_or_equiv >= 0 ? mmcif_parse_double(row_tokens[cols.b_iso_or_equiv]) : 0.0;
                    if (cols.model_num >= 0)
                        mmcif_copy_str(row->model_num, sizeof(row->model_num),
                                      row_tokens[cols.model_num]);
                    else
                        strcpy(row->model_num, "1");
                    if (row->model_num[0] == '\0')
                        strcpy(row->model_num, "1");
                }
                continue;
            }
        }

    }

    fclose(f);
    f = NULL;

    if (nrows == 0)
    {
        /* No _atom_site category found: reject as not-PDBx or empty
         * structure. This doubles as the discriminator against generic
         * (small-molecule) CIF files that lack atomic coordinates. */
        *retvalp = NC_EINVAL;
        free(rows);
        return NULL;
    }

    /* Determine distinct model numbers, in order of first appearance. */
    for (i = 0; i < nrows; i++)
    {
        int found = 0;
        for (m = 0; m < nmodel_values; m++)
        {
            if (!strcmp(model_list[m], rows[i].model_num))
            {
                found = 1;
                break;
            }
        }
        if (!found && nmodel_values < MMCIF_MAX_MODELS)
        {
            strncpy(model_list[nmodel_values], rows[i].model_num, 15);
            model_list[nmodel_values][15] = '\0';
            nmodel_values++;
        }
    }
    if (nmodel_values == 0)
        nmodel_values = 1;

    for (i = 0; i < nrows; i++)
    {
        for (m = 0; m < nmodel_values; m++)
        {
            if (!strcmp(model_list[m], rows[i].model_num))
            {
                model_counts[m]++;
                break;
            }
        }
    }

    if (!(mmcif_file = (NC_MMCIF_FILE_INFO_T *)calloc(1, sizeof(NC_MMCIF_FILE_INFO_T))))
    {
        *retvalp = NC_ENOMEM;
        free(rows);
        return NULL;
    }

    if (!(mmcif_file->path = strdup(path)))
    {
        *retvalp = NC_ENOMEM;
        goto fail2;
    }

    mmcif_file->nmodels = nmodel_values;
    mmcif_file->natoms = (size_t)model_counts[0];

    mmcif_file->x = (double *)calloc((size_t)nmodel_values * mmcif_file->natoms, sizeof(double));
    mmcif_file->y = (double *)calloc((size_t)nmodel_values * mmcif_file->natoms, sizeof(double));
    mmcif_file->z = (double *)calloc((size_t)nmodel_values * mmcif_file->natoms, sizeof(double));
    mmcif_file->id = (int *)calloc(mmcif_file->natoms, sizeof(int));
    mmcif_file->label_atom_id = (char *)calloc(mmcif_file->natoms * (MMCIF_ATOM_ID_LEN + 1), sizeof(char));
    mmcif_file->label_comp_id = (char *)calloc(mmcif_file->natoms * (MMCIF_COMP_ID_LEN + 1), sizeof(char));
    mmcif_file->auth_asym_id = (char *)calloc(mmcif_file->natoms * (MMCIF_ASYM_ID_LEN + 1), sizeof(char));
    mmcif_file->auth_seq_id = (int *)calloc(mmcif_file->natoms, sizeof(int));
    mmcif_file->occupancy = (double *)calloc(mmcif_file->natoms, sizeof(double));
    mmcif_file->b_iso_or_equiv = (double *)calloc(mmcif_file->natoms, sizeof(double));
    mmcif_file->type_symbol = (char *)calloc(mmcif_file->natoms * (MMCIF_TYPE_SYMBOL_LEN + 1), sizeof(char));
    mmcif_file->group_pdb = (char *)calloc(mmcif_file->natoms * (MMCIF_GROUP_LEN + 1), sizeof(char));

    if (!mmcif_file->x || !mmcif_file->y || !mmcif_file->z || !mmcif_file->id ||
        !mmcif_file->label_atom_id || !mmcif_file->label_comp_id ||
        !mmcif_file->auth_asym_id || !mmcif_file->auth_seq_id ||
        !mmcif_file->occupancy || !mmcif_file->b_iso_or_equiv ||
        !mmcif_file->type_symbol || !mmcif_file->group_pdb)
    {
        *retvalp = NC_ENOMEM;
        goto fail2;
    }

    /* Populate identity fields from the first model's rows, and
     * coordinates for every model (assumes each model contributes the
     * same atom count as the first, in file order; untested for
     * nmodels > 1 since all current test files are single-model). */
    {
        int model_pos[MMCIF_MAX_MODELS];
        memset(model_pos, 0, sizeof(model_pos));

        for (i = 0; i < nrows; i++)
        {
            mmcif_atom_row_t *row = &rows[i];
            int model_idx = 0;
            size_t atom_idx;

            for (m = 0; m < nmodel_values; m++)
            {
                if (!strcmp(model_list[m], row->model_num))
                {
                    model_idx = m;
                    break;
                }
            }

            atom_idx = (size_t)model_pos[model_idx];
            if (atom_idx >= mmcif_file->natoms)
                continue; /* extra rows beyond the first model's count */
            model_pos[model_idx]++;

            mmcif_file->x[(size_t)model_idx * mmcif_file->natoms + atom_idx] = row->cartn_x;
            mmcif_file->y[(size_t)model_idx * mmcif_file->natoms + atom_idx] = row->cartn_y;
            mmcif_file->z[(size_t)model_idx * mmcif_file->natoms + atom_idx] = row->cartn_z;

            if (model_idx == 0)
            {
                mmcif_file->id[atom_idx] = row->id;
                strcpy(&mmcif_file->label_atom_id[atom_idx * (MMCIF_ATOM_ID_LEN + 1)], row->label_atom_id);
                strcpy(&mmcif_file->label_comp_id[atom_idx * (MMCIF_COMP_ID_LEN + 1)], row->label_comp_id);
                strcpy(&mmcif_file->auth_asym_id[atom_idx * (MMCIF_ASYM_ID_LEN + 1)], row->auth_asym_id);
                mmcif_file->auth_seq_id[atom_idx] = row->auth_seq_id;
                mmcif_file->occupancy[atom_idx] = row->occupancy;
                mmcif_file->b_iso_or_equiv[atom_idx] = row->b_iso_or_equiv;
                strcpy(&mmcif_file->type_symbol[atom_idx * (MMCIF_TYPE_SYMBOL_LEN + 1)], row->type_symbol);
                strcpy(&mmcif_file->group_pdb[atom_idx * (MMCIF_GROUP_LEN + 1)], row->group_pdb);
            }
        }
    }

    free(rows);
    rows = NULL;

    /* Second lightweight pass to collect single-row category attributes;
     * simpler and more robust than threading attribute assignment through
     * the state machine above. */
    f = fopen(path, "r");
    if (!f)
    {
        *retvalp = NC_EINVAL;
        goto fail2;
    }
    mode = MMCIF_MODE_NONE;
    seen_data_block = 0;
    while (fgets(line, sizeof(line), f))
    {
        char tokens[2][MMCIF_TOK_LEN];
        int ntok;
        size_t linelen = strlen(line);

        if (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            line[--linelen] = '\0';
        if (linelen > 0 && line[linelen - 1] == '\r')
            line[--linelen] = '\0';
        mmcif_rtrim(line);

        if (line[0] == '\0' || line[0] == '#')
            continue;

        if (!strncmp(line, "data_", 5))
        {
            if (seen_data_block)
                break;
            seen_data_block = 1;
            mode = MMCIF_MODE_NONE;
            continue;
        }

        if (!strncmp(line, "loop_", 5))
        {
            mode = MMCIF_MODE_LOOP_HEADER;
            continue;
        }

        if (mode == MMCIF_MODE_LOOP_HEADER)
        {
            if (line[0] == '_')
                continue;
            mode = MMCIF_MODE_LOOP_DATA;
            continue;
        }

        if (mode == MMCIF_MODE_LOOP_DATA)
        {
            if (line[0] == '_')
                mode = MMCIF_MODE_NONE;
            else
                continue;
        }

        if (mode == MMCIF_MODE_NONE && line[0] == '_')
        {
            ntok = mmcif_tokenize(line, tokens, 2);
            if (ntok < 2)
                continue;
            mmcif_assign_scalar(mmcif_file, tokens[0], tokens[1]);
        }
    }
    fclose(f);

    return mmcif_file;

fail:
    if (f)
        fclose(f);
    free(rows);
    return NULL;

fail2:
    free(rows);
    mmcif_free_file_info(mmcif_file);
    return NULL;
}

/**
 * @internal Build the netCDF-4 metadata model for a parsed mmCIF file.
 *
 * Creates dimensions, variables, and global attributes matching the
 * schema in docs/plan/v3.4.0-sprint2-mmcif-dispatch-layer.md.
 *
 * @param h5 NetCDF-4 file info struct.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
mmcif_build_metadata(NC_FILE_INFO_T *h5)
{
    NC_MMCIF_FILE_INFO_T *mmcif_file;
    NC_GRP_INFO_T *grp;
    NC_DIM_INFO_T *model_dim;
    NC_DIM_INFO_T *atom_dim;
    int model_dimid;
    int atom_dimid;
    NC_VAR_INFO_T *var;
    int retval;

    mmcif_file = (NC_MMCIF_FILE_INFO_T *)h5->format_file_info;
    grp = h5->root_grp;

    if ((retval = nc4_dim_list_add(grp, "model", (size_t)mmcif_file->nmodels,
                                   -1, &model_dim)))
        return retval;
    model_dimid = (int)model_dim->hdr.id;

    if ((retval = nc4_dim_list_add(grp, "atom", mmcif_file->natoms, -1,
                                   &atom_dim)))
        return retval;
    atom_dimid = (int)atom_dim->hdr.id;

    /* Coordinate variables: [model][atom]. */
    if ((retval = mmcif_var_list_add_full(grp, "atom_site_Cartn_x", 2,
                                          NC_DOUBLE, NC_ENDIAN_NATIVE,
                                          sizeof(double), "double", &var)))
        return retval;
    var->dimids[0] = model_dimid;
    var->dimids[1] = atom_dimid;

    if ((retval = mmcif_var_list_add_full(grp, "atom_site_Cartn_y", 2,
                                          NC_DOUBLE, NC_ENDIAN_NATIVE,
                                          sizeof(double), "double", &var)))
        return retval;
    var->dimids[0] = model_dimid;
    var->dimids[1] = atom_dimid;

    if ((retval = mmcif_var_list_add_full(grp, "atom_site_Cartn_z", 2,
                                          NC_DOUBLE, NC_ENDIAN_NATIVE,
                                          sizeof(double), "double", &var)))
        return retval;
    var->dimids[0] = model_dimid;
    var->dimids[1] = atom_dimid;

    /* Per-atom identity and numeric variables: [atom]. */
    if ((retval = mmcif_var_list_add_full(grp, "atom_site_id", 1, NC_INT,
                                          NC_ENDIAN_NATIVE, sizeof(int),
                                          "int", &var)))
        return retval;
    var->dimids[0] = atom_dimid;

    if ((retval = mmcif_add_string_var(grp, "atom_site_label_atom_id",
                                       atom_dimid, MMCIF_ATOM_ID_LEN, &var)))
        return retval;

    if ((retval = mmcif_add_string_var(grp, "atom_site_label_comp_id",
                                       atom_dimid, MMCIF_COMP_ID_LEN, &var)))
        return retval;

    if ((retval = mmcif_add_string_var(grp, "atom_site_auth_asym_id",
                                       atom_dimid, MMCIF_ASYM_ID_LEN, &var)))
        return retval;

    if ((retval = mmcif_var_list_add_full(grp, "atom_site_auth_seq_id", 1,
                                          NC_INT, NC_ENDIAN_NATIVE,
                                          sizeof(int), "int", &var)))
        return retval;
    var->dimids[0] = atom_dimid;

    if ((retval = mmcif_var_list_add_full(grp, "atom_site_occupancy", 1,
                                          NC_DOUBLE, NC_ENDIAN_NATIVE,
                                          sizeof(double), "double", &var)))
        return retval;
    var->dimids[0] = atom_dimid;

    if ((retval = mmcif_var_list_add_full(grp, "atom_site_B_iso_or_equiv", 1,
                                          NC_DOUBLE, NC_ENDIAN_NATIVE,
                                          sizeof(double), "double", &var)))
        return retval;
    var->dimids[0] = atom_dimid;

    if ((retval = mmcif_add_string_var(grp, "atom_site_type_symbol",
                                       atom_dimid, MMCIF_TYPE_SYMBOL_LEN, &var)))
        return retval;

    if ((retval = mmcif_add_string_var(grp, "atom_site_group_PDB",
                                       atom_dimid, MMCIF_GROUP_LEN, &var)))
        return retval;

    /* Global attributes. */
    if ((retval = mmcif_add_char_att(grp, "entry_id", mmcif_file->entry_id)))
        return retval;
    if ((retval = mmcif_add_char_att(grp, "struct_title", mmcif_file->struct_title)))
        return retval;
    if ((retval = mmcif_add_char_att(grp, "pdbx_database_status_recvd_initial_deposition_date",
                                     mmcif_file->dep_date)))
        return retval;

    if (mmcif_file->has_cell)
    {
        if ((retval = mmcif_add_char_att(grp, "cell_length_a", mmcif_file->cell_length_a)))
            return retval;
        if ((retval = mmcif_add_char_att(grp, "cell_length_b", mmcif_file->cell_length_b)))
            return retval;
        if ((retval = mmcif_add_char_att(grp, "cell_length_c", mmcif_file->cell_length_c)))
            return retval;
        if ((retval = mmcif_add_char_att(grp, "cell_angle_alpha", mmcif_file->cell_angle_alpha)))
            return retval;
        if ((retval = mmcif_add_char_att(grp, "cell_angle_beta", mmcif_file->cell_angle_beta)))
            return retval;
        if ((retval = mmcif_add_char_att(grp, "cell_angle_gamma", mmcif_file->cell_angle_gamma)))
            return retval;
    }

    if (mmcif_file->has_symmetry)
    {
        if ((retval = mmcif_add_char_att(grp, "symmetry_space_group_name_H-M",
                                         mmcif_file->space_group_name)))
            return retval;
        if ((retval = mmcif_add_char_att(grp, "symmetry_Int_Tables_number",
                                         mmcif_file->int_tables_number)))
            return retval;
    }

    return NC_NOERR;
}

/**
 * @internal Open a PDBx/mmCIF file.
 *
 * V3.4.0 Sprint 2: parses the STAR/CIF `_atom_site` loop and single-row
 * metadata categories, validates that at least one `_atom_site` row
 * exists, and builds the netCDF-4 in-memory metadata model. Read-only
 * access is enforced.
 *
 * @param path Path to the mmCIF file.
 * @param mode Open mode (must not include NC_WRITE).
 * @param basepe Ignored.
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Pointer to dispatch table.
 * @param ncid NetCDF ID assigned to this file.
 *
 * @return NC_NOERR No error.
 * @return NC_EINVAL Invalid parameters, or no `_atom_site` rows found.
 * @return NC_EPERM Write mode requested.
 * @return NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
int
NC_MMCIF_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
              void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC_FILE_INFO_T *h5;
    NC_MMCIF_FILE_INFO_T *mmcif_file;
    int retval;

    (void)basepe;
    (void)chunksizehintp;
    (void)parameters;
    (void)dispatch;

    if (!path)
        return NC_EINVAL;

    /* Only read-only access is supported. */
    if (mode & NC_WRITE)
        return NC_EPERM;

    /* Add necessary structs to hold netcdf-4 file data. */
    if ((retval = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
        return retval;
    assert(h5 && h5->root_grp);
    h5->no_write = NC_TRUE;
    h5->root_grp->atts_read = 1;

    /* Parse the mmCIF file into an in-memory representation. */
    if (!(mmcif_file = mmcif_parse_file(path, &retval)))
    {
        nc4_file_list_del(ncid);
        return retval;
    }

    h5->format_file_info = mmcif_file;

    /* Build the netCDF-4 metadata model. */
    if ((retval = mmcif_build_metadata(h5)))
    {
        mmcif_free_file_info(mmcif_file);
        h5->format_file_info = NULL;
        nc4_file_list_del(ncid);
        return retval;
    }

    return NC_NOERR;
}

/**
 * @internal Close a PDBx/mmCIF file.
 *
 * Frees the file-specific state allocated in NC_MMCIF_open().
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_MMCIF_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_MMCIF_FILE_INFO_T *mmcif_file;
    int retval;

    (void)ignore;

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get mmCIF-specific info. */
    mmcif_file = (NC_MMCIF_FILE_INFO_T *)h5->format_file_info;
    mmcif_free_file_info(mmcif_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Abort opening a PDBx/mmCIF file.
 *
 * @param ncid NetCDF ID.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_MMCIF_abort(int ncid)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_MMCIF_FILE_INFO_T *mmcif_file;
    int retval;

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get mmCIF-specific info. */
    mmcif_file = (NC_MMCIF_FILE_INFO_T *)h5->format_file_info;
    mmcif_free_file_info(mmcif_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Inquire the format of a PDBx/mmCIF file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_MMCIF_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * @internal Inquire the extended format of a PDBx/mmCIF file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 * @param modep Pointer that gets mode flags.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_MMCIF_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_MMCIF;
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
mmcif_type_size(nc_type xtype)
{
    switch (xtype)
    {
    case NC_INT:    return sizeof(int);
    case NC_DOUBLE: return sizeof(double);
    case NC_CHAR:   return 1;
    default:        return 1;
    }
}

/**
 * @internal Read a hyperslab from a 2D [model][atom] double source array.
 *
 * @param src Source array.
 * @param natoms Atom dimension length (fastest varying).
 * @param start Start indices.
 * @param count Counts.
 * @param value Output buffer.
 * @param value_type NetCDF type of output values.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 * @return NC_EBADTYPE Unsupported type conversion.
 */
static int
mmcif_read_coord_var(const double *src, size_t natoms,
                     const size_t *start, const size_t *count,
                     void *value, nc_type value_type)
{
    size_t m, a;
    size_t nelems = count[0] * count[1];
    size_t tmp_size = nelems * sizeof(double);
    double *tmp = (double *)malloc(tmp_size);

    if (!tmp)
        return NC_ENOMEM;

    for (m = 0; m < count[0]; m++)
    {
        size_t model_off = (start[0] + m) * natoms;
        for (a = 0; a < count[1]; a++)
            tmp[m * count[1] + a] = src[model_off + start[1] + a];
    }

    if (value_type == NC_DOUBLE)
    {
        memcpy(value, tmp, tmp_size);
    }
    else
    {
        int retval = nc4_convert_type(tmp, value, NC_DOUBLE, value_type,
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
mmcif_read_1d_var(const void *src, size_t start, size_t count,
                  void *value, nc_type src_type, nc_type value_type)
{
    size_t src_size = mmcif_type_size(src_type);
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
mmcif_read_string_var(const char *src, size_t row_len,
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
 * @internal Read a hyperslab of data from a PDBx/mmCIF variable.
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
NC_MMCIF_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                  void *value, nc_type memtype)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NC_MMCIF_FILE_INFO_T *mmcif_file;
    nc_type xtype;
    int retval;

    if (!start || !count || !value)
        return NC_EINVAL;

    if ((retval = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return retval;

    mmcif_file = (NC_MMCIF_FILE_INFO_T *)h5->format_file_info;
    if (!mmcif_file)
        return NC_EBADID;

    xtype = (nc_type)var->type_info->hdr.id;
    if (memtype == NC_NAT)
        memtype = xtype;

    if (!strcmp(var->hdr.name, "atom_site_Cartn_x"))
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return mmcif_read_coord_var(mmcif_file->x, mmcif_file->natoms,
                                    start, count, value, memtype);
    }
    else if (!strcmp(var->hdr.name, "atom_site_Cartn_y"))
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return mmcif_read_coord_var(mmcif_file->y, mmcif_file->natoms,
                                    start, count, value, memtype);
    }
    else if (!strcmp(var->hdr.name, "atom_site_Cartn_z"))
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return mmcif_read_coord_var(mmcif_file->z, mmcif_file->natoms,
                                    start, count, value, memtype);
    }
    else if (!strcmp(var->hdr.name, "atom_site_id"))
    {
        if (var->ndims != 1)
            return NC_EINVAL;
        return mmcif_read_1d_var(mmcif_file->id, start[0], count[0],
                                 value, NC_INT, memtype);
    }
    else if (!strcmp(var->hdr.name, "atom_site_auth_seq_id"))
    {
        if (var->ndims != 1)
            return NC_EINVAL;
        return mmcif_read_1d_var(mmcif_file->auth_seq_id, start[0], count[0],
                                 value, NC_INT, memtype);
    }
    else if (!strcmp(var->hdr.name, "atom_site_occupancy"))
    {
        if (var->ndims != 1)
            return NC_EINVAL;
        return mmcif_read_1d_var(mmcif_file->occupancy, start[0], count[0],
                                 value, NC_DOUBLE, memtype);
    }
    else if (!strcmp(var->hdr.name, "atom_site_B_iso_or_equiv"))
    {
        if (var->ndims != 1)
            return NC_EINVAL;
        return mmcif_read_1d_var(mmcif_file->b_iso_or_equiv, start[0], count[0],
                                 value, NC_DOUBLE, memtype);
    }
    else if (!strcmp(var->hdr.name, "atom_site_label_atom_id"))
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return mmcif_read_string_var(mmcif_file->label_atom_id, MMCIF_ATOM_ID_LEN + 1,
                                     start, count, value);
    }
    else if (!strcmp(var->hdr.name, "atom_site_label_comp_id"))
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return mmcif_read_string_var(mmcif_file->label_comp_id, MMCIF_COMP_ID_LEN + 1,
                                     start, count, value);
    }
    else if (!strcmp(var->hdr.name, "atom_site_auth_asym_id"))
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return mmcif_read_string_var(mmcif_file->auth_asym_id, MMCIF_ASYM_ID_LEN + 1,
                                     start, count, value);
    }
    else if (!strcmp(var->hdr.name, "atom_site_type_symbol"))
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return mmcif_read_string_var(mmcif_file->type_symbol, MMCIF_TYPE_SYMBOL_LEN + 1,
                                     start, count, value);
    }
    else if (!strcmp(var->hdr.name, "atom_site_group_PDB"))
    {
        if (var->ndims != 2)
            return NC_EINVAL;
        return mmcif_read_string_var(mmcif_file->group_pdb, MMCIF_GROUP_LEN + 1,
                                     start, count, value);
    }

    return NC_EINVAL;
}
