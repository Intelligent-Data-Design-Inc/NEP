# NEXTCDF-4 References {#nextcdf4_refs}

NEXTCDF-4 stores HDF5 references through the atomic `nc_type`
`NC_STD_REF`, which maps to the HDF5 2.x unified reference datatype
`H5T_STD_REF`. Each element of an `NC_STD_REF` variable or attribute is
one opaque 64-byte `nc_std_ref_t` token — layout-compatible with HDF5
`H5R_ref_t`. A single token format covers all three HDF5 reference
kinds: object references, dataset region references, and attribute
references, including references to objects in other files.

The older HDF5 reference datatypes (`H5T_STD_REF_OBJ`,
`H5T_STD_REF_DSETREG` and their `hobj_ref_t`/`hdset_reg_ref_t` tokens)
are deprecated in HDF5 2.x and are not supported. A file containing a
dataset of either deprecated type fails `nc_open` with `NC_EBADTYPE`.

Reference functions are declared in `nep.h` and operate only on files
opened or created with `NC_NEXTCDF4`.

## Requirements and restrictions

- `NC_STD_REF` variables and attributes, and every `nc_ref_*` create
  function, require native NEXTCDF-4 mode. Under `NC_NETCDF4_MODEL` or
  `NC_CLASSIC_MODEL` they return `NC_ENOTNC4`. `nc_deref_*` and
  `nc_ref_get_*` work in any NEXTCDF4 mode.
- `NC_STD_REF` cannot be a compound member. (Reference-typed
  coordinate variables are allowed since v4.3.0 Sprint 1L1.)
- No default fill value is defined; variables behave as if `NC_NOFILL`
  were set unless the caller supplies an explicit fill pattern.
- Tokens are opaque 64-byte values. Never interpret the bytes.
- A token created for one file is meaningful only in the file that
  holds it — except that a token may itself *point into* an external
  file (see below).

## Types and constants

```c
#define NC_STD_REF       30   /* nc_type for H5T_STD_REF */
#define NC_STD_REF_SIZE  64   /* bytes per token, matches H5R_REF_BUF_SIZE */

typedef struct {
    union {
        unsigned char bytes[NC_STD_REF_SIZE];
        int64_t align;
    } u;
} nc_std_ref_t;

/* Token kinds, returned by nc_ref_get_type(). */
#define NC_STD_REF_OBJECT 1
#define NC_STD_REF_REGION 2
#define NC_STD_REF_ATTR   3

/* Object target kinds for nc_ref_object()/nc_ref_attr()/nc_deref_*. */
#define NC_REF_OBJ_VAR  1   /* NetCDF variable (HDF5 dataset) */
#define NC_REF_OBJ_GRP  2   /* NetCDF group (HDF5 group) */
#define NC_REF_OBJ_TYPE 3   /* NetCDF user-defined type (HDF5 committed datatype) */
```

## Storing and reading reference tokens

`NC_STD_REF` variables and attributes are defined and accessed like
any other atomic type: `nc_def_var`, `nc_put_att`, `nc_put_var`,
`nc_put_vara`, `nc_put_vars`, and the matching `nc_get_*` calls move
whole tokens. `nc_inq_type` reports a size of 64. Each element is an
independent token; strided access reads or writes independent
references.

## Creating references in the open file

All create functions take an open `ncid`, fill a caller-supplied
`nc_std_ref_t`, and require native mode (`NC_ENOTNC4` otherwise,
including on a non-NEXTCDF4 `ncid`).

```c
int nc_ref_object(int ncid, int obj_type, int objid, nc_std_ref_t *ref);
```

Creates an object reference. `obj_type` selects what `objid` is:
`NC_REF_OBJ_VAR` — a varid in group `ncid`; `NC_REF_OBJ_GRP` — a
group ncid (`ncid` ignored); `NC_REF_OBJ_TYPE` — a typeid in group
`ncid`.

```c
int nc_ref_region(int ncid, int varid, const size_t *start,
                  const size_t *count, const ptrdiff_t *stride,
                  nc_std_ref_t *ref);
```

Creates a dataset region reference to a hyperslab selection in
variable `varid`. `start`/`count`/`stride` use normal NetCDF indexing;
`stride` may be NULL for unit stride. Any selection expressible as
`start`/`count`/`stride` is allowed.

```c
int nc_ref_attr(int ncid, int obj_type, int objid,
                const char *att_name, nc_std_ref_t *ref);
```

Creates an attribute reference. `obj_type`/`objid` dispatch exactly as
in `nc_ref_object`, so attribute references work on every
attribute-bearing object kind HDF5 supports: variables, groups (the
group's attributes, i.e. what `nc_put_att` attaches to `NC_GLOBAL`),
and committed datatypes. Note the NetCDF API cannot put attributes on
types — a datatype attribute exists only if created through raw HDF5 —
but a reference can point at it regardless. `att_name` must name an
existing attribute on the object (`NC_EINVAL` otherwise).

## Dereferencing references

Dereference functions resolve a token back to NetCDF ids in the file
identified by `ncid`. They accept any NEXTCDF4 mode and return
`NC_EINVAL` when the token kind does not match the function, the
session is not the file the token points into, or the target no longer
exists. For a token pointing into another file, pass the session
returned by `nc_ref_open_target` as `ncid` (see below); the ids it
returns belong to that target session and remain valid until it is
closed.

```c
int nc_deref_object(int ncid, const nc_std_ref_t *ref, int *obj_typep,
                    int *grpidp, int *idp);
```

`*obj_typep` receives `NC_REF_OBJ_VAR`/`NC_REF_OBJ_GRP`/`NC_REF_OBJ_TYPE`.
For a variable, `*grpidp` is the containing group and `*idp` the varid;
for a group, both receive the group ncid; for a type, `*grpidp` is the
containing group and `*idp` the typeid.

```c
int nc_deref_region(int ncid, const nc_std_ref_t *ref, int *grpidp,
                    int *varidp, int *ndimsp, size_t *start,
                    size_t *count, ptrdiff_t *stride);
```

Recovers the target variable and its selection. Only regular strided
hyperslabs are representable: an irregular selection, or one built
with non-unit blocks, returns `NC_EINVAL`. Output pointers may be NULL
to skip a result.

```c
int nc_deref_attr(int ncid, const nc_std_ref_t *ref, int *obj_typep,
                  int *grpidp, int *idp, char *att_name);
```

Returns the attribute-bearing object's kind and ids as in
`nc_deref_object`, plus the attribute name in `att_name`, which must
be a caller buffer of at least `NC_MAX_NAME + 1` bytes. For
`NC_REF_OBJ_TYPE` owners the attribute value is read through the HDF5
API (`H5Aopen`/`H5Aread`, or `H5Ropen_attr` on the token), since the
NetCDF API has no datatype-attribute calls.

## External references — pointing into other files

The `nc_ref_*_ext` functions create tokens that reference objects in a
*different* file. `ncid` is still validated as a native-mode NEXTCDF4
file (the token can then be stored in its `NC_STD_REF` data), but the
target is named by `filename` plus `path`, the object's HDF5 path in
that file (for example `"grp/dset"` or `"/grp/dset"`).

```c
int nc_ref_object_ext(int ncid, const char *filename, const char *path,
                      nc_std_ref_t *ref);
int nc_ref_region_ext(int ncid, const char *filename, const char *path,
                      const size_t *start, const size_t *count,
                      const ptrdiff_t *stride, nc_std_ref_t *ref);
int nc_ref_attr_ext(int ncid, const char *filename, const char *path,
                    const char *att_name, nc_std_ref_t *ref);
```

`path` may name any attribute-bearing object kind, so
`nc_ref_attr_ext` covers the same owner set as `nc_ref_attr`. The
resulting token records `filename` and HDF5 resolves it at open time.

Two requirements apply to `filename`:

- **It must be an absolute filesystem path.** Relative names are
  rejected with `NC_EINVAL`, because a stored relative name would be
  reinterpreted against whatever working directory a later reader
  happens to have. There is no relocation: moving the target file
  breaks references to its old path.
- **The target file must be writable by the application.** HDF5 2.1.1
  reopens persisted external references with read-write intent even
  when only reads are planned, so creation opens the target
  read-write as well. Dereferencing never intentionally modifies
  target contents, but a read-only open is not promised.

### Dereferencing external tokens

```c
int nc_ref_open_target(int ncid, const nc_std_ref_t *ref,
                       int *target_ncidp);
```

`nc_ref_open_target` opens the file an external token points into as a
new NEXTCDF-4 session (`NC_NEXTCDF4|NC_WRITE`) and returns its NetCDF
file ID. The caller owns the returned session and closes it with
`nc_close`; each call returns an independent session even for tokens
into the same file. Pass the returned `target_ncid` to
`nc_deref_object`, `nc_deref_region`, or `nc_deref_attr` to resolve
the token — the returned group, variable and type IDs belong to the
target session and remain valid until it is closed. Token lifetime is
separate: `nc_ref_free` the token when done; freeing it does not close
the target session, and closing the session does not affect the token.

Errors: `NC_EINVAL` for an in-file token, a stored filename that is
not absolute, or a deref session that is not the token's target;
`NC_ENOTNC4` when the target is not a NEXTCDF-4-readable NetCDF-4
file; `NC_EPERM`-family errors when the target cannot be opened
read-write; `nc_open`'s codes (for example `NC_ENOTNC`) for a missing
or unreadable target. On failure `*target_ncidp` is `-1`.

One HDF5 caveat: while any token references a target file, HDF5 holds
that file open internally with its default file-access properties. A
plain `nc_open` on the same file can therefore fail with an HDF error
until the tokens are released with `nc_ref_free` — sessions obtained
through `nc_ref_open_target` use a compatible open mode and are not
affected.

## Remote references — pointing into datasets on other servers

Remote references are *not* `NC_STD_REF` tokens — HDF5 tokens store
filesystem filenames and cannot name a remote resource. A remote
reference is a stored convention of two strings, a **URL** and an
**object path**, carried in `NC_STRING` variables or attributes. No
names are reserved: any parallel pair of string holders works — for
example an `NC_STRING` variable `ref_url` whose values are URLs and a
sibling `ref_path` of the same shape holding object paths, or two
`NC_STRING` attributes.

The remote session is opened with `nc_open(url, NC_NOWRITE, ...)` —
an ordinary upstream NetCDF file ID, not a NEXTCDF-4 session — so any
remote-capable dispatch in the installed NetCDF-C answers: DAP2, DAP4,
or NCZarr `file://`/byte-range (`https://...#mode=bytes`) stores.
`s3://` URLs require NetCDF-C built with `NC_HAS_S3` and are untested.
Remote access is read-only; there is no constraint-expression or
subsetting API beyond the stored object path.

```c
int nc_ref_open_remote(int ncid, const char *url, const char *path,
                       int *target_ncidp, int *kindp, int *objidp,
                       char *att_name, size_t att_name_size);
int nc_ref_resolve(int ncid, const char *path, int *kindp,
                   int *objidp, char *att_name, size_t att_name_size);
```

`nc_ref_open_remote` validates `ncid` as a NEXTCDF-4 file (context
only), opens `url`, and resolves `path` in one call, returning a
caller-owned session the caller must `nc_close`. Each call returns an
independent session. If the open succeeds but the path fails to
resolve, the session is closed and `*target_ncidp` is -1.
`nc_ref_resolve` resolves additional paths on the same session without
paying another remote open.

The path grammar is a leading `/`, `/`-separated group components,
and a final component naming a variable, group, or committed datatype,
with an optional `#attname` suffix selecting an attribute of that
object. `/` alone resolves to the root group. The final component
resolves as variable, then group, then committed type. `kindp`
receives `NC_REF_OBJ_VAR`, `NC_REF_OBJ_GRP`, `NC_REF_OBJ_TYPE`, or
`NC_REF_OBJ_ATT` for a `#attname` path (then `att_name` receives the
attribute name and `objidp` the owner's ID). Attribute existence is
verified for variable and group owners; committed-datatype attributes
are returned unchecked because the NetCDF API cannot inquire them.

The feature requires building NEP with `NEP_ENABLE_REMOTE_REFS=ON`
(reported as `NEP_HAS_REMOTE_REFS` in `nep_meta.h`), which needs
NEXTCDF-4 and a NetCDF-C with `NC_HAS_DAP2`, `NC_HAS_DAP4` or
`NC_HAS_BYTERANGE`. Without it both functions return `NC_ENOTBUILT`.

Errors: `NC_EINVAL` for bad arguments or malformed paths,
`NC_ENOTVAR` for an unresolvable object, `NC_ENOTATT` for a missing
attribute, `NC_ENOTBUILT` when the feature is off, and the propagated
`nc_open`/inquiry error for unreachable or non-NetCDF endpoints.

See `examples/nextcdf4/remote_refs.c` for a complete program.

## Inspecting tokens

```c
int nc_ref_get_type(const nc_std_ref_t *ref, int *ref_typep);
```

Reports the token kind: `NC_STD_REF_OBJECT`, `NC_STD_REF_REGION`, or
`NC_STD_REF_ATTR`.

```c
int nc_ref_get_file_name(const nc_std_ref_t *ref, char *name, size_t size);
int nc_ref_get_obj_name(const nc_std_ref_t *ref, char *name, size_t size);
```

Copy the stored file name (of the file the token *points into*, which
differs from the containing file only for external references) and the
object's path within that file into caller buffers of `size` bytes.
Both return `NC_EINVAL` when `size` is too small. For an attribute
reference, `nc_ref_get_obj_name` returns the name of the object that
carries the attribute; `nc_deref_attr` recovers the attribute name
itself.

## Releasing tokens

```c
int nc_ref_free(nc_std_ref_t *ref);
```

A reference token holds an open reference to the file it was created
in; until it is released the HDF5 file cannot fully close, and a later
`H5Fopen` on the same file fails. Call `nc_ref_free` on every token
obtained from an `nc_ref_*` creator — and on tokens read back from a
variable or attribute — when it is no longer needed, and always before
`nc_close`. Releasing an uninitialized or already-released token is
safe.

## Reference validity

- A reference binds the target *object*, not its path, so renaming a
  referenced dimension, variable, group, or type never invalidates an
  existing reference.
- The NetCDF API cannot delete variables or groups, so in-file
  references cannot dangle through `nc_*` calls. An external reference
  fails to resolve when its target file is moved or deleted;
  `nc_ref_open_target` then reports the `nc_open` error and a raw-HDF5
  `H5Ropen_*` fails.
- Extending an unlimited dimension does not invalidate a region
  reference whose stored selection remains within the variable's
  current shape.

## Example

```c
#include <netcdf.h>
#include <nep.h>

int ncid, dimid, varid, refid;
nc_std_ref_t ref;
size_t one[1] = {0}, count[1] = {1};

nc_create("refs.nc", NC_NEXTCDF4 | NC_CLOBBER, &ncid);
nc_def_dim(ncid, "n", 4, &dimid);
nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid);
nc_def_var(ncid, "refs", NC_STD_REF, 1, &dimid, &refid);
nc_enddef(ncid);

/* Object reference to the variable "data". */
nc_ref_object(ncid, NC_REF_OBJ_VAR, varid, &ref);
nc_put_vara(ncid, refid, one, count, &ref);
nc_ref_free(&ref);
nc_close(ncid);

/* Later: read the token back and resolve it. */
nc_open("refs.nc", NC_NEXTCDF4 | NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "refs", &refid);
nc_get_vara(ncid, refid, one, count, &ref);
int obj_type, grpid, id;
nc_deref_object(ncid, &ref, &obj_type, &grpid, &id);   /* id == varid */
nc_ref_free(&ref);
nc_close(ncid);
```

External variant (absolute filename required):

```c
/* Point at variable "data" in another file. */
nc_ref_object_ext(ncid, "/data/other.nc", "/data", &ref);
nc_put_vara(ncid, refid, one, count, &ref);
nc_ref_free(&ref);   /* releases the token's pin on other.nc */
nc_close(ncid);

/* Later: read the token back and follow it into the other file. */
nc_open("refs.nc", NC_NEXTCDF4 | NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "refs", &refid);
nc_get_vara(ncid, refid, one, count, &ref);

int tnc;
nc_ref_open_target(ncid, &ref, &tnc);      /* caller-owned session */
nc_deref_object(tnc, &ref, &obj_type, &grpid, &id);
nc_get_var_int(tnc, id, values);           /* read through the target */

nc_ref_free(&ref);
nc_close(tnc);                             /* close the target session */
nc_close(ncid);
```
