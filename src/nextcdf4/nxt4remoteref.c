/**
 * @file nxt4remoteref.c
 * @brief NEXTCDF-4 remote reference API: resolve stored (URL, object
 * path) pairs against remote datasets.
 *
 * Remote references are a stored convention, not an HDF5 token: the
 * application keeps a URL and an object path as NC_STRING data, and
 * nc_ref_open_remote() resolves one pair by calling `nc_open()` on the
 * URL — plain upstream dispatch, so DAP2, DAP4 and NCZarr
 * file/byte-range URLs all work — then resolving the path by name with
 * the public inquiry API. nc_ref_resolve() exposes the path mapping
 * alone so one remote session can serve many references without
 * paying a remote open per element. See @ref nextcdf4_refs.
 *
 * Compiled unconditionally; when the library is built without
 * `NEP_ENABLE_REMOTE_REFS` both entry points return `NC_ENOTBUILT`.
 *
 * @author Edward Hartnett
 * @date 2026-11-27
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include "nxt4internal.h"

#ifdef HAVE_REMOTE_REFS

/**
 * @brief Resolve the final path component to a variable, group, or
 * committed type, in that precedence order.
 *
 * @param ncid Group the leaf name is looked up in.
 * @param leaf Final path component (non-empty).
 * @param kindp Receives NC_REF_OBJ_VAR, NC_REF_OBJ_GRP or
 * NC_REF_OBJ_TYPE.
 * @param objidp Receives the varid, group ncid or typeid.
 * @return NC_NOERR on success, NC_ENOTVAR when nothing resolves.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
resolve_leaf(int ncid, const char *leaf, int *kindp, int *objidp)
{
    int id;
    nc_type tid;

    if (nc_inq_varid(ncid, leaf, &id) == NC_NOERR) {
        *kindp = NC_REF_OBJ_VAR;
        *objidp = id;
        return NC_NOERR;
    }
    if (nc_inq_grp_ncid(ncid, leaf, &id) == NC_NOERR) {
        *kindp = NC_REF_OBJ_GRP;
        *objidp = id;
        return NC_NOERR;
    }
    if (nc_inq_typeid(ncid, leaf, &tid) == NC_NOERR) {
        id = (int)tid;
        *kindp = NC_REF_OBJ_TYPE;
        *objidp = id;
        return NC_NOERR;
    }
    return NC_ENOTVAR;
}

/**
 * @brief Verify that attribute @p att exists on the resolved owner.
 *
 * Variable and group owners are checked with nc_inq_attid() (global
 * attributes on a group owner). Committed-datatype attributes are not
 * exposed through the NetCDF API, so type owners are accepted without
 * verification and the name is returned to the caller as documented.
 *
 * @param ncid Session the owner was resolved on.
 * @param kind Owner kind from resolve_leaf().
 * @param objid Owner varid, group ncid or typeid.
 * @param att Attribute name.
 * @return NC_NOERR if the attribute exists (or cannot be checked on a
 * type owner), NC_ENOTATT otherwise.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
check_att(int ncid, int kind, int objid, const char *att)
{
    int attid;

    switch (kind) {
    case NC_REF_OBJ_VAR:
        return nc_inq_attid(ncid, objid, att, &attid);
    case NC_REF_OBJ_GRP:
        return nc_inq_attid(objid, NC_GLOBAL, att, &attid);
    case NC_REF_OBJ_TYPE:
        return NC_NOERR;
    }
    return NC_EINVAL;
}

#endif /* HAVE_REMOTE_REFS */

/**
 * @brief Resolve an object path on an already-open remote session.
 *
 * Implementation of the public contract declared in `nep.h`: split
 * the optional `#attname` suffix, walk the group components with
 * nc_inq_grp_ncid(), and resolve the leaf as variable, group or
 * committed type in that order.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_resolve(int ncid, const char *path, int *kindp, int *objidp,
               char *att_name, size_t att_name_size)
{
#ifndef HAVE_REMOTE_REFS
    (void)ncid; (void)path; (void)kindp; (void)objidp;
    (void)att_name; (void)att_name_size;
    return NC_ENOTBUILT;
#else
    char buf[NC_MAX_NAME * 8];
    char *leaf, *hash, *comp;
    int cur, kind, objid, ret;
    size_t len;

    ELOG((2, "%s: ncid %d path %s", __func__, ncid,
          path ? path : "NULL"));

    if (!path || !kindp || !objidp)
        return NC_EINVAL;
    if (path[0] != '/')
        return NC_EINVAL;
    len = strlen(path);
    if (len == 0 || len >= sizeof(buf))
        return NC_EINVAL;
    strcpy(buf, path);

    /* Split off an optional attribute suffix; '#' may not appear in
     * the object path itself. */
    hash = strchr(buf, '#');
    if (hash) {
        *hash = '\0';
        if (hash[1] == '\0' || strchr(hash + 1, '#'))
            return NC_EINVAL;
        if (!att_name || att_name_size == 0)
            return NC_EINVAL;
        if (strlen(hash + 1) >= att_name_size)
            return NC_EINVAL;
    }

    /* Reject empty components up front ("//" or a trailing "/"):
     * otherwise the walk would fail at a group lookup before reaching
     * the malformed component. "/" alone stays legal. */
    {
        const char *end = hash ? hash : buf + len;

        if (end == buf || strstr(buf, "//") ||
            (end - buf > 1 && end[-1] == '/'))
            return NC_EINVAL;
    }

    /* "/" alone resolves to the root group. */
    cur = ncid;
    comp = buf + 1;
    if (*comp == '\0') {
        kind = NC_REF_OBJ_GRP;
        objid = ncid;
        goto resolve_att;
    }

    /* Walk group components; the last one is the leaf. */
    for (;;) {
        char *slash = strchr(comp, '/');
        if (slash)
            *slash = '\0';
        if (*comp == '\0')
            return NC_EINVAL;
        if (slash) {
            if ((ret = nc_inq_grp_ncid(cur, comp, &cur)))
                return ret;
            comp = slash + 1;
        } else {
            leaf = comp;
            break;
        }
    }
    if ((ret = resolve_leaf(cur, leaf, &kind, &objid)))
        return ret;

resolve_att:
    if (hash) {
        if ((ret = check_att(cur, kind, objid, hash + 1)))
            return ret;
        strcpy(att_name, hash + 1);
        *kindp = NC_REF_OBJ_ATT;
    } else {
        *kindp = kind;
    }
    *objidp = objid;
    return NC_NOERR;
#endif
}

/**
 * @brief Open a remote reference target and resolve its object path.
 *
 * Implementation of the public contract declared in `nep.h`: the
 * containing file is validated as NEXTCDF-4 (context only), the URL
 * is opened with plain `nc_open(NC_NOWRITE)` so any remote-capable
 * upstream dispatch answers, and the path is resolved with
 * nc_ref_resolve(). A failed resolution closes the fresh session.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_open_remote(int ncid, const char *url, const char *path,
                   int *target_ncidp, int *kindp, int *objidp,
                   char *att_name, size_t att_name_size)
{
#ifndef HAVE_REMOTE_REFS
    (void)ncid; (void)url; (void)path; (void)target_ncidp;
    (void)kindp; (void)objidp; (void)att_name; (void)att_name_size;
    return NC_ENOTBUILT;
#else
    int ret;

    ELOG((2, "%s: ncid %d url %s path %s", __func__, ncid,
          url ? url : "NULL", path ? path : "NULL"));

    if (!url || !path || !target_ncidp || !kindp || !objidp)
        return NC_EINVAL;
    *target_ncidp = -1;
    if ((ret = nx4_ref_file(ncid, NULL, NULL)))
        return ret;
    if ((ret = nc_open(url, NC_NOWRITE, target_ncidp)))
        return ret;
    if ((ret = nc_ref_resolve(*target_ncidp, path, kindp, objidp,
                              att_name, att_name_size))) {
        nc_close(*target_ncidp);
        *target_ncidp = -1;
    }
    return ret;
#endif
}
