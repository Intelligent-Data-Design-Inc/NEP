/**
 * @file pds4file.c
 * @brief PDS4 User-Defined Format (UDF) dispatch layer.
 *
 * Implements the NEP PDS4 reader. Sprint 4 reads the PDS4 XML label and
 * maps array product metadata into the netCDF-4 in-memory model. Data reading
 * is deferred to a later sprint.
 *
 * PDS4 label files are XML documents that reference external data files.
 * This layer uses libxml2 to parse the XML label, validates the PDS4
 * namespace, and stores the document pointer for future sprints.
 *
 * @author Edward Hartnett
 * @date 2026-07-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
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
 * @return ::NC_NOERR on success.
 * @author Edward Hartnett
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
 * @return ::NC_NOERR on success.
 * @return ::NC_EBADTYPE if the type is unknown.
 * @author Edward Hartnett
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
 * @return ::NC_NOERR on success.
 * @author Edward Hartnett
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
 * @return ::NC_NOERR on success.
 * @author Edward Hartnett
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
 * @return ::NC_NOERR on success.
 * @author Edward Hartnett
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
 * @return ::NC_NOERR on success.
 * @author Edward Hartnett
 */
static int
pds4_read_array(NC_GRP_INFO_T *grp, xmlNode *array)
{
    xmlNode *cur;
    xmlNode *element_array;
    xmlNode *data_type_node;
    xmlNode *name_node;
    xmlNode *unit_node;
    char *data_type_str = NULL;
    char *var_name = NULL;
    char *unit_str = NULL;
    char type_name[NC_MAX_NAME + 1];
    nc_type xtype;
    size_t type_size;
    int endianness;
    NC_VAR_INFO_T *var;
    NC_TYPE_INFO_T *type_info;
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
 * @internal Read a `File_Area_Observational` element and create a child
 * group for it, then read each array inside the group.
 *
 * @param h5 Pointer to the file info struct.
 * @param root_grp Root group.
 * @param file_area XML `File_Area_Observational` element.
 *
 * @return ::NC_NOERR on success.
 * @author Edward Hartnett
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
 * @return ::NC_NOERR on success.
 * @author Edward Hartnett
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
 * @internal Open a PDS4 XML label file.
 *
 * Validates that the file begins with the XML declaration, parses the
 * XML label with libxml2, confirms the root element belongs to the PDS4
 * namespace, and stores the document pointer in per-file state.
 *
 * In Sprint 4 the parsed label is also converted into netCDF-4 metadata
 * (groups, dimensions, variables, attributes). Data reading is still disabled.
 *
 * @param path Path to the PDS4 XML label file.
 * @param mode Open mode flags.
 * @param basepe Ignored.
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Dispatch table pointer.
 * @param ncid NetCDF ID assigned by the dispatch layer.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EINVAL Invalid parameters or not a PDS4 label.
 * @return ::NC_EPERM Write mode requested.
 * @return ::NC_ENOMEM Out of memory.
 * @author Edward Hartnett
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
 * @internal Close a PDS4 file.
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Edward Hartnett
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
 * @internal Abort opening a PDS4 file.
 *
 * @param ncid NetCDF ID.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
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
 * @internal Inquire the format of a PDS4 file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
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
 * @internal Inquire the extended format of a PDS4 file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 * @param modep Pointer that gets mode flags.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
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
 * @internal Read a hyperslab of data from a PDS4 variable.
 *
 * Data reading is not implemented in Sprint 4. Returns NC_EINVAL.
 *
 * @param ncid NetCDF ID.
 * @param varid Variable ID.
 * @param start Start indices.
 * @param count Counts.
 * @param value Output buffer.
 * @param memtype Memory type.
 *
 * @return ::NC_EINVAL Data reading not yet implemented.
 * @author Edward Hartnett
 */
int
NC_PDS4_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                 void *value, nc_type memtype)
{
    (void)ncid;
    (void)varid;
    (void)start;
    (void)count;
    (void)value;
    (void)memtype;
    return NC_EINVAL;
}
