/**
 * @file
 * @brief Public types and prototypes for the PDBx/mmCIF UDF dispatch layer.
 *
 * @author Edward Hartnett
 * @date 2026-08-01
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _MMCIFDISPATCH_H
#define _MMCIFDISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/** PDBx/mmCIF format uses UDF8 slot for dispatch table model field (see
 * nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF8
#define NC_FORMATX_NC_MMCIF NC_FORMATX_UDF8
#else
#define NC_FORMATX_NC_MMCIF NC_FORMATX_UDF0
#endif

/** Per-file PDBx/mmCIF state.
 *
 * V3.4.0 Sprint 2: the parsed in-memory representation of an mmCIF file.
 * Coordinate and per-atom data (from the `_atom_site` loop) are stored in
 * flat arrays sized nmodels * natoms. String attributes (from single-row
 * categories like `_entry`, `_struct`, `_cell`, `_symmetry`,
 * `_pdbx_database_status`) are owned by this struct and freed on close.
 */
typedef struct NC_MMCIF_FILE_INFO
{
    char *path;               /**< Path to the open mmCIF file */
    size_t natoms;            /**< Number of _atom_site rows per model */
    int nmodels;              /**< Number of distinct pdbx_PDB_model_num values, or 1 */

    double *x;                 /**< atom_site_Cartn_x [model][atom] */
    double *y;                 /**< atom_site_Cartn_y [model][atom] */
    double *z;                 /**< atom_site_Cartn_z [model][atom] */

    int *id;                   /**< atom_site_id [atom] */
    char *label_atom_id;        /**< atom_site_label_atom_id [atom][MMCIF_ATOM_ID_LEN+1] */
    char *label_comp_id;        /**< atom_site_label_comp_id [atom][MMCIF_COMP_ID_LEN+1] */
    char *auth_asym_id;         /**< atom_site_auth_asym_id [atom][MMCIF_ASYM_ID_LEN+1] */
    int *auth_seq_id;           /**< atom_site_auth_seq_id [atom] */
    double *occupancy;          /**< atom_site_occupancy [atom] */
    double *b_iso_or_equiv;     /**< atom_site_B_iso_or_equiv [atom] */
    char *type_symbol;          /**< atom_site_type_symbol [atom][MMCIF_TYPE_SYMBOL_LEN+1] */
    char *group_pdb;            /**< atom_site_group_PDB [atom][MMCIF_GROUP_LEN+1] */

    char *entry_id;             /**< _entry.id */
    char *struct_title;         /**< _struct.title */
    char *dep_date;             /**< _pdbx_database_status.recvd_initial_deposition_date */

    int has_cell;               /**< Non-zero if a _cell category was parsed */
    char *cell_length_a;        /**< _cell.length_a */
    char *cell_length_b;        /**< _cell.length_b */
    char *cell_length_c;        /**< _cell.length_c */
    char *cell_angle_alpha;     /**< _cell.angle_alpha */
    char *cell_angle_beta;      /**< _cell.angle_beta */
    char *cell_angle_gamma;     /**< _cell.angle_gamma */

    int has_symmetry;           /**< Non-zero if a _symmetry category was parsed */
    char *space_group_name;     /**< _symmetry.space_group_name_H-M */
    char *int_tables_number;    /**< _symmetry.Int_Tables_number */
} NC_MMCIF_FILE_INFO_T;

/** Max length of the atom_site_label_atom_id string field. */
#define MMCIF_ATOM_ID_LEN 8

/** Max length of the atom_site_label_comp_id string field. */
#define MMCIF_COMP_ID_LEN 8

/** Max length of the atom_site_auth_asym_id string field. */
#define MMCIF_ASYM_ID_LEN 4

/** Max length of the atom_site_type_symbol string field. */
#define MMCIF_TYPE_SYMBOL_LEN 4

/** Max length of the atom_site_group_PDB string field. */
#define MMCIF_GROUP_LEN 6

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_MMCIF_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                  void *parameters, const NC_Dispatch *, int);

    extern int
    NC_MMCIF_abort(int ncid);

    extern int
    NC_MMCIF_close(int ncid, void *ignore);

    extern int
    NC_MMCIF_inq_format(int ncid, int *formatp);

    extern int
    NC_MMCIF_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_MMCIF_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                      void *value, nc_type);

    extern NC_Dispatch*
    NC_MMCIF_initialize(void);

    extern int
    NC_MMCIF_finalize(void);

    extern const NC_Dispatch *MMCIF_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* _MMCIFDISPATCH_H */
