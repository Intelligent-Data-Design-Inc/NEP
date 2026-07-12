/**
 * @file
 * @brief Public types and prototypes for the PDS4 UDF dispatch layer.
 *
 * Defines the per-variable layout struct (NC_PDS4_VAR_INFO_T), the
 * per-file state struct (NC_PDS4_FILE_INFO_T), and the public API
 * prototypes for the PDS4 User-Defined Format handler.
 *
 * @author Edward Hartnett
 * @date 2026-07-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _PDS4DISPATCH_H
#define _PDS4DISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/* Include libxml2 header if available */
#ifdef HAVE_PDS4
#include <libxml/parser.h>
#include <libxml/tree.h>
#endif

/** PDS4 format uses UDF5 slot for dispatch table model field (see nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF5
#define NC_FORMATX_NC_PDS4 NC_FORMATX_UDF5
#else
#define NC_FORMATX_NC_PDS4 NC_FORMATX_UDF0
#endif

/** PDS4 magic number length (5 bytes: "<?xml") */
#define PDS4_MAGIC_LEN 5

/** PDS4 XML namespace URI */
#define PDS4_NAMESPACE "http://pds.nasa.gov/pds4/pds/v1"

/** Per-variable PDS4 layout info (stored in var->format_var_info) */
typedef struct NC_PDS4_VAR_INFO
{
    size_t data_offset;           /**< Byte offset of Array or Table in data file */
    size_t record_length;         /**< Record length in bytes (tables only) */
    size_t field_offset;          /**< Field byte offset within record (0-based, tables only) */
    size_t field_length;          /**< Field byte length (tables only) */
    int is_table_field;           /**< 1 if this is a table field, 0 if array */
    int is_ascii;                 /**< 1 if ASCII text field (needs parsing), 0 if binary */
    int is_delimited;             /**< 1 if Table_Delimited (comma-separated); field_number used instead of field_offset */
    int field_number;             /**< 1-based column index for Table_Delimited fields */
    int is_group_field;           /**< 1 if this field is inside a Group_Field_Binary */
    size_t group_location;        /**< Byte offset of group start within record (0-based) */
    size_t group_length;          /**< Byte size of one repetition block */
    size_t repetitions;           /**< Number of group repetitions */
    size_t inner_field_offset;    /**< Byte offset of field within one repetition (0-based) */
    /* Depth-2 nested Group_Field_Binary support (group_depth == 2). */
    int group_depth;              /**< Nesting depth: 1 = flat group, 2 = group-in-group */
    size_t outer_group_location;  /**< Byte offset of outer group start within record (0-based) */
    size_t outer_group_length;    /**< Byte size of one outer repetition block */
    size_t outer_repetitions;     /**< Number of outer repetitions */
    size_t inner_group_location;  /**< Byte offset of inner group start within outer rep (0-based) */
    size_t inner_group_length;    /**< Byte size of one inner repetition block */
    size_t inner_repetitions;     /**< Number of inner repetitions */
    size_t inner2_field_offset;   /**< Byte offset of field within one inner rep (0-based) */
} NC_PDS4_VAR_INFO_T;

/** Per-file PDS4 state */
typedef struct NC_PDS4_FILE_INFO
{
#ifdef HAVE_PDS4
    xmlDoc *doc;                  /**< Parsed XML document */
#else
    void *doc;                    /**< Placeholder when libxml2 not available */
#endif
    char *path;                   /**< Path to the open PDS4 label file */
} NC_PDS4_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_PDS4_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                 void *parameters, const NC_Dispatch *, int);

    extern int
    NC_PDS4_abort(int ncid);

    extern int
    NC_PDS4_close(int ncid, void *ignore);

    extern int
    NC_PDS4_inq_format(int ncid, int *formatp);

    extern int
    NC_PDS4_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_PDS4_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                     void *value, nc_type);

    extern int
    NC_PDS4_initialize(void);

    extern int
    NC_PDS4_finalize(void);

    extern const NC_Dispatch *PDS4_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* _PDS4DISPATCH_H */
