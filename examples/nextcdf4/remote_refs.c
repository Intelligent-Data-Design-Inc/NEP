/**
 * @file remote_refs.c
 * @brief Store references to remote datasets and follow them.
 *
 * This program demonstrates NEXTCDF-4 remote references. Unlike
 * file references (see external_refs.c), a remote reference is not an
 * HDF5 token: it is a stored convention of two strings — a URL and an
 * object path — kept here as NC_STRING attribute values. The program
 * writes a (URL, path) pair to a NEXTCDF-4 file, reads it back after
 * reopening, then calls nc_ref_open_remote() to open the remote
 * dataset through nc_open() — DAP2, DAP4 or NCZarr dispatch, whatever
 * the installed NetCDF-C resolves — and resolve the path to a real
 * NetCDF variable, group or attribute in one call.
 *
 * **Usage:**
 * @code
 *   NCRCENV_RC=<build>/.ncrc NETCDF_RC=<build> ./remote_refs <url> [path]
 * @endcode
 *
 * With no arguments the program only exercises the storage convention
 * and reports that no remote URL was given. Pass a resolvable URL
 * (for example a public DAP server dataset) and an object path like
 * `/sst` or `/grp/var#units` to resolve a real remote object.
 *
 * **Learning Objectives:**
 * - Store a (URL, object path) remote reference as NC_STRING data
 * - Resolve a remote reference with nc_ref_open_remote()
 * - Resolve further paths on the same session with nc_ref_resolve()
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include "nep.h"
#include "nep_meta.h"

#define FILE_NAME "remote_refs.nc"
#define ATT_URL   "remote_url"
#define ATT_PATH  "remote_path"

#define CHECK(expr) do { \
    int _ret = (expr); \
    if (_ret != NC_NOERR) { \
        fprintf(stderr, "%s:%d: %s failed: %s\n", __FILE__, __LINE__, #expr, nc_strerror(_ret)); \
        return 1; \
    } \
} while (0)

int
main(int argc, char **argv)
{
    int ncid, attid;
    const char *url = argc > 1 ? argv[1] : NULL;
    const char *path = argc > 2 ? argv[2] : "/";

    printf("*** Remote references example.\n");

    /* Store the (URL, path) pair as NC_STRING global attributes —
     * the remote-reference convention. Any string-carrying variable
     * or attribute pair works; no names are reserved. */
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    {
        const char *u[1] = {url ? url : ""};
        const char *p[1] = {path};
        CHECK(nc_put_att_string(ncid, NC_GLOBAL, ATT_URL, 1, u));
        CHECK(nc_put_att_string(ncid, NC_GLOBAL, ATT_PATH, 1, p));
    }
    CHECK(nc_close(ncid));

    /* Read the pair back after reopening. */
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    {
        char *u[1] = {NULL};
        char *p[1] = {NULL};
        CHECK(nc_inq_attid(ncid, NC_GLOBAL, ATT_URL, &attid));
        CHECK(nc_get_att_string(ncid, NC_GLOBAL, ATT_URL, u));
        CHECK(nc_get_att_string(ncid, NC_GLOBAL, ATT_PATH, p));
        printf("stored URL: %s\nstored path: %s\n", u[0], p[0]);
        url = url ? url : (strlen(u[0]) ? u[0] : NULL);
        nc_free_string(1, u);
        nc_free_string(1, p);
    }

#if !NEP_HAS_REMOTE_REFS
    printf("NEP was built without NEP_ENABLE_REMOTE_REFS; remote "
           "resolution is unavailable.\n");
#else
    if (!url) {
        printf("No URL given; storage convention verified. Pass a "
               "DAP or NCZarr URL to resolve a remote object.\n");
    } else {
        int target_ncid, kind, objid;
        char att_name[NC_MAX_NAME + 1] = "";

        CHECK(nc_ref_open_remote(ncid, url, path, &target_ncid, &kind,
                                 &objid, att_name, sizeof(att_name)));
        switch (kind) {
        case NC_REF_OBJ_VAR:  printf("resolved variable, varid %d\n", objid); break;
        case NC_REF_OBJ_GRP:  printf("resolved group, grpid %d\n", objid); break;
        case NC_REF_OBJ_TYPE: printf("resolved type, typeid %d\n", objid); break;
        case NC_REF_OBJ_ATT: printf("resolved attribute '%s', owner id %d\n",
                                     att_name, objid); break;
        }

        /* The session stays open for data reads and further
         * nc_ref_resolve() calls; / always resolves to the root
         * group. */
        {
            int rkind, rid;
            CHECK(nc_ref_resolve(target_ncid, "/", &rkind, &rid,
                                 NULL, 0));
            printf("root group resolves (kind %d, id %d)\n", rkind, rid);
        }
        CHECK(nc_close(target_ncid));
    }
#endif

    CHECK(nc_close(ncid));
    printf("*** SUCCESS!\n");
    return 0;
}
