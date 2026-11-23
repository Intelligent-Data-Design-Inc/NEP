/**
 * @file nxt4group.c
 * @brief NEXTCDF-4 group creation and inquiry.

 * @author Edward Hartnett
 * @date 2026-08-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "nclist.h"
#include "nxt4internal.h"

/**
 * @brief Return the open HDF5 group handle for an in-memory group.
 *
 * Every `NC_GRP_INFO_T` carries a `NEXTCDF4_GRP_INFO_T` holding the
 * `hid_t` of the corresponding HDF5 group. If that private record was
 * never allocated (for example for the root group right after
 * create), this function allocates it on demand: non-root groups get
 * a placeholder handle that is filled in when the HDF5 group is
 * created; the root group uses the file's `rootid`.
 *
 * @param grp Group.
 * @param gid Destination for the HDF5 group identifier.
 * @return `NC_NOERR` on success or `NC_ENOMEM`.
 */
static int
get_grp_hdf(NC_GRP_INFO_T *grp, hid_t *gid)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    NEXTCDF4_FILE_INFO_T *file = grp->nc4_info->format_file_info;

    if (!ginfo) {
        if (!(ginfo = calloc(1, sizeof(*ginfo))))
            return NC_ENOMEM;
        if (grp->parent)
            ginfo->hdf_group = -1; /* must be filled on creation */
        else
            ginfo->hdf_group = file->rootid;
        grp->format_grp_info = ginfo;
    }
    if (gid)
        *gid = ginfo->hdf_group;
    return NC_NOERR;
}

/**
 * @brief Attach an HDF5 group handle to a freshly-added group.
 *
 * Companion to get_grp_hdf(): used when the `NC_GRP_INFO_T` already
 * exists (created by `nc4_grp_list_add`) and only the private
 * `NEXTCDF4_GRP_INFO_T` record with its `hid_t` needs to be hung off
 * it. A no-op if the record is already attached.
 *
 * @param grp Group.
 * @param hdf_group HDF5 group identifier to attach.
 * @return `NC_NOERR` on success or `NC_ENOMEM`.
 */
static int
attach_grp_hdf(NC_GRP_INFO_T *grp, hid_t hdf_group)
{
    NEXTCDF4_GRP_INFO_T *ginfo;
    if (grp->format_grp_info)
        return NC_NOERR;
    if (!(ginfo = calloc(1, sizeof(*ginfo))))
        return NC_ENOMEM;
    ginfo->hdf_group = hdf_group;
    grp->format_grp_info = ginfo;
    return NC_NOERR;
}

/**
 * @brief Recursively find a group in the tree by its in-file group id.
 *
 * In netcdf-c an ncid is two halves: the upper 16 bits identify the
 * file, the lower 16 bits the group inside that file. This walks the
 * in-memory group tree looking for a group whose `hdr.id` matches the
 * low half — the HDF5 object ids are not used for this lookup.
 *
 * @param root Group tree root.
 * @param grpid In-file group id (low half of an ncid).
 * @param grpp Destination for the matching group.
 * @return `NC_NOERR` on success or `NC_EBADID`.
 */
static int
find_grp_by_id(NC_GRP_INFO_T *root, int grpid, NC_GRP_INFO_T **grpp)
{
    size_t i;
    if (!root)
        return NC_EBADID;
    if ((int)root->hdr.id == grpid) {
        *grpp = root;
        return NC_NOERR;
    }
    for (i = 0; i < ncindexsize(root->children); i++) {
        NC_GRP_INFO_T *g = (NC_GRP_INFO_T *)ncindexith(root->children, i);
        int ret;
        if (!g)
            continue;
        if ((ret = find_grp_by_id(g, grpid, grpp)) == NC_NOERR)
            return NC_NOERR;
    }
    return NC_EBADID;
}

/**
 * @brief Resolve the `NC_GRP_INFO_T` an ncid refers to.
 *
 * Splits the ncid into file id (upper 16 bits) and in-file group id
 * (lower 16 bits), locates the file's root group via netcdf-c's
 * `nc4_find_grp_h5`, then descends the in-memory group tree. Group
 * operations dispatch on an ncid that may name any group in the file,
 * so nearly every public function in this file starts here.
 *
 * @param ncid NetCDF file identifier (file id | group id).
 * @param h5 Destination for the file state, or NULL; if non-NULL and
 * `*h5` is NULL it is filled with the file's `NC_FILE_INFO_T`.
 * @param grpp Destination for the group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
get_grp_from_ncid(int ncid, NC_FILE_INFO_T **h5, NC_GRP_INFO_T **grpp)
{
    int ret;
    NC_GRP_INFO_T *root = NULL;

    if ((ret = nc4_find_grp_h5(ncid & 0xffff0000, &root, h5)))
        return ret;
    if (!root)
        return NC_EBADID;
    if (h5 && !*h5)
        *h5 = root->nc4_info;
    return find_grp_by_id(root, ncid & 0xffff, grpp);
}

/**
 * @brief Create a subgroup — the `nc_def_grp()` dispatch entry.
 *
 * Validates define-mode write access, creates the HDF5 group under
 * the parent's `hid_t`, adds the matching `NC_GRP_INFO_T` to the
 * in-memory tree, and returns the child ncid. Groups are an
 * enhanced-model feature: native and `NC_NETCDF4_MODEL` files accept
 * them, while `NC_CLASSIC_MODEL` files return `NC_ENOTNC4`.
 *
 * @param ncid NetCDF file identifier of the parent group.
 * @param name Subgroup name.
 * @param grpidp Destination for the new subgroup ncid.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_grp(int ncid, const char *name, int *grpidp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *parent;
    NC_GRP_INFO_T *grp = NULL;
    hid_t parent_g, child_g;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(h5, file)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if ((ret = get_grp_from_ncid(ncid, &h5, &parent)))
        return ret;

    if (file->mode & NC_CLASSIC_MODEL)
        return NC_ENOTNC4;

    if ((ret = get_grp_hdf(parent, &parent_g)))
        return ret;

    /* Create the HDF5 group first, then mirror it in the in-memory
     * tree; on metadata failure the HDF5 group is closed again.
     * Link and attribute creation order are tracked so readers
     * (including upstream) iterate committed datatypes and attributes
     * in creation order, as upstream files record. */
    {
        hid_t gcpl = H5Pcreate(H5P_GROUP_CREATE);
        if (gcpl < 0 ||
            H5Pset_link_creation_order(gcpl, H5P_CRT_ORDER_TRACKED |
                                       H5P_CRT_ORDER_INDEXED) < 0 ||
            H5Pset_attr_creation_order(gcpl, H5P_CRT_ORDER_TRACKED |
                                       H5P_CRT_ORDER_INDEXED) < 0) {
            if (gcpl >= 0)
                H5Pclose(gcpl);
            return NC_EHDFERR;
        }
        child_g = H5Gcreate2(parent_g, name, H5P_DEFAULT, gcpl, H5P_DEFAULT);
        H5Pclose(gcpl);
    }
    if (child_g < 0)
        return NC_EHDFERR;

    if ((ret = nc4_grp_list_add(h5, parent, (char *)name, &grp)))
        goto fail;
    if ((ret = attach_grp_hdf(grp, child_g)))
        goto fail;

    if (grpidp)
        *grpidp = (ncid & 0xffff0000) | (int)grp->hdr.id;
    return NC_NOERR;

fail:
    if (grp)
        nc4_rec_grp_del(grp);
    H5Gclose(child_g);
    return ret;
}

/**
 * @brief Locate a direct child group by name.
 * @param parent Parent group.
 * @param name Name.
 * @param grpp Destination for the child group.
 * @return `NC_NOERR` on success or `NC_ENOTNC` when absent.
 */
static int
find_child_grp(NC_GRP_INFO_T *parent, const char *name, NC_GRP_INFO_T **grpp)
{
    size_t i;
    for (i = 0; i < ncindexsize(parent->children); i++) {
        NC_GRP_INFO_T *g = (NC_GRP_INFO_T *)ncindexith(parent->children, i);
        if (g && !strcmp(g->hdr.name, name)) {
            *grpp = g;
            return NC_NOERR;
        }
    }
    return NC_ENOTNC;
}

/**
 * @brief Get the ncid of a named subgroup — the `nc_inq_ncid()`
 * dispatch entry.
 *
 * @p name may be a simple name ("g1"), a relative path ("g1/g2"), or
 * an absolute path ("/g1/g2"); "/" alone resolves to the root group.
 * The lookup walks the in-memory group tree component by component —
 * no HDF5 calls are needed because the metadata was fully loaded at
 * open.
 *
 * @param ncid NetCDF file identifier.
 * @param name Subgroup name or path.
 * @param grpidp Destination for the subgroup ncid.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_ncid(int ncid, const char *name, int *grpidp)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    char path[NC_MAX_NAME * 4 + 1];
    char *tok;
    char *saveptr;
    int ret;

    if ((ret = get_grp_from_ncid(ncid, &h5, &grp)))
        return ret;
    if (!name || !*name)
        return NC_EINVAL;

    if (!strcmp(name, "/")) {
        if (grpidp)
            *grpidp = (int)h5->root_grp->hdr.id;
        return NC_NOERR;
    }

    strncpy(path, name, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    if (path[0] == '/')
        grp = h5->root_grp;

    tok = strtok_r(path, "/", &saveptr);
    while (tok) {
        NC_GRP_INFO_T *child = NULL;
        if ((ret = find_child_grp(grp, tok, &child)))
            return ret;
        grp = child;
        tok = strtok_r(NULL, "/", &saveptr);
    }

    if (grpidp)
        *grpidp = (ncid & 0xffff0000) | (int)grp->hdr.id;
    return NC_NOERR;
}

/**
 * @brief Get the ncids of all subgroups — the `nc_inq_grps()`
 * dispatch entry.
 *
 * @param ncid NetCDF file identifier.
 * @param numgrps Destination for the number of subgroups.
 * @param grpidsp Destination for the subgroup ncids.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_grps(int ncid, int *numgrps, int *grpidsp)
{
    NC_GRP_INFO_T *grp;
    int ret;
    size_t i, n;

    if ((ret = get_grp_from_ncid(ncid, NULL, &grp)))
        return ret;
    n = ncindexsize(grp->children);
    if (numgrps)
        *numgrps = (int)n;
    if (grpidsp) {
        for (i = 0; i < n; i++) {
            NC_GRP_INFO_T *child = (NC_GRP_INFO_T *)ncindexith(grp->children, i);
            if (child)
                grpidsp[i] = (ncid & 0xffff0000) | (int)child->hdr.id;
        }
    }
    return NC_NOERR;
}

/**
 * @brief Get a group's (short) name — the `nc_inq_grpname()`
 * dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param name Destination for the group name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_grpname(int ncid, char *name)
{
    NC_GRP_INFO_T *grp;
    int ret;

    if ((ret = get_grp_from_ncid(ncid, NULL, &grp)))
        return ret;
    if (name)
        strncpy(name, grp->hdr.name, NC_MAX_NAME);
    return NC_NOERR;
}

/**
 * @brief Get a group's full path name — the `nc_inq_grpname_full()`
 * dispatch entry.
 *
 * Rebuilds "/g1/g2/..." by walking the parent chain up to the root;
 * the root group itself contributes nothing but the leading slash.
 * Nesting is capped at 32 levels.
 *
 * @param ncid NetCDF file identifier.
 * @param lenp Destination for the name length.
 * @param name Destination for the full group name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_grpname_full(int ncid, size_t *lenp, char *name)
{
    NC_GRP_INFO_T *grp;
    NC_GRP_INFO_T *path_grps[32];
    int depth = 0;
    size_t pos = 0;
    char path[NC_MAX_NAME * 4 + 1];
    int i;
    int ret;

    if ((ret = get_grp_from_ncid(ncid, NULL, &grp)))
        return ret;

    /* Collect groups from root to this one. */
    while (grp) {
        if (depth >= 32)
            return NC_EINVAL;
        path_grps[depth++] = grp;
        grp = grp->parent;
    }

    pos = 0;
    path[pos++] = '/';
    for (i = depth - 2; i >= 0; i--) {
        size_t n = strlen(path_grps[i]->hdr.name);
        if (pos + n + 1 > sizeof(path))
            return NC_EINVAL;
        memcpy(path + pos, path_grps[i]->hdr.name, n);
        pos += n;
        if (i > 0)
            path[pos++] = '/';
    }
    path[pos] = '\0';

    if (lenp)
        *lenp = pos;
    if (name)
        memcpy(name, path, pos + 1);
    return NC_NOERR;
}

/**
 * @brief Get a group's parent ncid — the `nc_inq_grp_parent()`
 * dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param parentidp Destination for the parent ncid.
 * @return `NC_NOERR` on success, `NC_ENOGRP` for the root group, or
 * another NetCDF error code.
 */
int
NEXTCDF4_inq_grp_parent(int ncid, int *parentidp)
{
    NC_GRP_INFO_T *grp;
    int ret;

    if ((ret = get_grp_from_ncid(ncid, NULL, &grp)))
        return ret;
    if (!grp->parent)
        return NC_ENOGRP;
    if (parentidp)
        *parentidp = (ncid & 0xffff0000) | (int)grp->parent->hdr.id;
    return NC_NOERR;
}

/**
 * @brief Get the ncid of a group given its full path — the
 * `nc_inq_grp_full_ncid()` dispatch entry.
 *
 * `NEXTCDF4_inq_ncid()` already accepts absolute paths, so this is a
 * straight delegation.
 *
 * @param ncid NetCDF file identifier.
 * @param full_name Full group path.
 * @param grpidp Destination for the group ncid.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_grp_full_ncid(int ncid, const char *full_name, int *grpidp)
{
    return NEXTCDF4_inq_ncid(ncid, full_name, grpidp);
}

/**
 * @brief Rename a populated enhanced-model group without changing object IDs.
 * Moves only the parent link, retaining open descendants and their dimension
 * and datatype relationships. Full names are derived from the updated tree.
 * @param grpid Non-root group identifier.
 * @param name New short name, normalized according to netCDF naming rules.
 * @return NC_NOERR, a naming/access error, or NC_EHDFERR on link/flush failure.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
NEXTCDF4_rename_grp(int grpid, const char *name)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NEXTCDF4_GRP_INFO_T *parent;
    char normalized[NC_MAX_NAME + 1];
    char *replacement, *oldname;
    int ret;

    if ((ret = NEXTCDF4_get_file(grpid, &h5, &file)) ||
        (ret = get_grp_from_ncid(grpid, &h5, &grp)))
        return ret;
    if (file->no_write)
        return NC_EPERM;
    if (file->mode & NC_CLASSIC_MODEL)
        return NC_ENOTNC4;
    if (!grp->parent)
        return NC_EBADGRPID;
    if (!name)
        return NC_EINVAL;
    if ((ret = nc4_check_name(name, normalized)) ||
        (ret = nc4_check_dup_name(grp->parent, normalized)))
        return ret;
    if (!(replacement = strdup(normalized)))
        return NC_ENOMEM;
    if ((ret = NEXTCDF4_check_write_define(h5, file))) {
        free(replacement);
        return ret;
    }
    parent = grp->parent->format_grp_info;
    oldname = grp->hdr.name;
    if (H5Lmove(parent->hdf_group, oldname, parent->hdf_group,
                normalized, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        free(replacement);
        return NC_EHDFERR;
    }
    grp->hdr.name = replacement;
    if (!ncindexrebuild(grp->parent->children)) {
        if (H5Lmove(parent->hdf_group, normalized, parent->hdf_group,
                    oldname, H5P_DEFAULT, H5P_DEFAULT) < 0) {
            free(oldname);
            return NC_EHDFERR;
        }
        grp->hdr.name = oldname;
        free(replacement);
        ncindexrebuild(grp->parent->children);
        return NC_ENOMEM;
    }
    free(oldname);
    return H5Fflush(file->hdfid, H5F_SCOPE_LOCAL) < 0 ? NC_EHDFERR : NC_NOERR;
}
