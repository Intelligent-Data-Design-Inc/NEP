/**
 * @file
 * @brief Public types and prototypes for the legacy PDB UDF dispatch layer.
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _PDBDISPATCH_H
#define _PDBDISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/** Legacy PDB format uses UDF7 slot for dispatch table model field (see
 * nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF7
#define NC_FORMATX_NC_PDB NC_FORMATX_UDF7
#else
#define NC_FORMATX_NC_PDB NC_FORMATX_UDF0
#endif

/** Per-file legacy PDB state.
 *
 * V3.3.0 Sprint 2: the parsed in-memory representation of a legacy PDB
 * file. Coordinate and per-atom data are stored in flat arrays sized
 * nmodels * natoms. String attributes (title, compnd, source, etc.) are
 * owned by this struct and freed on close.
 */
typedef struct NC_PDB_FILE_INFO
{
    char *path;               /**< Path to the open PDB file */
    size_t natoms;            /**< Number of ATOM/HETATM records per model */
    int nmodels;              /**< Number of MODEL blocks, or 1 if none */

    float *x;                 /**< atom_site_Cartn_x [model][atom] */
    float *y;                 /**< atom_site_Cartn_y [model][atom] */
    float *z;                 /**< atom_site_Cartn_z [model][atom] */

    int *serial;              /**< atom_site_id [atom] */
    char *name;               /**< atom_site_label_atom_id [atom][5] */
    char *res_name;           /**< atom_site_label_comp_id [atom][4] */
    char *chain_id;           /**< atom_site_auth_asym_id [atom][2] */
    int *res_seq;             /**< atom_site_auth_seq_id [atom] */
    float *occupancy;         /**< atom_site_occupancy [atom] */
    float *temp_factor;       /**< atom_site_B_iso_or_equiv [atom] */
    char *element;            /**< atom_site_type_symbol [atom][3] */
    char *group;              /**< atom_site_group_PDB [atom][7] */

    char *id_code;            /**< HEADER idCode */
    char *classification;     /**< HEADER classification */
    char *dep_date;           /**< HEADER depDate */
    char *title;              /**< Concatenated TITLE records */
    char *compnd;             /**< Concatenated COMPND records */
    char *source;             /**< Concatenated SOURCE records */

    int has_cryst1;           /**< Non-zero if a CRYST1 record was parsed */
    char *cell_a;             /**< CRYST1 unit cell length a */
    char *cell_b;             /**< CRYST1 unit cell length b */
    char *cell_c;             /**< CRYST1 unit cell length c */
    char *cell_alpha;         /**< CRYST1 unit cell angle alpha */
    char *cell_beta;          /**< CRYST1 unit cell angle beta */
    char *cell_gamma;         /**< CRYST1 unit cell angle gamma */
    char *space_group;        /**< CRYST1 space group */
    char *symmetry_z;         /**< CRYST1 Z value */
} NC_PDB_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_PDB_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                void *parameters, const NC_Dispatch *, int);

    extern int
    NC_PDB_abort(int ncid);

    extern int
    NC_PDB_close(int ncid, void *ignore);

    extern int
    NC_PDB_inq_format(int ncid, int *formatp);

    extern int
    NC_PDB_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_PDB_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                    void *value, nc_type);

    extern NC_Dispatch*
    NC_PDB_initialize(void);

    extern int
    NC_PDB_finalize(void);

    extern const NC_Dispatch *PDB_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* _PDBDISPATCH_H */
