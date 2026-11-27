/**
 * @file external_refs.c
 * @brief Store references to variables in other files and follow them.
 *
 * This program demonstrates NEXTCDF-4 external references. It creates
 * two target files, each holding one variable with a little sample
 * data and an attribute, then creates a third file whose `refs`
 * variable stores one `NC_STD_REF` token per target variable. After
 * closing everything, it reopens the third file, reads the tokens
 * back, opens each target with nc_ref_open_target(), and resolves the
 * references with nc_deref_object() to read the data and metadata
 * straight through the NetCDF API.
 *
 * **Usage:**
 * @code
 *   NCRCENV_RC=<build>/.ncrc NETCDF_RC=<build> ./external_refs
 * @endcode
 *
 * The NEXTCDF-4 backend is loaded through the .ncrc UDF9 autoload
 * mechanism, so @c NCRCENV_RC and @c NETCDF_RC must name the NEP build
 * directory that contains the generated @c .ncrc. The program writes
 * and reads three files in the current directory:
 * @c external_ref_target_a.h5, @c external_ref_target_b.h5 and
 * @c external_refs.h5.
 *
 * **Learning Objectives:**
 * - Create an external object reference with nc_ref_object_ext()
 * - Store and reload NC_STD_REF tokens like ordinary variable data
 * - Open a token's target file with nc_ref_open_target()
 * - Resolve the target to NetCDF ids with nc_deref_object()
 * - Understand the ownership contract for tokens and target sessions
 *
 * **Key Concepts:**
 * - **Absolute filenames**: external creators require an absolute
 *   filesystem path; a stored relative name would be reinterpreted
 *   against the reader's working directory.
 * - **Writable targets**: HDF5 may reopen external references
 *   read-write, so target files must be writable by the application
 *   even though this program only reads them.
 * - **Object identity**: a reference binds the target object, not its
 *   path, so the target variable can even be renamed after the token
 *   is created.
 * - **Two lifetimes**: a token is released with nc_ref_free(); the
 *   target session returned by nc_ref_open_target() is released with
 *   nc_close(). Neither affects the other.
 *
 * **Key API functions:**
 * - nc_ref_object_ext()   Create a token pointing into another file
 * - nc_ref_open_target()  Open a token's external target file
 * - nc_deref_object()     Resolve a token to NetCDF ids on the target
 * - nc_ref_get_file_name()/nc_ref_get_obj_name()  Inspect a token
 * - nc_ref_free()         Release a token
 *
 * **Related Examples:**
 * - docs/refs.md - The full reference API documentation
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <netcdf.h>
#include <nep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** Number of sample values in each target variable. */
#define NVALS 4
/** Number of reference tokens stored in the index file. */
#define NREFS 2

/** First target file. */
#define TARGET_A "external_ref_target_a.h5"
/** Second target file. */
#define TARGET_B "external_ref_target_b.h5"
/** File holding the reference tokens. */
#define INDEX_FILE "external_refs.h5"

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/* Create one target file with one variable and an attribute. */
static void
make_target(const char *name, const char *var_name,
            const char *units, const int *data)
{
    int ncid, dimid, varid, ret;

    if ((ret = nc_create(name, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
	ERR(ret);
    if ((ret = nc_def_dim(ncid, "n", NVALS, &dimid)))
	ERR(ret);
    if ((ret = nc_def_var(ncid, var_name, NC_INT, 1, &dimid, &varid)))
	ERR(ret);
    if ((ret = nc_put_att_text(ncid, varid, "units", strlen(units),
                               units)))
	ERR(ret);
    if ((ret = nc_enddef(ncid)))
	ERR(ret);
    if ((ret = nc_put_var_int(ncid, varid, data)))
	ERR(ret);
    if ((ret = nc_close(ncid)))
	ERR(ret);
}

int
main(void)
{
    static const int data_a[NVALS] = {10, 11, 12, 13};
    static const int data_b[NVALS] = {20, 21, 22, 23};
    char cwd[1024], abs_a[1200], abs_b[1200];
    int ncid, dimid, refid;
    size_t start[1] = {0}, count[1] = {NREFS};
    nc_std_ref_t refs[NREFS];
    int i, ret;

    /* The external creators require absolute filenames; build them
     * from the current directory so the stored names work no matter
     * where the reader runs from later. */
    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "getcwd failed\n");
        return 2;
    }
    snprintf(abs_a, sizeof(abs_a), "%s/%s", cwd, TARGET_A);
    snprintf(abs_b, sizeof(abs_b), "%s/%s", cwd, TARGET_B);

    /* --- Create the two target files and the index file. --- */

    make_target(TARGET_A, "alpha", "m/s", data_a);
    make_target(TARGET_B, "beta", "K", data_b);
    printf("Created %s (var alpha) and %s (var beta)\n",
           TARGET_A, TARGET_B);

    if ((ret = nc_create(INDEX_FILE, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
	ERR(ret);
    if ((ret = nc_def_dim(ncid, "nref", NREFS, &dimid)))
	ERR(ret);
    if ((ret = nc_def_var(ncid, "refs", NC_STD_REF, 1, &dimid, &refid)))
	ERR(ret);
    if ((ret = nc_enddef(ncid)))
	ERR(ret);

    /* One object-reference token per target variable, stored like
     * ordinary variable data. */
    if ((ret = nc_ref_object_ext(ncid, abs_a, "/alpha", &refs[0])))
	ERR(ret);
    if ((ret = nc_ref_object_ext(ncid, abs_b, "/beta", &refs[1])))
	ERR(ret);
    if ((ret = nc_put_vara(ncid, refid, start, count, refs)))
	ERR(ret);
    /* Live tokens pin their target files open; release them before
     * closing. */
    for (i = 0; i < NREFS; i++)
	if ((ret = nc_ref_free(&refs[i])))
	    ERR(ret);
    if ((ret = nc_close(ncid)))
	ERR(ret);
    printf("Stored %d external reference tokens in %s\n\n",
           NREFS, INDEX_FILE);

    /* --- Reopen the index file and follow the references. --- */

    if ((ret = nc_open(INDEX_FILE, NC_NEXTCDF4 | NC_NOWRITE, &ncid)))
	ERR(ret);
    if ((ret = nc_inq_varid(ncid, "refs", &refid)))
	ERR(ret);
    memset(refs, 0, sizeof(refs));
    if ((ret = nc_get_vara(ncid, refid, start, count, refs)))
	ERR(ret);

    for (i = 0; i < NREFS; i++) {
        char fname[1024], oname[NC_MAX_NAME + 1], vname[NC_MAX_NAME + 1],
            units[64];
        int tnc, obj_type, grpid, varid, ndims, dimid_t;
        size_t dimlen;
        int vals[NVALS];
        int d;

        /* Inspect the token, open its target file (caller-owned
         * session), and resolve it to NetCDF ids on that session. */
        if ((ret = nc_ref_get_file_name(&refs[i], fname, sizeof(fname))))
	    ERR(ret);
        if ((ret = nc_ref_get_obj_name(&refs[i], oname, sizeof(oname))))
	    ERR(ret);
        if ((ret = nc_ref_open_target(ncid, &refs[i], &tnc)))
	    ERR(ret);
        if ((ret = nc_deref_object(tnc, &refs[i], &obj_type, &grpid,
                                   &varid)))
	    ERR(ret);

        printf("token %d -> %s object %s\n", i, fname, oname);
        if (obj_type != NC_REF_OBJ_VAR) {
            fprintf(stderr, "expected a variable reference\n");
            return ERRCODE;
        }

        /* Metadata and data through the returned target ids. */
        if ((ret = nc_inq_var(grpid, varid, vname, NULL, &ndims,
                             &dimid_t, NULL)))
	    ERR(ret);
        if ((ret = nc_inq_dimlen(grpid, dimid_t, &dimlen)))
	    ERR(ret);
        {
            size_t attlen = 0;

            memset(units, 0, sizeof(units));
            if ((ret = nc_inq_attlen(grpid, varid, "units", &attlen)))
		ERR(ret);
            if ((ret = nc_get_att_text(grpid, varid, "units", units)))
		ERR(ret);
            units[attlen] = '\0';
        }
        if ((ret = nc_get_var_int(grpid, varid, vals)))
	    ERR(ret);
        printf("  var %s, ndims %d, len %lu, units %s, data",
               vname, ndims, (unsigned long)dimlen, units);
        for (d = 0; d < NVALS; d++)
            printf(" %d", vals[d]);
        printf("\n");

        /* Close the caller-owned target session; the token lifetime is
         * independent. */
        if ((ret = nc_close(tnc)))
	    ERR(ret);
        if ((ret = nc_ref_free(&refs[i])))
	    ERR(ret);
    }

    if ((ret = nc_close(ncid)))
	ERR(ret);
    printf("\nDone.\n");
    return 0;
}
