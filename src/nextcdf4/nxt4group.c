/**
 * @file nxt4group.c
 * @brief NEXTCDF-4 group creation and inquiry.
 *
 * @author Edward Hartnett
 * @date 2026-08-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "nclist.h"
#include "nxt4internal.h"

/*
 * @return The NEXTCDF-4 group info, creating a default one if absent.
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

/*
 * @return The group info attached to a freshly-added or existing group.
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

/*
 * Recursively find a group by its in-file id.
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

/*
 * Resolve a group from a NetCDF ncid, creating root group info on demand.
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
    if ((ret = NEXTCDF4_check_write_define(file)))
        return ret;
    if ((ret = NC_check_name(name)))
        return ret;
    if ((ret = get_grp_from_ncid(ncid, &h5, &parent)))
        return ret;

    if (file->mode & NC_CLASSIC_MODEL)
        return NC_ENOTNC4;
    if (file->netcdf4_model)
        return NC_ENOTNC4;

    if ((ret = get_grp_hdf(parent, &parent_g)))
        return ret;

    child_g = H5Gcreate2(parent_g, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
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

/*
 * Locate a direct child group by name.
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
        strncpy(name, path, NC_MAX_NAME);
    return NC_NOERR;
}

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

int
NEXTCDF4_inq_grp_full_ncid(int ncid, const char *full_name, int *grpidp)
{
    return NEXTCDF4_inq_ncid(ncid, full_name, grpidp);
}
