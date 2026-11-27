/*
 * tst_nextcdf4_remote_ref.c - tests for remote (URL, object path)
 * reference storage and resolution through nc_ref_open_remote() and
 * nc_ref_resolve().
 *
 * Runs in two configurations: built without NEP_ENABLE_REMOTE_REFS
 * (the CI configuration) the resolution calls must return
 * NC_ENOTBUILT; built with it, argument and path-grammar errors are
 * checked, and a live URL is exercised only when the
 * NEP_REMOTE_TEST_URL environment variable is set (never in CI).
 *
 * Author: Edward Hartnett
 * Date: 2026-11-27
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include "nep.h"
#include "nep_meta.h"
#include "nextcdf4dispatch.h"

#define FILE_NAME "tst_nextcdf4_remote_ref.nc"
#define TEST_URL  "https://example.invalid/dap/data.nc"
#define TEST_PATH "/grp/var#units"

/* Helper macro for error checking. */
#define CHECK(expr) do { \
    int _ret = (expr); \
    if (_ret != NC_NOERR) { \
        fprintf(stderr, "%s:%d: %s failed: %s\n", __FILE__, __LINE__, #expr, nc_strerror(_ret)); \
        return 1; \
    } \
} while (0)

/* Require a specific NetCDF error code. */
#define EXPECT(expr, want) do { \
    int _ret = (expr); \
    if (_ret != (want)) { \
        fprintf(stderr, "%s:%d: %s returned %d, wanted %d\n", __FILE__, __LINE__, #expr, _ret, (want)); \
        return 1; \
    } \
} while (0)

int
main(void)
{
    int ncid, dimid, url_var, path_var;
    int target_ncid, kind, objid;
    char att_name[NC_MAX_NAME + 1];
    const char *url, *path;

    printf("*** Testing remote reference storage and resolution.\n");

    /* Stored convention: parallel NC_STRING variables hold the URL and
     * the object path. Pure NC_STRING I/O; works regardless of the
     * remote-refs build option. */
    printf("*** Testing NC_STRING (URL, path) storage convention...\n");
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_dim(ncid, "ref", 2, &dimid));
    CHECK(nc_def_var(ncid, "ref_url", NC_STRING, 1, &dimid, &url_var));
    CHECK(nc_def_var(ncid, "ref_path", NC_STRING, 1, &dimid, &path_var));
    {
        const char *urls[2] = {TEST_URL, TEST_URL};
        const char *paths[2] = {"/var", TEST_PATH};
        CHECK(nc_put_var_string(ncid, url_var, urls));
        CHECK(nc_put_var_string(ncid, path_var, paths));
    }
    CHECK(nc_close(ncid));

    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "ref_url", &url_var));
    CHECK(nc_inq_varid(ncid, "ref_path", &path_var));
    {
        char *urls[2] = {NULL, NULL};
        char *paths[2] = {NULL, NULL};
        CHECK(nc_get_var_string(ncid, url_var, urls));
        CHECK(nc_get_var_string(ncid, path_var, paths));
        if (strcmp(urls[0], TEST_URL) || strcmp(paths[1], TEST_PATH))
            return 1;
        nc_free_string(2, urls);
        nc_free_string(2, paths);
    }

#if NEP_HAS_REMOTE_REFS
    printf("*** Testing argument and path-grammar errors...\n");
    EXPECT(nc_ref_open_remote(ncid, NULL, "/x", &target_ncid, &kind,
                              &objid, att_name, sizeof(att_name)),
           NC_EINVAL);
    EXPECT(nc_ref_open_remote(ncid, TEST_URL, NULL, &target_ncid,
                              &kind, &objid, att_name,
                              sizeof(att_name)), NC_EINVAL);
    EXPECT(nc_ref_open_remote(ncid, TEST_URL, "/x", NULL, &kind,
                              &objid, att_name, sizeof(att_name)),
           NC_EINVAL);
    EXPECT(nc_ref_resolve(ncid, NULL, &kind, &objid, att_name,
                          sizeof(att_name)), NC_EINVAL);
    EXPECT(nc_ref_resolve(ncid, "no/leading/slash", &kind, &objid,
                          att_name, sizeof(att_name)), NC_EINVAL);
    EXPECT(nc_ref_resolve(ncid, "/a//b", &kind, &objid, att_name,
                          sizeof(att_name)), NC_EINVAL);
    EXPECT(nc_ref_resolve(ncid, "/var#", &kind, &objid, att_name,
                          sizeof(att_name)), NC_EINVAL);
    EXPECT(nc_ref_resolve(ncid, "/var#a#b", &kind, &objid, att_name,
                          sizeof(att_name)), NC_EINVAL);
    EXPECT(nc_ref_resolve(ncid, "/var#att", &kind, &objid, NULL, 0),
           NC_EINVAL);
    EXPECT(nc_ref_resolve(ncid, "/does_not_exist", &kind, &objid,
                          att_name, sizeof(att_name)), NC_ENOTVAR);

    /* A URL that no dispatch can open must surface nc_open's error,
     * not succeed or crash. */
    if (nc_ref_open_remote(ncid, "garbage:://bad", "/x", &target_ncid,
                           &kind, &objid, att_name,
                           sizeof(att_name)) == NC_NOERR) {
        fprintf(stderr, "garbage URL unexpectedly opened\n");
        return 1;
    }
    if (target_ncid != -1)
        return 1;

    /* Live remote resolution: opt-in via environment, skipped in CI. */
    url = getenv("NEP_REMOTE_TEST_URL");
    if (url) {
        int ncid2, kind2, objid2;

        path = getenv("NEP_REMOTE_TEST_PATH");
        if (!path)
            path = "/";
        printf("*** Testing live remote resolution of %s %s...\n",
               url, path);
        CHECK(nc_ref_open_remote(ncid, url, path, &target_ncid, &kind,
                                 &objid, att_name, sizeof(att_name)));
        /* A second open is an independent session. */
        CHECK(nc_ref_open_remote(ncid, url, "/", &ncid2, &kind2,
                                 &objid2, NULL, 0));
        if (kind2 != NC_REF_OBJ_GRP)
            return 1;
        CHECK(nc_close(ncid2));
        /* A deliberately bad path on a good URL closes the session. */
        EXPECT(nc_ref_open_remote(ncid, url, "/no_such_object_xyz",
                                  &ncid2, &kind, &objid,
                                  att_name, sizeof(att_name)),
               NC_ENOTVAR);
        if (ncid2 != -1)
            return 1;
        CHECK(nc_close(target_ncid));
    } else {
        printf("*** SKIP live remote resolution (NEP_REMOTE_TEST_URL not set)\n");
    }
#else
    printf("*** Testing NC_ENOTBUILT without NEP_ENABLE_REMOTE_REFS...\n");
    EXPECT(nc_ref_open_remote(ncid, TEST_URL, "/x", &target_ncid,
                              &kind, &objid, att_name,
                              sizeof(att_name)), NC_ENOTBUILT);
    EXPECT(nc_ref_resolve(ncid, "/x", &kind, &objid, att_name,
                          sizeof(att_name)), NC_ENOTBUILT);
#endif

    CHECK(nc_close(ncid));
    printf("*** SUCCESS!\n");
    return 0;
}
