/**
 * @file pds4file.c
 * @brief PDS4 User-Defined Format (UDF) dispatch layer.
 *
 * Implements the NEP PDS4 reader. The PDS4 XML label is parsed with libxml2
 * and mapped into the netCDF-4 in-memory model (groups, dimensions, variables,
 * attributes). Data reading via nc_get_vara() reads from the referenced binary
 * or ASCII data files with automatic byte-order conversion.
 *
 * Supported data objects:
 * - Array / Array_2D_Image (contiguous, row-major)
 * - Table_Binary (fixed-record, binary fields)
 * - Table_Character (fixed-record, ASCII fields)
 * - Table_Delimited (variable-record, ASCII fields)
 *
 * @section pds4_model netCDF Model Mapping
 *
 * A PDS4 label file contains one or more `File_Area_Observational` elements,
 * each referencing a separate binary or ASCII data file. The NEP PDS4 reader
 * maps the label structure into the netCDF-4 in-memory model as follows:
 *
 * - The XML label root → root group. Elements of `Identification_Area` and
 *   `Observation_Area` become global string attributes on the root group.
 * - Each `File_Area_Observational` → a child group of the root group, named
 *   from the `File/file_name` text content.
 * - `Array` / `Array_2D_Image` inside a file area → netCDF dimensions created
 *   from the `Axis_Array` children (sorted ascending by `sequence_number`);
 *   each `axis_name` becomes the dimension name and `elements` its length;
 *   `Last Index Fastest` maps to C-order (fastest-varying index is rightmost).
 *   The array becomes a single netCDF variable in the child group, with type
 *   derived from `Element_Array/data_type`.
 * - `Table_Binary`, `Table_Character`, `Table_Delimited` → a `record`
 *   dimension whose length comes from `<records>`; one netCDF variable per
 *   `Field_Binary`/`Field_Character`/`Field_Delimited` child, named from
 *   `<name>`, typed from `<data_type>`, with an optional `units` attribute
 *   from `<unit>`.
 *
 * @section pds4_byteorder Byte-Order Handling
 *
 * PDS4 specifies byte order per data object: MSB (Most Significant Byte
 * first, big-endian) or LSB (Least Significant Byte first, little-endian).
 * NEP records the byte order as `NC_ENDIAN_BIG` or `NC_ENDIAN_LITTLE` on
 * each netCDF variable at open time. During data reading in
 * NC_PDS4_get_vara(), elements are byte-swapped in-place when the file
 * byte order differs from the host byte order. ASCII numeric fields
 * (Table_Character, Table_Delimited) do not require byte-swapping; they are
 * parsed directly via `strtod()` or `strtoll()`.
 *
 * @section pds4_datapath Data File Resolution
 *
 * Binary and text data files are referenced by filename only in the PDS4
 * label (the `File/file_name` element). NEP resolves each data filename
 * relative to the directory that contains the XML label file, using
 * pds4_resolve_data_path(). If the label has no directory component (i.e.
 * it is in the current directory), the data file is also looked up in the
 * current directory.
 *
 * @author Edward Hartnett
 * @date 2026-07-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "nep_nc4.h"
#include "pds4dispatch.h"

/* Include libxml2 headers if available */
#ifdef HAVE_PDS4
#include <libxml/parser.h>
#include <libxml/tree.h>
#endif

/** PDS4 XML namespace URI */
#define PDS4_NS "http://pds.nasa.gov/pds4/pds/v1"

#ifdef HAVE_PDS4

/**
 * @internal Trim leading and trailing whitespace from a string.
 *
 * @param s Input string (may be empty).
 *
 * @return Pointer to newly allocated trimmed string, or NULL on failure.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static char *
pds4_trim(const char *s)
{
    const char *start;
    char *end;
    size_t len;
    char *res;

    if (!s)
        return strdup("");

    start = s;
    while (isspace((unsigned char)*start))
        start++;

    if (*start == '\0')
        return strdup("");

    end = (char *)start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
        end--;

    len = (size_t)(end - start + 1);
    res = malloc(len + 1);
    if (!res)
        return NULL;
    memcpy(res, start, len);
    res[len] = '\0';
    return res;
}

/**
 * @internal Get trimmed text content of an XML node.
 *
 * @param node XML node.
 *
 * @return Newly allocated trimmed text, or NULL if the node has no text.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static char *
pds4_get_text(xmlNode *node)
{
    xmlChar *content;
    char *trimmed;

    if (!node)
        return NULL;

    content = xmlNodeGetContent(node);
    if (!content)
        return NULL;

    trimmed = pds4_trim((const char *)content);
    xmlFree(content);
    return trimmed;
}

/**
 * @internal Find a direct child element by local name.
 *
 * @param parent Parent XML element.
 * @param name Local name to find.
 *
 * @return Pointer to the child element, or NULL if not found.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static xmlNode *
pds4_find_child(xmlNode *parent, const char *name)
{
    xmlNode *cur;

    for (cur = parent->children; cur; cur = cur->next)
    {
        if (cur->type == XML_ELEMENT_NODE && cur->ns &&
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) == 0 &&
            xmlStrcmp(cur->name, (const xmlChar *)name) == 0)
            return cur;
    }
    return NULL;
}

/**
 * @internal Check whether an attribute with the given name already exists.
 *
 * @param list Attribute list (group or variable).
 * @param name Attribute name.
 *
 * @return 1 if it exists, 0 otherwise.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_att_exists(NCindex *list, const char *name)
{
    size_t i;
    size_t n;

    n = ncindexsize(list);
    for (i = 0; i < n; i++)
    {
        NC_ATT_INFO_T *att = (NC_ATT_INFO_T *)ncindexith(list, i);
        if (att && att->hdr.name && strcmp(att->hdr.name, name) == 0)
            return 1;
    }
    return 0;
}

/**
 * @internal Add a string attribute to a group or variable.
 *
 * @param list Attribute list.
 * @param name Attribute name.
 * @param value Attribute string value.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_add_att(NCindex *list, const char *name, const char *value)
{
    NC_ATT_INFO_T *att;
    size_t len;
    char *data;
    int retval;

    if ((retval = nc4_att_list_add(list, name, &att)))
        return retval;

    len = strlen(value);
    att->nc_typeid = NC_CHAR;
    att->len = len;
    if (len > 0)
    {
        data = malloc(len + 1);
        if (!data)
            return NC_ENOMEM;
        memcpy(data, value, len + 1);
        att->data = data;
    }
    att->dirty = NC_TRUE;

    return NC_NOERR;
}

/**
 * @internal Map a PDS4 data_type string to a netCDF type.
 *
 * @param pds4_type PDS4 data_type value.
 * @param xtypep Pointer that gets the netCDF type.
 * @param type_sizep Pointer that gets the element size in bytes.
 * @param endiannessp Pointer that gets the endianness.
 * @param type_name Buffer (at least NC_MAX_NAME+1 bytes) that gets the type name.
 *
 * @return NC_NOERR on success.
 * @return NC_EBADTYPE if the type is unknown.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_type_to_nc_type(const char *pds4_type, nc_type *xtypep, size_t *type_sizep,
                     int *endiannessp, char *type_name)
{
    nc_type xtype = NC_NAT;
    size_t type_size = 0;
    int endianness = NC_ENDIAN_NATIVE;
    const char *name = NULL;

    if (strcmp(pds4_type, "IEEE754MSBSingle") == 0)
    { xtype = NC_FLOAT; type_size = 4; endianness = NC_ENDIAN_BIG; name = "float"; }
    else if (strcmp(pds4_type, "IEEE754LSBSingle") == 0)
    { xtype = NC_FLOAT; type_size = 4; endianness = NC_ENDIAN_LITTLE; name = "float"; }
    else if (strcmp(pds4_type, "IEEE754MSBDouble") == 0)
    { xtype = NC_DOUBLE; type_size = 8; endianness = NC_ENDIAN_BIG; name = "double"; }
    else if (strcmp(pds4_type, "IEEE754LSBDouble") == 0)
    { xtype = NC_DOUBLE; type_size = 8; endianness = NC_ENDIAN_LITTLE; name = "double"; }
    else if (strcmp(pds4_type, "UnsignedByte") == 0)
    { xtype = NC_UBYTE; type_size = 1; endianness = NC_ENDIAN_NATIVE; name = "ubyte"; }
    else if (strcmp(pds4_type, "SignedByte") == 0)
    { xtype = NC_BYTE; type_size = 1; endianness = NC_ENDIAN_NATIVE; name = "byte"; }
    else if (strcmp(pds4_type, "UnsignedMSB2") == 0)
    { xtype = NC_USHORT; type_size = 2; endianness = NC_ENDIAN_BIG; name = "ushort"; }
    else if (strcmp(pds4_type, "UnsignedLSB2") == 0)
    { xtype = NC_USHORT; type_size = 2; endianness = NC_ENDIAN_LITTLE; name = "ushort"; }
    else if (strcmp(pds4_type, "SignedMSB2") == 0)
    { xtype = NC_SHORT; type_size = 2; endianness = NC_ENDIAN_BIG; name = "short"; }
    else if (strcmp(pds4_type, "SignedLSB2") == 0)
    { xtype = NC_SHORT; type_size = 2; endianness = NC_ENDIAN_LITTLE; name = "short"; }
    else if (strcmp(pds4_type, "UnsignedMSB4") == 0)
    { xtype = NC_UINT; type_size = 4; endianness = NC_ENDIAN_BIG; name = "uint"; }
    else if (strcmp(pds4_type, "UnsignedLSB4") == 0)
    { xtype = NC_UINT; type_size = 4; endianness = NC_ENDIAN_LITTLE; name = "uint"; }
    else if (strcmp(pds4_type, "SignedMSB4") == 0)
    { xtype = NC_INT; type_size = 4; endianness = NC_ENDIAN_BIG; name = "int"; }
    else if (strcmp(pds4_type, "SignedLSB4") == 0)
    { xtype = NC_INT; type_size = 4; endianness = NC_ENDIAN_LITTLE; name = "int"; }
    else if (strcmp(pds4_type, "UnsignedMSB8") == 0)
    { xtype = NC_UINT64; type_size = 8; endianness = NC_ENDIAN_BIG; name = "uint64"; }
    else if (strcmp(pds4_type, "UnsignedLSB8") == 0)
    { xtype = NC_UINT64; type_size = 8; endianness = NC_ENDIAN_LITTLE; name = "uint64"; }
    else if (strcmp(pds4_type, "SignedMSB8") == 0)
    { xtype = NC_INT64; type_size = 8; endianness = NC_ENDIAN_BIG; name = "int64"; }
    else if (strcmp(pds4_type, "SignedLSB8") == 0)
    { xtype = NC_INT64; type_size = 8; endianness = NC_ENDIAN_LITTLE; name = "int64"; }
    /* ASCII table field types. */
    else if (strcmp(pds4_type, "ASCII_Real") == 0)
    { xtype = NC_DOUBLE; type_size = 8; endianness = NC_ENDIAN_NATIVE; name = "double"; }
    else if (strcmp(pds4_type, "ASCII_Integer") == 0)
    { xtype = NC_INT64; type_size = 8; endianness = NC_ENDIAN_NATIVE; name = "int64"; }
    else if (strcmp(pds4_type, "ASCII_NonNegative_Integer") == 0)
    { xtype = NC_UINT64; type_size = 8; endianness = NC_ENDIAN_NATIVE; name = "uint64"; }
    else if (strcmp(pds4_type, "ASCII_Boolean") == 0)
    { xtype = NC_UBYTE; type_size = 1; endianness = NC_ENDIAN_NATIVE; name = "ubyte"; }
    else if (strcmp(pds4_type, "ASCII_String") == 0 ||
             strcmp(pds4_type, "ASCII_Date") == 0 ||
             strcmp(pds4_type, "ASCII_Date_Time_YMD") == 0 ||
             strcmp(pds4_type, "ASCII_Date_Time_YMD_UTC") == 0 ||
             strcmp(pds4_type, "ASCII_Date_Time_DOY") == 0 ||
             strcmp(pds4_type, "ASCII_Date_Time_DOY_UTC") == 0 ||
             strcmp(pds4_type, "UTF8_String") == 0)
    { xtype = NC_CHAR; type_size = 1; endianness = NC_ENDIAN_NATIVE; name = "char"; }
    else
    {
        return NC_EBADTYPE;
    }

    if (xtypep)
        *xtypep = xtype;
    if (type_sizep)
        *type_sizep = type_size;
    if (endiannessp)
        *endiannessp = endianness;
    if (type_name)
        strncpy(type_name, name, NC_MAX_NAME);

    return NC_NOERR;
}

/**
 * @internal Set the type of a netCDF-4 variable.
 *
 * @param xtype NetCDF type.
 * @param endianness Byte order.
 * @param type_size Element size in bytes.
 * @param type_name Type name.
 * @param typep Pointer that gets the TYPE_INFO_T struct.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_set_var_type(nc_type xtype, int endianness, size_t type_size,
                  const char *type_name, NC_TYPE_INFO_T **typep)
{
    NC_TYPE_INFO_T *type;
    int retval;

    if ((retval = nc4_type_new(type_size, type_name, xtype, &type)))
        return retval;

    type->endianness = endianness;
    type->size = type_size;
    type->nc_type_class = xtype;

    *typep = type;
    return NC_NOERR;
}

/**
 * @internal Add all simple text-valued direct children of an area as
 * attributes on a group.
 *
 * Used for `Identification_Area`.
 *
 * @param grp Group to receive attributes.
 * @param area XML area element.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_add_area_atts(NC_GRP_INFO_T *grp, xmlNode *area)
{
    xmlNode *cur;
    int retval;

    for (cur = area->children; cur; cur = cur->next)
    {
        if (cur->type == XML_ELEMENT_NODE && cur->ns &&
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) == 0)
        {
            char *text = pds4_get_text(cur);
            if (text && *text)
            {
                retval = pds4_add_att(grp->att, (const char *)cur->name, text);
                free(text);
                if (retval)
                    return retval;
            }
            else
            {
                free(text);
            }
        }
    }
    return NC_NOERR;
}

/**
 * @internal Recursively add leaf text-valued descendants of an area as
 * attributes on a group.
 *
 * Used for `Observation_Area`. Duplicate names are skipped so the mapping
 * stays deterministic.
 *
 * @param grp Group to receive attributes.
 * @param node XML node to examine.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_add_leaf_atts(NC_GRP_INFO_T *grp, xmlNode *node)
{
    xmlNode *cur;
    int has_element_children = 0;
    int retval;

    for (cur = node->children; cur; cur = cur->next)
    {
        if (cur->type == XML_ELEMENT_NODE && cur->ns &&
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) == 0)
        {
            has_element_children = 1;
            break;
        }
    }

    if (!has_element_children)
    {
        char *text = pds4_get_text(node);
        if (text && *text)
        {
            if (!pds4_att_exists(grp->att, (const char *)node->name))
                retval = pds4_add_att(grp->att, (const char *)node->name, text);
            else
                retval = NC_NOERR;
            free(text);
            if (retval)
                return retval;
        }
        else
        {
            free(text);
        }
        return NC_NOERR;
    }

    for (cur = node->children; cur; cur = cur->next)
    {
        if (cur->type == XML_ELEMENT_NODE && cur->ns &&
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) == 0)
        {
            if ((retval = pds4_add_leaf_atts(grp, cur)))
                return retval;
        }
    }

    return NC_NOERR;
}

/**
 * @internal Axis information collected from an `Axis_Array` element.
 */
struct pds4_axis
{
    int seq;
    size_t len;
    char name[NC_MAX_NAME + 1];
};

/**
 * @internal qsort comparison for axes by sequence_number.
 */
static int
pds4_axis_cmp(const void *a, const void *b)
{
    const struct pds4_axis *aa = (const struct pds4_axis *)a;
    const struct pds4_axis *bb = (const struct pds4_axis *)b;
    return aa->seq - bb->seq;
}

/**
 * @internal Read an `Array` or `Array_2D_Image` element and create the
 * corresponding netCDF dimensions and variable inside a file-area group.
 *
 * @param grp File-area group that will contain the variable.
 * @param array XML `Array` or `Array_2D_Image` element.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_read_array(NC_GRP_INFO_T *grp, xmlNode *array)
{
    xmlNode *cur;
    xmlNode *element_array;
    xmlNode *data_type_node;
    xmlNode *name_node;
    xmlNode *unit_node;
    xmlNode *offset_node;
    char *data_type_str = NULL;
    char *var_name = NULL;
    char *unit_str = NULL;
    char *offset_str = NULL;
    char type_name[NC_MAX_NAME + 1];
    nc_type xtype;
    size_t type_size;
    size_t data_offset = 0;
    int endianness;
    NC_VAR_INFO_T *var;
    NC_TYPE_INFO_T *type_info;
    NC_PDS4_VAR_INFO_T *var_info = NULL;
    struct pds4_axis *axes = NULL;
    int *dimids = NULL;
    int naxes = 0;
    int d;
    int retval = NC_NOERR;

    /* Count axes. */
    for (cur = array->children; cur; cur = cur->next)
        if (cur->type == XML_ELEMENT_NODE && cur->ns &&
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) == 0 &&
            xmlStrcmp(cur->name, (const xmlChar *)"Axis_Array") == 0)
            naxes++;

    if (naxes <= 0)
    {
        retval = NC_EINVAL;
        goto cleanup;
    }

    axes = calloc(naxes, sizeof(struct pds4_axis));
    dimids = calloc(naxes, sizeof(int));
    if (!axes || !dimids)
    {
        retval = NC_ENOMEM;
        goto cleanup;
    }

    element_array = pds4_find_child(array, "Element_Array");
    if (!element_array)
    {
        retval = NC_EINVAL;
        goto cleanup;
    }

    data_type_node = pds4_find_child(element_array, "data_type");
    if (!data_type_node)
    {
        retval = NC_EINVAL;
        goto cleanup;
    }

    data_type_str = pds4_get_text(data_type_node);
    if (!data_type_str)
    {
        retval = NC_EINVAL;
        goto cleanup;
    }

    retval = pds4_type_to_nc_type(data_type_str, &xtype, &type_size,
                                  &endianness, type_name);
    if (retval)
        goto cleanup;

    /* Variable name: use label name if present, otherwise "data". */
    name_node = pds4_find_child(array, "name");
    if (name_node)
        var_name = pds4_get_text(name_node);
    if (!var_name)
        var_name = strdup("data");
    if (!var_name)
    {
        retval = NC_ENOMEM;
        goto cleanup;
    }

    /* Collect axis information. */
    d = 0;
    for (cur = array->children; cur; cur = cur->next)
    {
        xmlNode *seq_node;
        xmlNode *axis_name_node;
        xmlNode *elements_node;
        char *seq_str;
        char *axis_name_str;
        char *elements_str;

        if (cur->type != XML_ELEMENT_NODE || !cur->ns ||
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) != 0 ||
            xmlStrcmp(cur->name, (const xmlChar *)"Axis_Array") != 0)
            continue;

        seq_node = pds4_find_child(cur, "sequence_number");
        axis_name_node = pds4_find_child(cur, "axis_name");
        elements_node = pds4_find_child(cur, "elements");
        if (!seq_node || !axis_name_node || !elements_node)
        {
            retval = NC_EINVAL;
            goto cleanup;
        }

        seq_str = pds4_get_text(seq_node);
        axis_name_str = pds4_get_text(axis_name_node);
        elements_str = pds4_get_text(elements_node);
        if (!seq_str || !axis_name_str || !elements_str)
        {
            free(seq_str);
            free(axis_name_str);
            free(elements_str);
            retval = NC_EINVAL;
            goto cleanup;
        }

        axes[d].seq = atoi(seq_str);
        axes[d].len = (size_t)atol(elements_str);
        strncpy(axes[d].name, axis_name_str, NC_MAX_NAME);
        axes[d].name[NC_MAX_NAME] = '\0';

        free(seq_str);
        free(axis_name_str);
        free(elements_str);
        d++;
    }

    qsort(axes, naxes, sizeof(struct pds4_axis), pds4_axis_cmp);

    /* Create dimensions in sequence order. */
    for (d = 0; d < naxes; d++)
    {
        NC_DIM_INFO_T *dim;
        if ((retval = nc4_dim_list_add(grp, axes[d].name, axes[d].len,
                                       -1, &dim)))
            goto cleanup;
        dimids[d] = dim->hdr.id;
    }

    /* Create the variable. */
    if ((retval = nc4_var_list_add(grp, var_name, naxes, &var)))
        goto cleanup;
    if ((retval = nc4_var_set_ndims(var, naxes)))
        goto cleanup;
    if ((retval = pds4_set_var_type(xtype, endianness, type_size, type_name,
                                    &type_info)))
        goto cleanup;

    var->type_info = type_info;
    var->type_info->rc++;
    var->endianness = endianness;
    var->created = NC_TRUE;
    var->written_to = NC_TRUE;
    var->atts_read = 1;
    var->storage = NC_CONTIGUOUS;

    for (d = 0; d < naxes; d++)
        var->dimids[d] = dimids[d];

    /* Parse optional <offset> element (defaults to 0). */
    offset_node = pds4_find_child(array, "offset");
    if (offset_node)
    {
        offset_str = pds4_get_text(offset_node);
        if (offset_str)
        {
            data_offset = (size_t)atol(offset_str);
            free(offset_str);
        }
    }

    /* Store per-variable layout info. */
    var_info = calloc(1, sizeof(NC_PDS4_VAR_INFO_T));
    if (!var_info)
    {
        retval = NC_ENOMEM;
        goto cleanup;
    }
    var_info->data_offset = data_offset;
    var_info->is_table_field = 0;
    var_info->is_ascii = 0;
    var->format_var_info = var_info;

    /* Optional units attribute. */
    unit_node = pds4_find_child(element_array, "unit");
    if (!unit_node)
        unit_node = pds4_find_child(array, "unit");
    if (unit_node)
    {
        unit_str = pds4_get_text(unit_node);
        if (unit_str && *unit_str)
            retval = pds4_add_att(var->att, "units", unit_str);
        free(unit_str);
        if (retval)
            goto cleanup;
    }

cleanup:
    free(data_type_str);
    free(var_name);
    free(axes);
    free(dimids);
    return retval;
}

/**
 * @internal Read a `Table_Binary`, `Table_Character`, or `Table_Delimited`
 * element and create the corresponding netCDF dimension and variables
 * inside the file-area group.
 *
 * Creates a `record` dimension of length `records` and one variable per
 * field. Each variable has dimension `[record]` (or `[record, field_length]`
 * for NC_CHAR string fields) and an optional `units` attribute.
 *
 * @param grp File-area group that will contain the dimension and variables.
 * @param table XML `Table_Binary`, `Table_Character`, or `Table_Delimited` element.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_read_table(NC_GRP_INFO_T *grp, xmlNode *table)
{
    xmlNode *records_node;
    xmlNode *record_node;
    xmlNode *name_node;
    xmlNode *offset_node;
    xmlNode *record_length_node;
    xmlNode *cur;
    char *records_str = NULL;
    char *table_name = NULL;
    char *offset_str = NULL;
    char *record_length_str = NULL;
    size_t nrecords;
    size_t table_offset = 0;
    size_t record_length = 0;
    int is_ascii_table;
    NC_DIM_INFO_T *record_dim = NULL;
    int record_dimid;
    int retval = NC_NOERR;
    const char *record_child_name;
    const char *field_child_name;

    /* Determine which Record_* and Field_* elements to look for. */
    if (xmlStrcmp(table->name, (const xmlChar *)"Table_Binary") == 0)
    {
        record_child_name = "Record_Binary";
        field_child_name = "Field_Binary";
        is_ascii_table = 0;
    }
    else if (xmlStrcmp(table->name, (const xmlChar *)"Table_Character") == 0)
    {
        record_child_name = "Record_Character";
        field_child_name = "Field_Character";
        is_ascii_table = 1;
    }
    else if (xmlStrcmp(table->name, (const xmlChar *)"Table_Delimited") == 0)
    {
        record_child_name = "Record_Delimited";
        field_child_name = "Field_Delimited";
        is_ascii_table = 1;
    }
    else
    {
        return NC_EINVAL;
    }

    /* Get the number of records. */
    records_node = pds4_find_child(table, "records");
    if (!records_node)
        return NC_EINVAL;

    records_str = pds4_get_text(records_node);
    if (!records_str || !*records_str)
    {
        free(records_str);
        return NC_EINVAL;
    }
    nrecords = (size_t)atol(records_str);
    free(records_str);

    /* Parse optional <offset> (defaults to 0). */
    offset_node = pds4_find_child(table, "offset");
    if (offset_node)
    {
        offset_str = pds4_get_text(offset_node);
        if (offset_str)
        {
            table_offset = (size_t)atol(offset_str);
            free(offset_str);
        }
    }

    /* Table name: use <name> if present, otherwise "table". */
    name_node = pds4_find_child(table, "name");
    if (name_node)
        table_name = pds4_get_text(name_node);
    if (!table_name || !*table_name)
    {
        free(table_name);
        table_name = strdup("table");
    }
    if (!table_name)
        return NC_ENOMEM;

    /* Store table name as a group attribute. */
    if ((retval = pds4_add_att(grp->att, "table_name", table_name)))
    {
        free(table_name);
        return retval;
    }
    free(table_name);

    /* Create the record dimension. */
    if ((retval = nc4_dim_list_add(grp, "record", nrecords, -1, &record_dim)))
        return retval;
    record_dimid = record_dim->hdr.id;

    /* Find the Record_* child. */
    record_node = pds4_find_child(table, record_child_name);
    if (!record_node)
        return NC_EINVAL;

    /* Parse record_length. */
    record_length_node = pds4_find_child(record_node, "record_length");
    if (record_length_node)
    {
        record_length_str = pds4_get_text(record_length_node);
        if (record_length_str)
        {
            record_length = (size_t)atol(record_length_str);
            free(record_length_str);
        }
    }

    /* Iterate over Field_* children inside the Record_* element. */
    for (cur = record_node->children; cur; cur = cur->next)
    {
        xmlNode *field_name_node;
        xmlNode *field_type_node;
        xmlNode *field_unit_node;
        xmlNode *field_length_node;
        xmlNode *field_location_node;
        char *field_name = NULL;
        char *field_type_str = NULL;
        char *field_unit = NULL;
        char *field_length_str = NULL;
        char *field_location_str = NULL;
        char type_name[NC_MAX_NAME + 1];
        nc_type xtype;
        size_t type_size;
        size_t field_length = 0;
        size_t field_location = 0;
        int endianness;
        NC_VAR_INFO_T *var;
        NC_TYPE_INFO_T *type_info;
        NC_PDS4_VAR_INFO_T *var_info;

        if (cur->type != XML_ELEMENT_NODE || !cur->ns ||
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) != 0 ||
            xmlStrcmp(cur->name, (const xmlChar *)field_child_name) != 0)
            continue;

        /* Get field name. */
        field_name_node = pds4_find_child(cur, "name");
        if (!field_name_node)
            continue;
        field_name = pds4_get_text(field_name_node);
        if (!field_name || !*field_name)
        {
            free(field_name);
            continue;
        }

        /* Sanitize field name: replace spaces with underscores. */
        {
            char *p;
            for (p = field_name; *p; p++)
                if (*p == ' ')
                    *p = '_';
        }

        /* Get field data_type. */
        field_type_node = pds4_find_child(cur, "data_type");
        if (!field_type_node)
        {
            free(field_name);
            continue;
        }
        field_type_str = pds4_get_text(field_type_node);
        if (!field_type_str)
        {
            free(field_name);
            continue;
        }

        /* Map PDS4 type to netCDF type. */
        retval = pds4_type_to_nc_type(field_type_str, &xtype, &type_size,
                                      &endianness, type_name);
        if (retval)
        {
            free(field_name);
            free(field_type_str);
            continue; /* Skip unsupported types. */
        }

        /* Parse field_location (1-based in PDS4). */
        field_location_node = pds4_find_child(cur, "field_location");
        if (field_location_node)
        {
            field_location_str = pds4_get_text(field_location_node);
            if (field_location_str)
            {
                field_location = (size_t)atol(field_location_str);
                free(field_location_str);
            }
        }

        /* Parse field_length. */
        field_length_node = pds4_find_child(cur, "field_length");
        if (!field_length_node)
            field_length_node = pds4_find_child(cur, "maximum_field_length");
        if (field_length_node)
        {
            field_length_str = pds4_get_text(field_length_node);
            if (field_length_str)
            {
                field_length = (size_t)atol(field_length_str);
                free(field_length_str);
            }
        }
        if (field_length == 0)
            field_length = type_size;

        /* For NC_CHAR fields, create a 2D variable [record, strlen]. */
        if (xtype == NC_CHAR)
        {
            NC_DIM_INFO_T *strlen_dim;
            int dimids[2];
            char strlen_dimname[NC_MAX_NAME + 1];

            /* Create a per-field string length dimension. */
            snprintf(strlen_dimname, NC_MAX_NAME, "%s_strlen", field_name);
            if ((retval = nc4_dim_list_add(grp, strlen_dimname, field_length,
                                           -1, &strlen_dim)))
            {
                free(field_name);
                free(field_type_str);
                return retval;
            }

            dimids[0] = record_dimid;
            dimids[1] = strlen_dim->hdr.id;

            if ((retval = nc4_var_list_add(grp, field_name, 2, &var)))
            {
                free(field_name);
                free(field_type_str);
                return retval;
            }
            if ((retval = nc4_var_set_ndims(var, 2)))
            {
                free(field_name);
                free(field_type_str);
                return retval;
            }
            var->dimids[0] = dimids[0];
            var->dimids[1] = dimids[1];
        }
        else
        {
            /* Scalar numeric field: 1D variable [record]. */
            if ((retval = nc4_var_list_add(grp, field_name, 1, &var)))
            {
                free(field_name);
                free(field_type_str);
                return retval;
            }
            if ((retval = nc4_var_set_ndims(var, 1)))
            {
                free(field_name);
                free(field_type_str);
                return retval;
            }
            var->dimids[0] = record_dimid;
        }

        /* Set variable type. */
        if ((retval = pds4_set_var_type(xtype, endianness, type_size,
                                        type_name, &type_info)))
        {
            free(field_name);
            free(field_type_str);
            return retval;
        }
        var->type_info = type_info;
        var->type_info->rc++;
        var->endianness = endianness;
        var->created = NC_TRUE;
        var->written_to = NC_TRUE;
        var->atts_read = 1;
        var->storage = NC_CONTIGUOUS;

        /* Store per-variable layout info for data reading. */
        var_info = calloc(1, sizeof(NC_PDS4_VAR_INFO_T));
        if (!var_info)
        {
            free(field_name);
            free(field_type_str);
            return NC_ENOMEM;
        }
        var_info->data_offset = table_offset;
        var_info->record_length = record_length;
        /* PDS4 field_location is 1-based; convert to 0-based. */
        var_info->field_offset = (field_location > 0) ? field_location - 1 : 0;
        var_info->field_length = field_length;
        var_info->is_table_field = 1;
        var_info->is_ascii = is_ascii_table;
        var->format_var_info = var_info;

        /* Optional units attribute. */
        field_unit_node = pds4_find_child(cur, "unit");
        if (field_unit_node)
        {
            field_unit = pds4_get_text(field_unit_node);
            if (field_unit && *field_unit)
            {
                retval = pds4_add_att(var->att, "units", field_unit);
                free(field_unit);
                if (retval)
                {
                    free(field_name);
                    free(field_type_str);
                    return retval;
                }
            }
            else
            {
                free(field_unit);
            }
        }

        free(field_name);
        free(field_type_str);
    }

    return NC_NOERR;
}

/**
 * @internal Read a `File_Area_Observational` element and create a child
 * group for it, then read each array or table inside the group.
 *
 * @param h5 Pointer to the file info struct.
 * @param root_grp Root group.
 * @param file_area XML `File_Area_Observational` element.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_read_file_area(NC_FILE_INFO_T *h5, NC_GRP_INFO_T *root_grp,
                    xmlNode *file_area)
{
    xmlNode *file_node;
    xmlNode *file_name_node;
    xmlNode *cur;
    char *file_name = NULL;
    char grp_name[NC_MAX_NAME + 1];
    NC_GRP_INFO_T *file_grp = NULL;
    int retval;

    file_node = pds4_find_child(file_area, "File");
    if (!file_node)
        return NC_EINVAL;

    file_name_node = pds4_find_child(file_node, "file_name");
    if (!file_name_node)
        return NC_EINVAL;

    file_name = pds4_get_text(file_name_node);
    if (!file_name || !*file_name)
    {
        free(file_name);
        return NC_EINVAL;
    }

    strncpy(grp_name, file_name, NC_MAX_NAME);
    grp_name[NC_MAX_NAME] = '\0';

    if ((retval = nc4_grp_list_add(h5, root_grp, grp_name, &file_grp)))
    {
        free(file_name);
        return retval;
    }
    file_grp->atts_read = 1;

    for (cur = file_area->children; cur; cur = cur->next)
    {
        if (cur->type != XML_ELEMENT_NODE || !cur->ns ||
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) != 0)
            continue;

        if (xmlStrcmp(cur->name, (const xmlChar *)"File") == 0)
            continue;

        if (xmlStrcmp(cur->name, (const xmlChar *)"Array_2D_Image") == 0 ||
            xmlStrcmp(cur->name, (const xmlChar *)"Array") == 0)
        {
            if ((retval = pds4_read_array(file_grp, cur)))
            {
                free(file_name);
                return retval;
            }
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"Table_Binary") == 0 ||
                 xmlStrcmp(cur->name, (const xmlChar *)"Table_Character") == 0 ||
                 xmlStrcmp(cur->name, (const xmlChar *)"Table_Delimited") == 0)
        {
            if ((retval = pds4_read_table(file_grp, cur)))
            {
                free(file_name);
                return retval;
            }
        }
    }

    free(file_name);
    return NC_NOERR;
}

/**
 * @internal Read a parsed PDS4 XML label and build the netCDF-4 metadata
 * model for it.
 *
 * @param h5 Pointer to the file info struct.
 * @param pds4_file PDS4-specific file info containing the parsed XML document.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_read_label(NC_FILE_INFO_T *h5, NC_PDS4_FILE_INFO_T *pds4_file)
{
    xmlNode *root;
    xmlNode *cur;
    int retval;

    root = xmlDocGetRootElement(pds4_file->doc);
    if (!root)
        return NC_EINVAL;

    h5->root_grp->atts_read = 1;

    for (cur = root->children; cur; cur = cur->next)
    {
        if (cur->type != XML_ELEMENT_NODE || !cur->ns ||
            xmlStrcmp(cur->ns->href, (const xmlChar *)PDS4_NS) != 0)
            continue;

        if (xmlStrcmp(cur->name, (const xmlChar *)"Identification_Area") == 0)
        {
            if ((retval = pds4_add_area_atts(h5->root_grp, cur)))
                return retval;
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"Observation_Area") == 0)
        {
            if ((retval = pds4_add_leaf_atts(h5->root_grp, cur)))
                return retval;
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"File_Area_Observational") == 0)
        {
            if ((retval = pds4_read_file_area(h5, h5->root_grp, cur)))
                return retval;
        }
    }

    return NC_NOERR;
}

#endif /* HAVE_PDS4 */

/**
 * Open a PDS4 XML label file.
 *
 * Parses the PDS4 XML label with libxml2, validates the root element
 * namespace, and maps the label content into the netCDF-4 in-memory
 * model (root-group attributes, child groups, dimensions, variables).
 * Only read-only access (NC_NOWRITE) is supported.
 *
 * @param path Path to the PDS4 XML label file.
 * @param mode Open mode flags; NC_WRITE causes NC_EPERM to be returned.
 * @param basepe Ignored (present for dispatch interface compatibility).
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Dispatch table pointer assigned by NetCDF-C.
 * @param ncid NetCDF ID assigned by the NetCDF-C dispatch layer.
 *
 * @return NC_NOERR No error.
 * @return NC_EINVAL Null path, XML parse failure, or non-PDS4 namespace.
 * @return NC_EPERM Write mode requested.
 * @return NC_ENOTNC Root element namespace is not the PDS4 namespace.
 * @return NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
int
NC_PDS4_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
             void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC *nc;
    NC_FILE_INFO_T *h5;
    NC_PDS4_FILE_INFO_T *pds4_file;
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

    /* Allocate PDS4-specific file info. */
    if (!(pds4_file = calloc(1, sizeof(NC_PDS4_FILE_INFO_T))))
        return NC_ENOMEM;

    if (!(pds4_file->path = strdup(path)))
    {
        free(pds4_file);
        return NC_ENOMEM;
    }

#ifdef HAVE_PDS4
    {
        xmlDoc *doc;
        xmlNode *root;
        const xmlChar *ns_href;

        /* Parse the XML label. XML_PARSE_NONET prevents network schema fetches. */
        doc = xmlReadFile(path, NULL, XML_PARSE_NONET);
        if (!doc)
        {
            free(pds4_file->path);
            free(pds4_file);
            return NC_EINVAL;
        }

        /* Confirm root element namespace is PDS4. */
        root = xmlDocGetRootElement(doc);
        if (!root)
        {
            xmlFreeDoc(doc);
            free(pds4_file->path);
            free(pds4_file);
            return NC_EINVAL;
        }

        ns_href = (root->ns && root->ns->href) ? root->ns->href : NULL;
        if (!ns_href || xmlStrcmp(ns_href, (const xmlChar *)PDS4_NS) != 0)
        {
            xmlFreeDoc(doc);
            free(pds4_file->path);
            free(pds4_file);
            return NC_ENOTNC;
        }

        /* Read label metadata into the netCDF-4 model. */
        pds4_file->doc = doc;
        if ((retval = pds4_read_label(h5, pds4_file)))
        {
            xmlFreeDoc(doc);
            pds4_file->doc = NULL;
            free(pds4_file->path);
            free(pds4_file);
            return retval;
        }
    }
#else
    pds4_file->doc = NULL;
#endif

    h5->format_file_info = pds4_file;
    return NC_NOERR;
}

/**
 * @internal Free format_var_info for all variables in a group and its
 * subgroups.
 */
static void
pds4_free_var_info(NC_GRP_INFO_T *grp)
{
    size_t i;

    /* Free format_var_info on each variable. */
    for (i = 0; i < ncindexsize(grp->vars); i++)
    {
        NC_VAR_INFO_T *var = (NC_VAR_INFO_T *)ncindexith(grp->vars, i);
        if (var && var->format_var_info)
        {
            free(var->format_var_info);
            var->format_var_info = NULL;
        }
    }

    /* Recurse into child groups. */
    for (i = 0; i < ncindexsize(grp->children); i++)
    {
        NC_GRP_INFO_T *child = (NC_GRP_INFO_T *)ncindexith(grp->children, i);
        if (child)
            pds4_free_var_info(child);
    }
}

/**
 * Close a PDS4 file.
 *
 * Frees per-variable layout info, the parsed XML document, and the
 * per-file PDS4 state struct.
 *
 * @param ncid NetCDF ID of the open PDS4 file.
 * @param ignore Ignored (NULL).
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
int
NC_PDS4_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_PDS4_FILE_INFO_T *pds4_file;
    int retval;

    assert(ignore || !ignore);

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Free format_var_info on all variables. */
    pds4_free_var_info(h5->root_grp);

    /* Get PDS4-specific info. */
    pds4_file = (NC_PDS4_FILE_INFO_T *)h5->format_file_info;
    if (!pds4_file)
        return NC_NOERR;

#ifdef HAVE_PDS4
    /* Free the parsed XML document. */
    if (pds4_file->doc)
    {
        xmlFreeDoc(pds4_file->doc);
        pds4_file->doc = NULL;
    }
#endif

    /* Free PDS4-specific info. */
    free(pds4_file->path);
    free(pds4_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * Abort opening a PDS4 file.
 *
 * Releases all resources allocated during a partial open, including the
 * parsed XML document and per-file PDS4 state.
 *
 * @param ncid NetCDF ID of the partially opened file.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
int
NC_PDS4_abort(int ncid)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_PDS4_FILE_INFO_T *pds4_file;
    int retval;

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get PDS4-specific info. */
    pds4_file = (NC_PDS4_FILE_INFO_T *)h5->format_file_info;
    if (!pds4_file)
        return NC_NOERR;

#ifdef HAVE_PDS4
    if (pds4_file->doc)
    {
        xmlFreeDoc(pds4_file->doc);
        pds4_file->doc = NULL;
    }
#endif

    free(pds4_file->path);
    free(pds4_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * Inquire the format of a PDS4 file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that receives the format code (NC_FORMAT_NETCDF4).
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
int
NC_PDS4_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * Inquire the extended format of a PDS4 file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that receives the extended format code
 *        (NC_FORMATX_NC_PDS4 = NC_FORMATX_UDF5).
 * @param modep Pointer that receives the mode flags (NC_NOWRITE).
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
int
NC_PDS4_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_PDS4;
    if (modep)
        *modep = NC_NOWRITE;
    return NC_NOERR;
}

/**
 * @internal Resolve the data file path relative to the XML label directory.
 *
 * @param label_path Path to the PDS4 XML label.
 * @param data_filename Data file name from <File/file_name>.
 * @param resolved Buffer (at least PATH_MAX bytes) to receive result.
 *
 * @return NC_NOERR on success.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_resolve_data_path(const char *label_path, const char *data_filename,
                       char *resolved)
{
    const char *last_slash;
    size_t dir_len;

    last_slash = strrchr(label_path, '/');
    if (last_slash)
    {
        dir_len = (size_t)(last_slash - label_path + 1);
        if (dir_len + strlen(data_filename) >= 4096)
            return NC_EINVAL;
        memcpy(resolved, label_path, dir_len);
        strcpy(resolved + dir_len, data_filename);
    }
    else
    {
        /* No directory separator: data file is in current directory. */
        strncpy(resolved, data_filename, 4095);
        resolved[4095] = '\0';
    }
    return NC_NOERR;
}

/**
 * @internal Swap bytes in-place for an array of elements.
 *
 * @param buf Buffer containing elements.
 * @param elem_size Size of each element in bytes.
 * @param nelems Number of elements.
 *
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static void
pds4_byte_swap(void *buf, size_t elem_size, size_t nelems)
{
    unsigned char *p = (unsigned char *)buf;
    size_t i, j;

    for (i = 0; i < nelems; i++)
    {
        unsigned char *elem = p + i * elem_size;
        for (j = 0; j < elem_size / 2; j++)
        {
            unsigned char tmp = elem[j];
            elem[j] = elem[elem_size - 1 - j];
            elem[elem_size - 1 - j] = tmp;
        }
    }
}

/**
 * @internal Determine if byte swapping is needed.
 *
 * @param endianness Variable endianness (NC_ENDIAN_BIG or NC_ENDIAN_LITTLE).
 *
 * @return 1 if swapping needed, 0 otherwise.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
static int
pds4_needs_swap(int endianness)
{
    static const int one = 1;
    int host_is_little = (*(const char *)&one == 1);

    if (endianness == NC_ENDIAN_BIG && host_is_little)
        return 1;
    if (endianness == NC_ENDIAN_LITTLE && !host_is_little)
        return 1;
    return 0;
}

/**
 * Read a hyperslab of data from a PDS4 variable.
 *
 * Reads contiguous array data or fixed-record table field data from the
 * binary or ASCII data file referenced by the PDS4 label. Array reads
 * honor the `start`/`count` hyperslab in C (row-major) order. Table
 * field reads iterate over records `start[0]` through
 * `start[0]+count[0]-1`. Byte-order conversion is applied automatically
 * for binary types; ASCII fields are parsed via `strtod()`/`strtoll()`.
 *
 * @param ncid NetCDF ID of the group containing the variable.
 * @param varid Variable ID within the group.
 * @param start Array of start indices (0-based) for each dimension.
 * @param count Number of elements to read along each dimension.
 * @param value Output buffer large enough to hold the requested elements.
 * @param memtype Requested memory type (type conversion not yet implemented;
 *        caller must request the native file type).
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @return NC_ENOTVAR No variable with varid in this group.
 * @return NC_EINVAL Invalid parameters or seek error.
 * @return NC_ENOMEM Out of memory.
 * @return NC_ENOTFOUND Data file not found.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
int
NC_PDS4_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                 void *value, nc_type memtype)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NC_PDS4_FILE_INFO_T *pds4_file;
    NC_PDS4_VAR_INFO_T *var_info;
    char data_path[4096];
    FILE *fp = NULL;
    int retval = NC_NOERR;

    (void)memtype; /* Type conversion deferred. */

    /* Get file and group info. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get the PDS4 file info for the label path. */
    pds4_file = (NC_PDS4_FILE_INFO_T *)h5->format_file_info;
    if (!pds4_file)
        return NC_EINVAL;

    /* Find the variable. */
    var = (NC_VAR_INFO_T *)ncindexith(grp->vars, varid);
    if (!var)
        return NC_ENOTVAR;

    /* Get the layout info. */
    var_info = (NC_PDS4_VAR_INFO_T *)var->format_var_info;
    if (!var_info)
        return NC_EINVAL;

    /* Resolve the data file path. The group name IS the data filename. */
    if ((retval = pds4_resolve_data_path(pds4_file->path, grp->hdr.name,
                                         data_path)))
        return retval;

    /* Open the data file. */
    fp = fopen(data_path, "rb");
    if (!fp)
        return NC_ENOTFOUND;

    if (!var_info->is_table_field)
    {
        /* --- Array data reading --- */
        size_t elem_size = var->type_info->size;
        size_t total_elems = 1;
        size_t d;
        int need_swap;
        size_t *dim_lens = NULL;

        /* Compute total elements requested and collect dim lengths. */
        dim_lens = calloc(var->ndims, sizeof(size_t));
        if (!dim_lens)
        {
            fclose(fp);
            return NC_ENOMEM;
        }
        for (d = 0; d < var->ndims; d++)
        {
            NC_DIM_INFO_T *dim = (NC_DIM_INFO_T *)ncindexith(grp->dim, var->dimids[d]);
            dim_lens[d] = dim->len;
            total_elems *= count[d];
        }

        /* For 1D or when last dim count matches last dim length, we can
         * do a simpler read. But the general approach for any dimensions
         * is to iterate over the "inner rows" of the last dimension. */
        {
            /* Compute the number of inner-row reads and the file stride. */
            size_t inner_count = count[var->ndims - 1];
            size_t nrows = total_elems / inner_count;
            unsigned char *out = (unsigned char *)value;
            size_t row;

            for (row = 0; row < nrows; row++)
            {
                /* Convert the linear row index back to multi-dim indices
                 * for dimensions 0..ndims-2. */
                size_t row_tmp = row;
                size_t file_linear = 0;

                /* Compute file linear element index for this row. */
                for (d = 0; d < var->ndims - 1; d++)
                {
                    size_t row_count_product = 1;
                    size_t dd;
                    for (dd = d + 1; dd < var->ndims - 1; dd++)
                        row_count_product *= count[dd];

                    size_t idx_in_count = row_tmp / row_count_product;
                    row_tmp %= row_count_product;

                    /* The actual index in the file for this dimension. */
                    size_t file_idx = start[d] + idx_in_count;

                    /* Accumulate into linear offset. */
                    size_t file_stride = 1;
                    for (dd = d + 1; dd < var->ndims; dd++)
                        file_stride *= dim_lens[dd];
                    file_linear += file_idx * file_stride;
                }
                /* Add the start offset for the last dimension. */
                file_linear += start[var->ndims - 1];

                size_t byte_offset = var_info->data_offset + file_linear * elem_size;

                if (fseek(fp, (long)byte_offset, SEEK_SET) != 0)
                {
                    free(dim_lens);
                    fclose(fp);
                    return NC_EINVAL;
                }
                if (fread(out + row * inner_count * elem_size, elem_size,
                          inner_count, fp) != inner_count)
                {
                    free(dim_lens);
                    fclose(fp);
                    return NC_EINVAL;
                }
            }
        }

        free(dim_lens);

        /* Byte-swap if needed. */
        need_swap = pds4_needs_swap(var->endianness);
        if (need_swap && elem_size > 1)
            pds4_byte_swap(value, elem_size, total_elems);
    }
    else
    {
        /* --- Table field data reading --- */
        size_t nrecords = count[0];
        size_t start_record = start[0];
        size_t r;
        size_t elem_size = var->type_info->size;
        int need_swap;

        if (var_info->is_ascii)
        {
            /* ASCII table: read text and parse. */
            char *field_buf = malloc(var_info->field_length + 1);
            if (!field_buf)
            {
                fclose(fp);
                return NC_ENOMEM;
            }

            for (r = 0; r < nrecords; r++)
            {
                size_t seek_pos = var_info->data_offset +
                    (start_record + r) * var_info->record_length +
                    var_info->field_offset;

                if (fseek(fp, (long)seek_pos, SEEK_SET) != 0)
                {
                    free(field_buf);
                    fclose(fp);
                    return NC_EINVAL;
                }
                if (fread(field_buf, 1, var_info->field_length, fp) != var_info->field_length)
                {
                    free(field_buf);
                    fclose(fp);
                    return NC_EINVAL;
                }
                field_buf[var_info->field_length] = '\0';

                /* Parse based on the netCDF type. */
                switch (var->type_info->nc_type_class)
                {
                case NC_DOUBLE:
                    ((double *)value)[r] = strtod(field_buf, NULL);
                    break;
                case NC_FLOAT:
                    ((float *)value)[r] = (float)strtod(field_buf, NULL);
                    break;
                case NC_INT64:
                    ((long long *)value)[r] = strtoll(field_buf, NULL, 10);
                    break;
                case NC_UINT64:
                    ((unsigned long long *)value)[r] = strtoull(field_buf, NULL, 10);
                    break;
                case NC_INT:
                    ((int *)value)[r] = (int)strtol(field_buf, NULL, 10);
                    break;
                case NC_CHAR:
                    /* Copy raw text into output. */
                    memcpy((char *)value + r * var_info->field_length,
                           field_buf, var_info->field_length);
                    break;
                default:
                    ((double *)value)[r] = strtod(field_buf, NULL);
                    break;
                }
            }
            free(field_buf);
        }
        else
        {
            /* Binary table: read raw bytes and byte-swap. */
            for (r = 0; r < nrecords; r++)
            {
                size_t seek_pos = var_info->data_offset +
                    (start_record + r) * var_info->record_length +
                    var_info->field_offset;

                if (fseek(fp, (long)seek_pos, SEEK_SET) != 0)
                {
                    fclose(fp);
                    return NC_EINVAL;
                }
                if (fread((unsigned char *)value + r * elem_size, 1, elem_size, fp) != elem_size)
                {
                    fclose(fp);
                    return NC_EINVAL;
                }
            }

            /* Byte-swap if needed. */
            need_swap = pds4_needs_swap(var->endianness);
            if (need_swap && elem_size > 1)
                pds4_byte_swap(value, elem_size, nrecords);
        }
    }

    fclose(fp);
    return NC_NOERR;
}
