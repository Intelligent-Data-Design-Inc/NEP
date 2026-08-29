# NEP Development Roadmap

### V4.0.0 - NEXTCDF4

NEXTCDF-4 is a clean-room rewrite of the NetCDF-4/HDF5 backend delivered as a NEP UDF expansion pack. The complete architecture, compatibility contract, datatype roadmap, and proposed source layout are defined in `docs/plan/NEXTCDF4_plan.md`.

#### Sprint 1: Prepare the NEXTCDF-4 Foundation
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "NEXTCDF-4 as a NEP UDF Expansion Pack," "Implementation Phases," and "`src/nextcdf4/` Source Organization."

**Objective:** Establish a buildable, testable NEXTCDF-4 plugin skeleton and prove UDF9 registration before implementing HDF5 file operations. This sprint creates the integration boundary that later sprints will fill in; it does not create, open, modify, or read files.

**Prerequisites:**
- Complete the v3.5.0 `nep_meta.h` work so NEXTCDF-4 availability can be advertised as `NEP_HAS_NEXTCDF4`.
- Build against a netcdf-c version that provides `NC_UDF9`, supports UDF plugin autoloading, and exposes the `NC_Dispatch` interface required by the backend.
- Detect HDF5 1.14.0 or newer. If the dependency is unavailable, keep NEXTCDF-4 disabled without affecting the existing NEP handlers or the built-in NetCDF-4 backend.

**Deliverables:**
- Reserve UDF9 permanently in `include/nep.h` with `NEP_UDF_NEXTCDF4` and the public alias `NC_NEXTCDF4`; update the UDF allocation documentation so the slot is no longer described as reserved.
- Add an opt-in CMake option for NEXTCDF-4, disabled by default, and gate it on compatible netcdf-c and HDF5 versions. Keep existing configurations and installations unchanged when the option is off.
- Create the initial `src/nextcdf4/` build target and private headers. Limit the first source set to dispatch, initialization/finalization, and the minimum shared definitions needed by later sprints.
- Implement `NC_NEXTCDF4_initialize()` and `NC_NEXTCDF4_finalize()` with repeatable initialization, safe cleanup, and UDF9 registration through `nc_def_user_format()`.
- Define the NEXTCDF-4 `NC_Dispatch` table with the correct model identifier and complete ABI-compatible layout for the supported netcdf-c version. File and metadata callbacks remain intentional stubs that return an appropriate NetCDF "not supported/not implemented" error rather than succeeding or dereferencing null pointers.
- Add `.ncrc`/autoload metadata for `NETCDF.UDF9.LIBRARY` and `NETCDF.UDF9.INIT`, following the same install and plugin-discovery conventions as existing NEP UDF handlers.
- Install any public declarations needed to initialize and feature-detect NEXTCDF-4, while keeping implementation headers private.
- Update architecture, build, and UDF-slot documentation to describe NEXTCDF-4 as an optional backend that coexists with netcdf-c's built-in HDF5 backend.

**Verification and acceptance criteria:**
- A default build with NEXTCDF-4 disabled continues to configure, build, and pass the full existing unit test suite.
- An enabled build with supported dependencies compiles and links the NEXTCDF-4 plugin without unresolved dispatch symbols or compiler warnings introduced by this sprint.
- A focused unit test initializes the plugin, confirms UDF9 registration, verifies repeated initialization/finalization is safe, and confirms a representative unimplemented operation returns the documented error without creating a file.
- Autoload verification confirms the installed plugin can be discovered through the UDF9 `.ncrc` keys without an explicit initialization call.
- Configuration with HDF5 older than 1.14.0 either disables NEXTCDF-4 with a clear status message or fails only when the user explicitly requires the feature; all unrelated NEP features remain buildable.
- Public UDF constants, generated feature macros, installed configuration, and user-facing documentation agree on whether NEXTCDF-4 is available.

**Out of scope for Sprint 1:**
- HDF5 `create`, `open`, `close`, or metadata discovery.
- Superblock selection, `_Nextcdf4Backend`/`_Nextcdf4Model` markers, and `NC_NETCDF4_MODEL` behavior.
- Dimensions, variables, attributes, groups, user-defined types, filters, data I/O, renaming fixes, Fortran APIs, `nextcopy`, and `nextdump`.
- Automatic magic-number selection for HDF5 files; NEXTCDF-4 selection remains explicit through `NC_NEXTCDF4`.

#### Sprint 2: Implement the File Lifecycle
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "Create-Time Backend Selection," "Open-Time Backend Selection," "Stored Markers," "Use Superblock v3," and "File life-cycle."

**Objective:** Replace the Sprint 1 file-operation stubs with a complete lifecycle for empty NEXTCDF-4 files. When this sprint is complete, `nc_create()`, `nc_open()`, and `nc_close()` will work for NEXTCDF-4 files selected explicitly with `NC_NEXTCDF4`; metadata definition and data I/O remain later work.

**Prerequisites and constraints:**
- Build on the Sprint 1 UDF9 dispatch registration, `.ncrc` autoload configuration, dependency checks, and `NEP_HAS_NEXTCDF4` feature macro.
- Continue using `/usr/local/netcdf-c` and `/usr/local/hdf5-2.1.1` for local development, with Fortran disabled for this sprint.
- Keep NEXTCDF-4 external to netcdf-c: do not modify netcdf-c's dispatch, model-inference, or HDF5 backend sources.
- Treat the implementation as a clean rewrite. Existing netcdf-c `libsrc4` and `libhdf5` code may be studied for required behavior and ABI contracts but must not be copied.
- Require explicit `NC_NEXTCDF4` selection for both create and open; do not register the shared HDF5 magic number for UDF9.

**Lifecycle design:**
- Add a private `nxt4internal.h` containing the minimum per-file state for this sprint: the owning `NC`/ncid relationship, normalized mode, path, HDF5 file and root-group identifiers, read-only/define-mode state, and ownership flags used for reliable cleanup.
- Split lifecycle code out of `nxt4dispatch.c` into `nxt4create.c`, `nxt4open.c`, and `nxt4file.c`, leaving dispatch registration and table wiring in `nxt4dispatch.c`.
- Attach NEXTCDF-4 state to the netcdf-c `NC` handle through the supported dispatch interface, initialize the minimum root `NC_FILE_INFO_T`/`NC_GRP_INFO_T` metadata required by existing `NC4_*` inquiry callbacks, and remove all registered state on close or failed initialization.
- Centralize HDF5 identifier cleanup so partial create/open failures close resources exactly once and never leave a usable ncid, leaked identifier, or half-created in-memory file entry.
- Translate HDF5 failures to stable NetCDF error codes and suppress expected HDF5 diagnostic-stack output on handled error paths.

**Create path (`nxt4create.c`):**
- Implement `NEXTCDF4_create()` and internal helpers to validate the path and create-mode flags before allocating persistent state.
- Accept `NC_NEXTCDF4 | NC_CLOBBER` and `NC_NEXTCDF4 | NC_NOCLOBBER`; accept redundant `NC_NETCDF4 | NC_NEXTCDF4` as specified by the NEXTCDF-4 plan.
- Reject incompatible format-selection flags and reject `NC_CLASSIC_MODEL | NC_NETCDF4_MODEL` together with `NC_EINVAL`.
- Add the public `NC_NETCDF4_MODEL` flag defined by the plan, ensuring its bit does not overlap the installed netcdf-c mode flags.
- Check the linked HDF5 runtime version before creating a file and fail cleanly if it is older than the configured minimum.
- Create an HDF5 file-access property list and select library-version bounds according to the plan: `H5F_LIBVER_LATEST` for native NEXTCDF-4 and `NC_CLASSIC_MODEL`, and `H5F_LIBVER_V110` for `NC_NETCDF4_MODEL` compatibility files.
- Map `NC_CLOBBER`/`NC_NOCLOBBER` to `H5F_ACC_TRUNC`/`H5F_ACC_EXCL` and preserve the expected NetCDF errors for an existing file, invalid path, or permission failure.
- Create/open the root group and write `_Nextcdf4Backend` on every new file; write `_Nextcdf4Model = 1` only when `NC_NETCDF4_MODEL` was requested. Flush these markers before returning success.
- Return a valid ncid in define mode with zero dimensions, variables, and user attributes visible through the basic NetCDF inquiry API.

**Open path (`nxt4open.c`):**
- Implement `NEXTCDF4_open()` and internal helpers using `H5F_ACC_RDONLY` for `NC_NOWRITE` and `H5F_ACC_RDWR` for `NC_WRITE`.
- Require a valid HDF5 file and `_Nextcdf4Backend`; reject non-HDF5 inputs and unmarked HDF5/legacy NetCDF-4 files rather than silently claiming files owned by the built-in backend.
- Read and validate `_Nextcdf4Backend` and optional `_Nextcdf4Model`, restore the mode recorded by the file, and reject malformed or unsupported marker values.
- Reconstruct only the root and empty-file metadata needed in this sprint. Opening files that contain dimensions, variables, subgroups, user-defined types, or unsupported NEXTCDF-4 metadata must fail explicitly rather than returning incomplete metadata.
- Honor read-only mode throughout the in-memory state so later modifying calls cannot accidentally write through an `NC_NOWRITE` handle.

**Close, abort, and basic inquiry path (`nxt4file.c`):**
- Implement `NEXTCDF4_close()` to flush writable files, close the root group and HDF5 file, free path and metadata allocations, detach dispatch state, and return the first meaningful cleanup error.
- Implement `NEXTCDF4_abort()` so a newly created define-mode file can be abandoned without leaking HDF5 or netcdf-c resources. Sprint 2 retains the valid empty file, matching the close path, so it can be reopened explicitly with `NC_NEXTCDF4`; test this behavior.
- Implement the minimum `sync`, `inq_format`, and `inq_format_extended` behavior needed for a valid lifecycle. `nc_inq_format()` reports NetCDF-4 semantics, while `nc_inq_format_extended()` identifies the UDF9/NEXTCDF-4 backend and returns the effective mode.
- Make repeated close impossible through normal ncid validation and ensure operations on a closed ncid return the standard bad-ID error.

**Implementation sequence:**
1. Define private lifecycle structures, ownership rules, and shared cleanup/error helpers.
2. Implement create for an empty native NEXTCDF-4 file, then verify the HDF5 signature, superblock version, and backend marker directly with HDF5.
3. Implement close/abort and verify that all HDF5 object counts return to their pre-operation values.
4. Implement read-only open/close of a file created in step 2, including marker validation and empty root metadata reconstruction.
5. Add read/write open, clobber/no-clobber handling, runtime-version checks, and failure-path cleanup.
6. Add `NC_NETCDF4_MODEL`, compatibility-mode property bounds, and marker restoration.
7. Replace lifecycle stubs in the dispatch table, then expand direct-registration and `.ncrc` autoload tests to exercise the public `nc_*` API.
8. Update design and user documentation with selection flags, compatibility behavior, and the intentional empty-file limitation for Sprint 2.

**Verification and acceptance criteria:**
- `nc_create(path, NC_NEXTCDF4 | NC_CLOBBER, &ncid)` succeeds, returns a valid ncid, and `nc_close(ncid)` produces a valid marked HDF5 file.
- `nc_open(path, NC_NEXTCDF4 | NC_NOWRITE, &ncid)` and `nc_open(path, NC_NEXTCDF4 | NC_WRITE, &ncid)` both succeed for a Sprint 2 file and close cleanly.
- `nc_inq()` on a newly created or reopened file reports zero dimensions, zero variables, zero visible global attributes, and no unlimited dimension.
- Native and `NC_CLASSIC_MODEL` files use the planned latest-format bounds; `NC_NETCDF4_MODEL` files use the planned compatibility bounds. Tests inspect the resulting superblock rather than assuming the property-list request succeeded.
- `_Nextcdf4Backend` survives close/reopen, `_Nextcdf4Model` is present only in compatibility mode, and neither marker is counted as a user-visible global attribute.
- `NC_NOCLOBBER` preserves an existing file and returns the expected NetCDF error; `NC_CLOBBER` replaces it.
- Explicit open rejects an ordinary unmarked HDF5/NetCDF-4 file, a non-HDF5 file, a missing file, and malformed marker metadata with stable errors and no leaked HDF5 identifiers.
- Repeated create/open/close cycles pass under HDF5 leak checks; injected or naturally triggered failures at each allocation/open step leave no registered file state behind.
- Direct UDF initialization and `.ncrc` autoload paths both pass the same public lifecycle tests.
- A clean CMake build with `NEP_ENABLE_NEXTCDF4=ON` and `NEP_ENABLE_FORTRAN=OFF` passes the full C test suite against `/usr/local/netcdf-c` and `/usr/local/hdf5-2.1.1`.

**Out of scope for Sprint 2:**
- Defining, reading, or writing dimensions, variables, user attributes, groups, filters, or user-defined datatypes.
- Opening populated NetCDF-4/HDF5 files or files produced by the built-in netcdf-c HDF5 backend.
- `redef`, `enddef`, fill behavior beyond lifecycle defaults, parallel I/O, diskless/in-memory files, and non-default virtual file drivers.
- Rename fixes, dimension scales, `_Netcdf4Coordinates`, `_Netcdf4Dimid`, new numeric/reference/bitfield types, Fortran APIs, `nextcopy`, and `nextdump`.

#### Sprint 3: Build the Core Metadata Model
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "Dimension Scales and Dimension Mapping," "Backward Compatibility," "Implementation Phases," and the type-mapping summary.

**Objective:** Replace the core metadata stubs with a persistent root-group metadata implementation for dimensions, variables, and attributes. Sprint 3 supports complete create/define/inquire/close/reopen round trips for metadata built from the existing fixed-size NetCDF atomic types; variable data I/O and dimension-scale attachment remain Sprint 4 work.

**Scope decisions:**
- Persist Sprint 3 metadata to HDF5 and reconstruct it through NEXTCDF-4 on reopen rather than limiting the sprint to an in-memory model or HDF5-only inspection.
- Support both global and variable attributes, including definition, retrieval, inquiry, rename, and deletion.
- Create basic HDF5 dimension-scale datasets and stable `_Netcdf4Dimid` values in this sprint. Attaching scales to variable datasets and writing `_Netcdf4Coordinates` remain in Sprint 4.
- Support the fixed-size standard atomic types allowed by the active model: `NC_BYTE`, `NC_CHAR`, `NC_SHORT`, `NC_INT`, `NC_FLOAT`, `NC_DOUBLE`, `NC_UBYTE`, `NC_USHORT`, `NC_UINT`, `NC_INT64`, and `NC_UINT64`. Defer `NC_STRING`, user-defined types, and all NEXTCDF-4-specific types.

**Prerequisites and constraints:**
- Build on the Sprint 2 file lifecycle, root `NC_FILE_INFO_T`/`NC_GRP_INFO_T` state, HDF5 identifiers, mode validation, markers, synchronization, and cleanup behavior.
- Continue to require explicit `NC_NEXTCDF4` selection and keep all changes within NEP; do not modify netcdf-c or its built-in HDF5 backend.
- Use netcdf-c's common NetCDF-4 metadata structures and inquiry helpers where their contracts fit, while keeping all new HDF5 persistence and discovery code in `src/nextcdf4/`.
- Restrict Sprint 3 to the root group. Group creation and nested metadata remain Sprint 5 work.
- Enforce `NC_NOWRITE`, define-mode, duplicate-name, invalid-ID, invalid-name, and active-model restrictions with standard NetCDF error codes.
- Preserve Sprint 2's hidden backend/model markers and ensure they never appear as user attributes.

**Metadata architecture:**
- Extend private NEXTCDF-4 state with the ownership and lookup information needed to keep HDF5 objects synchronized with `NC_FILE_INFO_T`, `NC_GRP_INFO_T`, `NC_DIM_INFO_T`, `NC_VAR_INFO_T`, and `NC_ATT_INFO_T` metadata.
- Add focused source modules for dimensions, variables, attributes, metadata loading, and shared type/name helpers rather than growing lifecycle or dispatch files into general metadata implementations.
- Allocate stable dimension and variable IDs in definition order and rebuild the same IDs when reopening a file.
- Represent each dimension with a basic HDF5 dimension-scale dataset carrying `_Netcdf4Dimid`. Support one or more unlimited dimensions in enhanced mode, but enforce the classic-model single-unlimited-dimension and ordering rules.
- Create variable datasets with the correct rank, extents, maximum extents, and fixed-size HDF5 atomic datatype. Dataset creation in this sprint establishes persistent variable metadata but does not expose variable data reads or writes.
- Store user attributes on the root group or variable dataset as appropriate and distinguish them from HDF5 and NEXTCDF-4 internal attributes during inquiry and open.
- Load dimensions before variables and variables before their attributes when reopening, validating malformed, duplicate, unsupported, or inconsistent metadata instead of exposing a partial file.

**Dispatch and API coverage:**
- Implement `redef` and `_enddef`, including legal mode transitions, read-only rejection, metadata materialization, flush behavior, and rollback/cleanup after a failed transition.
- Implement dimension definition and inquiry callbacks: define, lookup by name, lookup by ID, unlimited-dimension inquiry, and root-group dimension-ID enumeration. Dimension rename remains Sprint 8 work.
- Implement variable definition and inquiry callbacks: define, lookup by name, lookup by ID, complete variable inquiry, and root-group variable-ID enumeration. Variable rename and all `get_var*`/`put_var*` operations remain later work.
- Implement global and variable attribute put/get/inquiry/name/ID/rename/delete callbacks for the supported atomic types, including zero-length attributes and type conversion behavior provided by the supported NetCDF API contract.
- Keep unsupported callbacks wired to explicit `NC_ENOTBUILT`, `NC_ENOTNC4`, or read-only errors as appropriate; no callback may succeed without implementing its promised state change.
- Update `sync`, `close`, and `open` so pending metadata is materialized and flushed, populated Sprint 3 files can be reconstructed, and partial-load failures release every HDF5 identifier and netcdf-c metadata allocation.

**Implementation sequence:**
1. Define metadata ownership, ID-allocation, HDF5 naming, internal-attribute filtering, and supported-type mapping rules; add shared helpers and focused tests for those rules.
2. Implement define-mode transitions and root metadata transaction boundaries without changing variable data I/O.
3. Implement dimensions, basic dimension-scale creation, `_Netcdf4Dimid`, dimension inquiry, and classic/enhanced unlimited-dimension validation.
4. Implement fixed-size atomic variable definitions and inquiries, creating correctly shaped HDF5 datasets without attaching scales or enabling public data access.
5. Implement global and variable attribute mutation and inquiry, including filtering of all backend, dimension-scale, and NetCDF hidden attributes.
6. Implement populated-file metadata discovery in dependency order and verify stable IDs and equivalent inquiries after close/reopen.
7. Wire the completed callbacks into the dispatch table, retain intentional stubs for later sprints, and expand direct-registration and autoload test coverage.
8. Update NEXTCDF-4 design and user documentation to describe the supported Sprint 3 metadata subset and its intentional I/O limitations.

**Verification and acceptance criteria:**
- A file can define multiple fixed and unlimited dimensions, fixed-size atomic variables of ranks zero and higher, and global and variable attributes, then return matching metadata through all relevant inquiry calls.
- Closing and reopening through `NC_NEXTCDF4` preserves names, IDs, lengths, unlimited status, variable rank/type/dimension IDs, and user attributes.
- Direct HDF5 inspection confirms each dimension has a dimension-scale dataset and stable `_Netcdf4Dimid`, variable datasets have the expected datatype and dataspace, and internal attributes are not counted as user attributes.
- `nc_enddef()` and `nc_redef()` enforce valid transitions; repeated or invalid transitions, metadata mutation outside define mode where prohibited, and all mutation through `NC_NOWRITE` handles return the expected errors.
- Classic-model files reject enhanced atomic types, multiple unlimited dimensions, and an unlimited dimension in a disallowed variable position. Enhanced and `NC_NETCDF4_MODEL` files accept only the types allowed by their respective contracts.
- Duplicate names, invalid names, bad IDs, unsupported types, malformed on-disk metadata, and injected HDF5 failures return stable errors without leaking HDF5 identifiers or leaving partially registered metadata.
- Global and variable attribute put/get/rename/delete operations round-trip all supported types, lengths, and zero-length cases while hidden attributes remain invisible.
- Variable data access continues to return the documented not-implemented error, proving Sprint 3 does not accidentally claim Sprint 4 behavior.
- Direct initialization and `.ncrc` autoload run the same metadata round-trip tests, and a clean NEXTCDF-4-enabled build passes the full C test suite.

**Out of scope for Sprint 3:**
- Variable data reads or writes, dimension-scale attachment, `DIMENSION_LIST`/`REFERENCE_LIST`, and `_Netcdf4Coordinates`; these belong to Sprint 4.
- Nested groups, `NC_STRING`, compound, enum, opaque, or vlen types; these belong to Sprint 5.
- Chunking controls, compression, filters, checksums, quantization, endianness controls, and detailed fill behavior; these belong to Sprint 6.
- General discovery of legacy or arbitrary upstream NetCDF-4/HDF5 layouts beyond the populated subset written by Sprint 3; broader interoperability belongs to Sprint 7.
- Dimension and variable rename behavior, which remains Sprint 8 work. Attribute rename is included because it does not alter dimension-scale relationships.
- New floating-point, complex, bitfield, and reference types, tools, language bindings, parallel I/O, diskless/in-memory files, and non-default HDF5 virtual file drivers.

#### Sprint 4: Implement Variable I/O and Dimension Scales
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "Dimension Scales and Dimension Mapping," "Use Superblock v3," "Backward Compatibility," and the type-mapping summary.

**Objective:** Add read and write access for standard atomic variables, including scalar, array, strided, and unlimited-dimension data. Attach the basic HDF5 dimension scales created in Sprint 3, write `_Netcdf4Coordinates` and `DIMENSION_LIST`/`REFERENCE_LIST`, and reconstruct complete variable-to-dimension mappings when files are reopened. The legacy mapped variable I/O calls (`nc_get_varm`/`nc_put_varm`) are intentionally out of scope and will remain unimplemented in NEXTCDF-4 because they are deprecated.

**Scope decisions:**
- Support the full generic variable I/O surface except the deprecated mapped (`varm`) variants: `nc_put_var`/`nc_get_var`, `nc_put_var1`/`nc_get_var1`, `nc_put_vara`/`nc_get_vara`, and `nc_put_vars`/`nc_get_vars` for all fixed-size standard atomic types.
- Do not implement `nc_put_varm`/`nc_get_varm`; keep the dispatch table entries returning `NC_ENOTBUILT` and document that mapped I/O is deprecated in NEXTCDF-4.
- Support full read and extend behavior for unlimited dimensions, including appending record data and re-opening to report the updated length.
- Treat a variable that shares a dimension's name and has the same one-dimensional shape as the basic coordinate variable: it is stored using the same HDF5 dimension-scale dataset as the dimension and is identified during reopen and inquiry.
- Keep strided (`vars`) read and write in Sprint 4; leave the mapped (`varm`) calls unimplemented.

**Prerequisites and constraints:**
- Build on the Sprint 3 metadata model: `NC_FILE_INFO_T`/`NC_GRP_INFO_T`/`NC_DIM_INFO_T`/`NC_VAR_INFO_T`/`NC_ATT_INFO_T`, stable IDs, `_Netcdf4Dimid`, basic dimension scales, hidden attribute filtering, and root-group metadata persistence.
- Continue to require explicit `NC_NEXTCDF4` selection and keep all changes within NEP; do not modify netcdf-c or its built-in HDF5 backend.
- Use only public HDF5 APIs and NEP/netcdf-c internal metadata helpers; do not copy netcdf-c `libhdf5` code.
- Restrict variable I/O to the fixed-size standard atomic types supported in Sprint 3. `NC_STRING`, user-defined types, and all NEXTCDF-4-specific types remain later work.
- Preserve `NC_NOWRITE`, define-mode, compatibility-mode, and type restrictions with standard NetCDF error codes.
- Honor the classic model and `NC_NETCDF4_MODEL` storage restrictions defined in the NEXTCDF-4 plan.

**Dimension scale and coordinate architecture:**
- After defining a variable in `nc_def_var`, attach each dimension's existing HDF5 dimension-scale dataset to the variable's HDF5 dataset using the appropriate dimension-scale API.
- Maintain the `DIMENSION_LIST` attribute on the variable and the `REFERENCE_LIST` attribute on each used dimension scale to preserve standard HDF5 dimension-scale semantics.
- Write `_Netcdf4Coordinates` on every variable as a little-endian integer array of the variable's `dimids`, enabling fast variable-to-dimension mapping on reopen.
- When opening a file, prefer `_Netcdf4Coordinates` for variable-to-dimension recovery; retain the dimension-scale matching path as a fallback for files without the hidden attribute.
- Treat a variable whose name matches a dimension and whose shape is one dimension of that length as the coordinate variable for that dimension. The coordinate variable's data and the dimension-scale dataset share the same HDF5 dataset, not a separate one.
- For unlimited coordinate variables, ensure the dimension scale and coordinate dataset extend together when record data is appended.

**Variable I/O architecture:**
- Implement contiguous and chunked HDF5 dataset access in new or extended source modules. Fixed-size variables with no unlimited dimensions use contiguous storage; variables with at least one unlimited dimension use chunked storage with default chunk sizes.
- Implement memory-to-disk and disk-to-memory type conversion using the supported netcdf-c helper for the standard atomic types.
- Implement scalar, one-element, hyperslab, and strided access through a common hyperslab selection helper that maps NetCDF `start`/`count`/`stride` arrays to HDF5 dataspace selections.
- For unlimited variables, support appending record data by extending the dataset along the unlimited dimension. Validate that writes stay within the current+extended shape and that reads do not exceed the current extent.
- Update `sync`, `close`, and `open` so dimension-scale attachments, `_Netcdf4Coordinates`, and dataset extensions are materialized and flushed. Ensure partial-write and partial-load failures release every HDF5 identifier and netcdf-c metadata allocation.

**Dispatch and API coverage:**
- Implement `get_vara` and `put_vara` for all supported standard atomic types, scalar and array variables, and unlimited-dimension extension.
- Implement `get_var1` and `put_var1` as single-element `get_vara`/`put_vara` calls.
- Implement `get_var` and `put_var` as full-variable reads and writes using the variable's current shape.
- Implement `get_vars` and `put_vars` for strided hyperslab access. `get_varm` and `put_varm` remain unimplemented and return `NC_ENOTBUILT`.
- Update `inq_var_all` so `contiguousp`, `no_fill`, `fill_valuep`, and `endiannessp` reflect actual dataset storage when applicable.
- Ensure all read and write calls enforce `NC_NOWRITE`, `NC_EBADID`, `NC_EINVAL` for malformed hyperslabs, `NC_EEDGE` for out-of-bounds accesses, and `NC_EBADTYPE` for unsupported in-memory types.
- Keep unsupported variable controls (chunking, compression, filters, checksums, endianness, quantization) wired to explicit `NC_ENOTBUILT` or `NC_ENOTNC4` errors as appropriate.

**Implementation sequence:**
1. Add the dimension-scale attachment step to `nc_def_var`, write `_Netcdf4Coordinates`, `DIMENSION_LIST`, and `REFERENCE_LIST`, and add focused tests verifying the attachments persist after close/reopen.
2. Implement a shared HDF5 dataspace selection and I/O helper for `vara` access; cover scalar, fixed-shape, and unlimited-dimension cases.
3. Implement `put_vara`/`get_vara` and the generic `put_var`/`get_var` wrappers for supported atomic types, including memory type conversion and fill-value handling.
4. Implement `put_var1`/`get_var1` as single-element wrappers.
5. Implement `put_vars`/`get_vars` for strided access using the same selection helper.
6. Implement unlimited-dimension dataset extension and coordinate-variable extension, with tests for append and reopen.
7. Wire the completed I/O callbacks into the dispatch table, keep `put_varm`/`get_varm` as `NC_ENOTBUILT` stubs, and expand direct-registration and autoload test coverage.
8. Update NEXTCDF-4 design and user documentation to describe the supported I/O subset, the deprecated `varm` calls, and the coordinate-variable behavior.

**Verification and acceptance criteria:**
- Scalar, fixed-array, and unlimited variables can be written and read back with matching values for all fixed-size standard atomic types.
- `nc_put_var1`/`nc_get_var1` write and read a single element; `nc_put_vara`/`nc_get_vara` write and read arbitrary contiguous hyperslabs; `nc_put_vars`/`nc_get_vars` write and read strided selections.
- `nc_put_varm`/`nc_get_varm` return `NC_ENOTBUILT` with a documented deprecation note.
- Variables with multiple dimensions use the correct `start`/`count`/`stride` ordering; out-of-bounds and malformed selections return `NC_EEDGE` or `NC_EINVAL` without writing partial data.
- Unlimited variables can be extended by writing beyond the current length; `nc_inq_dimlen` reports the new length; reopening the file preserves and reports the extended length.
- Coordinate variables share the dimension-scale dataset; reading and writing the coordinate variable updates the dimension values, and reopening identifies the variable as the coordinate for that dimension.
- Direct HDF5 inspection confirms `DIMENSION_LIST` on variables, `REFERENCE_LIST` on dimension scales, `_Netcdf4Coordinates` on every variable, and `_Netcdf4Dimid` on every dimension scale.
- `nc_get_var`/`nc_put_var` round-trip all supported atomic types for scalar, fixed, and unlimited variables.
- Read-only, non-existent variable, bad ID, and unsupported type cases return stable errors without leaking HDF5 identifiers.
- Direct initialization and `.ncrc` autoload run the same variable I/O tests, and a clean NEXTCDF-4-enabled build passes the full C test suite.

**Out of scope for Sprint 4:**
- `nc_put_varm`/`nc_get_varm` are intentionally unimplemented because they are deprecated in NEXTCDF-4.
- User-defined types, `NC_STRING`, compound, enum, opaque, vlen, and all NEXTCDF-4-specific types; these belong to Sprints 5, 9, and 10.
- Chunking controls, compression, filters, checksums, quantization, endianness controls, and detailed fill behavior beyond default fill values; these belong to Sprint 6.
- General discovery of legacy or arbitrary upstream NetCDF-4/HDF5 layouts beyond the populated subset written by NEXTCDF-4; broader interoperability belongs to Sprint 7.
- Dimension and variable rename behavior, which remains Sprint 8 work.
- New floating-point, complex, bitfield, and reference types, tools, language bindings, parallel I/O, diskless/in-memory files, and non-default HDF5 virtual file drivers.

#### Sprint 5: Add Groups and User-Defined Types
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "Correct Renaming of Dims and Vars" for group/type interactions, "Backward Compatibility," the type-mapping summary, and the enhanced-data-model contract.

**Objective:** Extend NEXTCDF-4 from a single root group to the full enhanced NetCDF-4 data model with arbitrarily nested groups, plus user-defined types and string variables. Implement definition, inquiry, attribute, and basic variable I/O for `NC_STRING` and for compound, enum, opaque, and vlen types, while enforcing classic-model and `NC_NETCDF4_MODEL` restrictions.

**Scope decisions:**
- Support arbitrary-depth nested groups, including group creation, parent/child inquiry, full-name and full-path lookup, and per-group namespaces for dimensions, variables, types, attributes, and subgroups.
- Implement all user-defined types that are already part of the classic enhanced NetCDF-4 model: compound, enum, opaque, and vlen, plus the base `NC_STRING` variable-length string type. NEXTCDF-4-specific types (float16 variants, complex, bitfield, reference) remain Sprints 9 and 10.
- Support both fixed-size `NC_CHAR` arrays and true variable-length `NC_STRING` strings; `NC_STRING` is represented with HDF5 variable-length `H5T_C_S1` and is forbidden in classic model and `NC_NETCDF4_MODEL` mode.
- Support basic read and write for variables of all types introduced in this sprint, including scalar, fixed-array, and unlimited-dimension access, using the existing hyperslab I/O machinery. Compound, enum, opaque, vlen, and string attribute I/O is also included.

**Prerequisites and constraints:**
- Build on the Sprint 4 metadata and I/O foundation: stable IDs, in-memory `NC_FILE_INFO_T`/`NC_GRP_INFO_T`/`NC_DIM_INFO_T`/`NC_VAR_INFO_T`/`NC_ATT_INFO_T` trees, dimension scales, `_Netcdf4Coordinates`, hidden attribute filtering, and atomic type I/O.
- Continue to require explicit `NC_NEXTCDF4` selection and keep all changes within NEP; do not modify netcdf-c or its built-in HDF5 backend.
- Use only public HDF5 APIs and the existing NEP/netcdf-c internal metadata helpers; do not copy netcdf-c `libhdf5` code.
- Enforce the classic model and `NC_NETCDF4_MODEL` restrictions: no nested groups, no user-defined types other than those allowed, no `NC_STRING`, and only classic atomic variables in `NC_CLASSIC_MODEL`; in `NC_NETCDF4_MODEL` no user-defined types and no `NC_STRING`.
- Preserve stable object IDs across dimensions, variables, groups, and types so that reopening reconstructs a consistent namespace.

**Group architecture:**
- Move from a single hard-coded root group to a recursive `NC_GRP_INFO_T` tree under `NC_FILE_INFO_T`/`root_grp`.
- Implement `def_grp` to create an HDF5 subgroup and a matching `NC_GRP_INFO_T` child, allocating stable group IDs and recording parent/child relationships.
- Implement group lookup by full name, parent group, direct and recursive child enumeration, `ncid` derivation, and `inq_ncid`/`inq_grps`/`inq_grpname`/`inq_grpname_full`/`inq_grp_parent`/`inq_grp_full_ncid`.
- Distinguish group-local dimensions and variables from those in parent or sibling groups; dimension IDs remain file-scoped and must be resolvable from any group that contains them.
- Keep group-hidden attributes (`_Nextcdf4Backend`, `_Nextcdf4Model`) on the root group and add a group-marker attribute only if needed for fast root-group identification of subgroups.
- Maintain a single `_Nextcdf4VarDimids` per variable and `_Nextcdf4Dimid` per dimension scale regardless of the group that owns them.

**User-defined type architecture:**
- Add a type-creation and commit pipeline for compound, enum, opaque, and vlen, storing each committed HDF5 type inside its owning group and registering it in the group `NC_TYPE_INFO_T` index.
- Track type references (variables and attributes) so that types cannot be deleted while in use; for this sprint, committed types are immutable after creation.
- Implement compound member insertion with fixed-size atomic base types and, later, nested compounds. Keep the `r`/`i` structural rule for `NC_COMPLEX` detection out of this sprint; plain compounds are created with user-supplied member names.
- Map enum base types to the fixed-size signed and unsigned integers supported by the active model; store values in little-endian committed HDF5 `H5T_ENUM` datatypes.
- Map opaque types to `H5T_OPAQUE` with the declared size; provide round-trip `put`/`get` as raw bytes.
- Map vlen types to `H5T_VLEN` over a fixed-size atomic or `NC_CHAR`/`NC_STRING` base type; support the standard `put`/`get` vlen element helper APIs.
- Map `NC_STRING` to HDF5 variable-length `H5T_C_S1` with native memory layout `hvl_t`.
- Support attribute read/write for all new types, including compound, enum, opaque, vlen, and `NC_STRING`.
- Load all committed types before variables when opening a file so that variable and attribute datatypes can be resolved during variable discovery.

**Variable I/O extension:**
- Extend the existing `var_io` helper to dispatch on `var->type_info->nc_type_class` and, for user-defined types, to use the committed HDF5 type as the file datatype and a correctly shaped memory buffer.
- For compound and opaque variables, treat the memory buffer as raw bytes of the declared size and rely on HDF5 for endian conversion.
- For enum variables, support read and write using the enum's base type memory buffer; conversions between the numeric base and the enum are not yet required to be lossy-checked.
- For vlen and `NC_STRING` variables, use `hvl_t` or `char **` memory buffers, respectively, and free all returned elements on close or after `nc_free_vlens`.
- Extend coordinate-variable detection and dimension-scale attachment to handle variables whose dimensions are in the same group or an ancestor group.
- Keep `nc_put_varm`/`nc_get_varm` unimplemented and deprecated.

**Dispatch and API coverage:**
- Implement `def_grp`, `inq_ncid`, `inq_grps`, `inq_grpname`, `inq_grpname_full`, `inq_grp_parent`, and `inq_grp_full_ncid`.
- Implement `inq_typeids`, `inq_user_type`, `inq_typeid`, `inq_type_equal`, `inq_compound_field`, and `inq_compound_fieldindex`.
- Implement `def_compound`, `insert_compound`, `insert_array_compound`, `def_vlen`, `put_vlen_element`, `get_vlen_element`, `def_enum`, `insert_enum`, `inq_enum_member`, `inq_enum_ident`, and `def_opaque`.
- Update `def_var`, `put_att`, `get_att`, and `inq_var_all` so they accept and report the new types and groups correctly.
- Keep `NC_COMPLEX`/`NC_DOUBLECOMPLEX`, `NC_BITFIELD*`, `NC_FLOAT16`, and reference types as `NC_ENOTBUILT` or `NC_EBADTYPE` until Sprints 9 and 10.

**Implementation sequence:**
1. Add recursive group creation and discovery to `nxt4meta.c` or a new `nxt4group.c`, including HDF5 group link creation, parent-child indexing, and stable group-ID allocation.
2. Extend dimension and variable lookup to support group-local and ancestor-group dimension resolution, and add tests for group scoping.
3. Add a type-commitment module for compound, enum, opaque, and vlen types, and extend `NC_TYPE_INFO_T` ownership, reference counting, and on-disk committed-type naming.
4. Extend `NEXTCDF4_map_hdf_type` and `set_var_type` to recognize the new base and user-defined types and load committed HDF5 types on open.
5. Extend the attribute and variable I/O paths to support `NC_STRING` and the user-defined types with appropriate memory layouts.
6. Update the dispatch table, group/type inquiry callbacks, and classic/enhanced compatibility checks.
7. Add a dedicated `tst_nextcdf4_group.c` and expand `tst_nextcdf4_meta.c` with type and string round-trip tests.
8. Update NEXTCDF-4 design and user documentation to describe the new groups, types, and classic/compat restrictions.

**Verification and acceptance criteria:**
- Files with nested groups can be created, closed, reopened, and navigated using full names and `inq_grp*` functions; duplicate and invalid group names return the expected errors.
- Variables and dimensions defined in subgroups are resolved only from their owning and ancestor groups, and IDs remain stable across reopens.
- Compound, enum, opaque, vlen, and `NC_STRING` types can be defined, used in variables and attributes, and round-tripped with `put`/`get` for scalar, fixed, and unlimited cases.
- `NC_CHAR` multi-dimensional arrays continue to work as fixed-size character arrays, and `NC_STRING` works as variable-length strings.
- Classic-model files reject groups, `NC_STRING`, and all user-defined types; `NC_NETCDF4_MODEL` files reject `NC_STRING` and user-defined types.
- Type inquiry functions return correct names, sizes, base types, enum members, and compound field offsets and types.
- Direct HDF5 inspection confirms committed datatypes in the correct group, dimension scales attached to variables in subgroups, and `_Netcdf4Coordinates` present on all variables.
- A clean NEXTCDF-4-enabled build passes the full C test suite including the new group and type tests.

**Out of scope for Sprint 5:**
- The `NC_COMPLEX`/`NC_DOUBLECOMPLEX` structural reinterpretation, which belongs to Sprint 10.
- `NC_BITFIELD*` types, which belong to Sprint 10.
- `NC_FLOAT16` and other small floating-point types, which belong to Sprint 9.
- Reference types and `nc_ref_object`/`nc_ref_region`, which belong to Sprint 10.
- Chunking controls, compression, filters, checksums, quantization, endianness controls, and detailed fill behavior beyond the defaults, which belong to Sprint 6.
- Dimension and variable renaming, which remains Sprint 8 work.
- General discovery of legacy or arbitrary upstream NetCDF-4/HDF5 layouts beyond the populated subset written by NEXTCDF-4; broader interoperability belongs to Sprint 7.
- Tools, language bindings, parallel I/O, diskless/in-memory files, and non-default HDF5 virtual file drivers.

#### Sprint 6: Add Variable Storage Features
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially the "Variable I/O and chunking" section, the type-mapping summary, and the compatibility rules for `NC_NETCDF4_MODEL`.

**Objective:** Implement chunking, fill behavior, compression, filters, endianness, checksums, quantization, and related variable creation properties. Cover dataset extension and synchronization behavior needed for production use of unlimited dimensions and chunked data.

**Scope decisions:**
- Support both `NC_CHUNKED` (with caller-supplied or auto-computed chunk shapes) and `NC_CONTIGUOUS` storage. Default to chunked for variables with unlimited dimensions and to contiguous for small fixed-size variables unless the caller overrides.
- Implement `nc_def_var_chunking`, `nc_inq_var_chunking`, and the chunk-inquiry helpers.
- Implement fill mode and fill values for all current types: classic atomic, `NC_STRING`, compound, enum, opaque, and vlen. Support `nc_def_var_fill`, `nc_set_fill`, `nc_inq_var_fill`, and `NC_NOFILL`.
- Implement compression and checksum filters: `nc_def_var_deflate` (zlib level 0-9 plus optional shuffle), `nc_def_var_fletcher32`, and `nc_def_var_filter` for registering HDF5 filter IDs that the linked HDF5 supports.
- Implement `nc_def_var_endian` with `NC_ENDIAN_LITTLE`, `NC_ENDIAN_BIG`, and `NC_ENDIAN_NATIVE`.
- Implement `nc_def_var_quantize` with `NC_QUANTIZE_BITGROOM`, `NC_QUANTIZE_GRANULARBR`, and `NC_QUANTIZE_BITROUND` for numeric variables.
- Keep `nc_def_var_szip`, `nc_def_var_zstandard`, `nc_def_var_bzip2`, and other third-party filters out of this sprint unless they are already available as HDF5 plugins.
- Maintain `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` restrictions: no enhanced features in classic; `NC_NETCDF4_MODEL` must only use filter/chunking/fill settings that upstream netcdf-c can read back.

**Prerequisites and constraints:**
- Build on the Sprint 5 group/type and Sprint 4 I/O foundations: stable object IDs, recursive group metadata, type info, and the `NC_VAR_INFO_T`/`NEXTCDF4_VAR_INFO_T` structures.
- Use HDF5 Dataset Creation Property Lists (DCPL) and Dataset Access Property Lists (DAPL) only through public HDF5 APIs.
- Keep the existing `NC_NEXTCDF4` opt-in and do not modify netcdf-c or the built-in HDF5 backend.
- Preserve round-trip compatibility for `NC_NETCDF4_MODEL` files; write standard HDF5 chunking, fill, deflate, shuffle, and fletcher32 attributes.
- Ensure `nc_sync`, `nc_enddef`, and write operations correctly extend unlimited dimensions and flush chunked datasets.

**Variable storage architecture:**
- Introduce `src/nextcdf4/nxt4var.c` (or extend `nxt4io.c`) to centralize variable creation, DCPL construction, and the `def_var_chunking/deflate/fill/endian/fletcher32/quantize` dispatch functions.
- Build the DCPL with: chunk shape, fill value (`H5Pset_fill_value` or `NC_NOFILL`), deflate+shuffle filter (`H5Pset_deflate`/`H5Pset_shuffle`), fletcher32 (`H5Pset_fletcher32`), endianness (`H5Tset_order` on the file type), and quantization state.
- Store the file type with the chosen endianness in `var->type_info`/`format_type_info`; preserve the memory type as native.
- Apply quantization during write: round floating-point or integer data to the requested number of significant bits before calling `H5Dwrite`; on read, return the quantized (stored) values.
- Load chunking, fill, filter, endian, and quantization metadata when opening an existing file by reading the DCPL and dataset attributes; update `NC_VAR_INFO_T` accordingly.
- Continue to support `nc_put_vara`/`nc_get_vara`/`nc_put_vars`/`nc_get_vars` for chunked and unlimited variables; ensure `H5Dset_extent` is called when writing beyond the current unlimited dimension length.

**Dispatch and API coverage:**
- `nc_def_var_chunking`, `nc_inq_var_chunking`
- `nc_def_var_deflate`, `nc_inq_var_deflate`
- `nc_def_var_fill`, `nc_inq_var_fill`, `nc_set_fill`
- `nc_def_var_endian`, `nc_inq_var_endian`
- `nc_def_var_fletcher32`, `nc_inq_var_fletcher32`
- `nc_def_var_quantize`, `nc_inq_var_quantize`
- `nc_sync`, `nc__enddef`, and `nc_redef` already exist; extend them to flush/create datasets and extend unlimited dimensions for new variables.

**Implementation sequence:**
1. Add `nxt4var.c` skeleton and move existing `NEXTCDF4_def_var`/`var_create_dataset` logic into it or from `nxt4io.c`.
2. Implement DCPL builders for chunking, fill, deflate+shuffle, fletcher32, endian, and quantization.
3. Hook the DCPL into `NEXTCDF4_def_var` and `NEXTCDF4__enddef`.
4. Extend the I/O path to apply quantization before writing and to extend unlimited dimensions before `H5Dwrite`.
5. Update `NEXTCDF4_load_metadata` to read DCPL properties and reconstruct chunking/fill/filter/endian/quantize state.
6. Add `test/tst_nextcdf4_chunking.c` with scalar, fixed, unlimited, chunked, compressed, checksummed, big-endian, and quantized round-trip tests.
7. Update `docs/roadmap.md` and `NEXTCDF4_plan.md` to reflect the new capabilities.
8. Audit and bring all Doxygen comments in `src/nextcdf4/` up to date with the current implementation, and make Doxygen maintenance a required step in every future sprint.

**Verification and acceptance criteria:**
- Chunked, contiguous, and auto-chunked variables can be created, written, read, and reopened.
- Deflate/shuffle/fletcher32 round-trips produce the same data and report correct filter state.
- Fill values work for atomic, `NC_STRING`, compound, enum, opaque, and vlen variables; `NC_NOFILL` disables fill.
- `nc_def_var_endian` writes big-endian or little-endian datasets and reads them back as native.
- `nc_def_var_quantize` stores the specified number of significant bits and reads back the quantized values.
- Unlimited chunked variables can be extended and flushed with `nc_sync`/`nc_enddef`.
- `NC_CLASSIC_MODEL` rejects all enhanced storage calls; `NC_NETCDF4_MODEL` only uses upstream-compatible features and the resulting files are readable by netcdf-c.
- A clean NEXTCDF-4 build passes the full C test suite including the new chunking tests.

**Sprint 6 status:**
|- Implementation is complete in `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4io.c`, `src/nextcdf4/nxt4dispatch.c`, and `src/nextcdf4/nxt4internal.h`.
|- `test/tst_nextcdf4_chunking.c` covers `nc_def_var_chunking`, `nc_def_var_deflate`, `nc_def_var_fletcher32`, `nc_def_var_fill`, `nc_def_var_endian`, `nc_def_var_quantize`, and their `nc_inq_var_*` counterparts, including close/reopen round-trips for chunking, filters, fill, and endian settings.
|- Quantization is applied to floating-point data before `H5Dwrite` using `nc4_convert_type`.
|- DCPL properties (chunking, fill, deflate/shuffle, fletcher32, endianness) are read back from the HDF5 dataset when an existing file is opened.
|- The NEXTCDF-4 C test suite passes; the remaining `run_fortran_tests` segfault is unrelated to the NEXTCDF-4 C backend.

**Out of scope for Sprint 6:**
- Szip, Zstandard, BZIP2, LZ4, and other third-party filters (except through `nc_def_var_filter` if already registered).
- Parallel I/O, advanced chunk cache tuning, and non-default HDF5 virtual file drivers.
- Chunking/fill/filter discovery in arbitrarily legacy files not created by NEXTCDF-4 (Sprint 7).
- Rename of chunked/compressed variables (Sprint 8).
- Float16, complex, bitfield, reference types (Sprints 9 and 10).

#### Sprint 7: Open Existing and Populated Files
**Objective:** Expand metadata discovery so NEXTCDF-4 can open populated files produced by both NEXTCDF-4 and the upstream NetCDF-4/HDF5 backend. Support hidden-coordinate fast paths, dimension-scale reference fallback, and the compatibility rules for native, classic-model, and `NC_NETCDF4_MODEL` files. Reading from legacy and third-party HDF5 layouts remains out of scope.

**Scope decisions:**
- `nc_open(..., NC_NEXTCDF4 | NC_NOWRITE)` must be able to open any file previously written by `NC_NEXTCDF4` and any standard `NC_NETCDF4` file produced by the upstream netcdf-c HDF5 backend.
- Autoload behavior (`.ncrc` / `NCRCENV_RC` magic matching) must also route these files to the NEXTCDF-4 backend when the user has not explicitly passed `NC_NEXTCDF4`.
- All groups, dimensions, variables, attributes, and user-defined types reachable from the root must be reconstructed in the same `NC_FILE_INFO_T`/`NC_GRP_INFO_T` hierarchy used by the rest of the backend.
- Dimension-to-variable mapping must first use the `_Netcdf4Coordinates` integer array (the `_Netcdf4Dimid` fast path already used for dimensions) for O(1) reconstruction.
- If `_Netcdf4Coordinates` is absent, fall back to the HDF5 `DIMENSION_LIST` / `REFERENCE_LIST` dimension-scale reference attributes and resolve them to `NC_DIM_INFO_T` entries.
- If neither hidden attribute is present, fall back to matching dimension scales by `NAME` and `CLASS` attributes and by the dimension length, tolerating minor naming/ordering differences where the match is unambiguous.
- Variable datatypes, shapes, chunking, fill, filters, endianness, and quantization state must be recovered from the DCPL and dataset attributes (reusing the Sprint 6 DCPL reader).
- Opened files are read-only with respect to legacy HDF5 layout details: rewriting or re-chunking files not created by NEXTCDF-4 is out of scope.

**Prerequisites and constraints:**
- Build on Sprint 5 recursive group/type loading and Sprint 6 DCPL recovery.
- Use only public HDF5 and HDF5 high-level APIs (`H5Gopen2`, `H5DSget_num_scales`, `H5DSget_scale_name`, `H5Aopen`, `H5Dget_create_plist`, etc.).
- Keep `NC_NEXTCDF4` opt-in and do not modify the upstream netcdf-c or built-in HDF5 backend.
- Preserve `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` restrictions when opening files: reject classic-model files that contain unsupported enhanced features, and allow only features that upstream netcdf-c can read back for `NC_NETCDF4_MODEL` files.

**Metadata discovery architecture:**
- Refactor `load_dimensions` and `load_variables` to accept an `NC_GRP_INFO_T *` (and its `hdf_group`) so that every group loads its own dimensions and variables before child groups are processed.
- Update `load_one_dim` to read existing HDF5 dimension-scale datasets, extract `_Netcdf4Dimid`, `CLASS`, and `NAME` attributes, and determine unlimited status from the dataspace max extent.
- Update `load_one_var` to open the variable dataset in the correct group, read `_Netcdf4Coordinates` if available, and otherwise resolve dimensions through `DIMENSION_LIST` references or by scale-name matching.
- Introduce a `resolve_var_dimids` helper that maps a variable's dimension-scale references or scale names to the `dimids` already loaded into the parent `NC_GRP_INFO_T`.
- Ensure `load_global_attributes` and variable attribute loading run after all dimensions and variables for the group have been created, so that user attributes attach to the right container.
- Update `NEXTCDF4_load_metadata` to walk the group tree recursively: load dimensions, variables, and attributes for the root, then repeat for each child group loaded by `load_children`.

**Dispatch and API coverage:**
- `NEXTCDF4_open` is the entry point; the work is in `NEXTCDF4_load_metadata` and the per-group loaders.
- `nc_open` with `NC_NEXTCDF4` and the autoload path must both route to the NEXTCDF-4 backend.
- `nc_inq_*`, `nc_get_var*`, and, for files NEXTCDF-4 created, `nc_put_var*` must work on opened files without redefinition errors.

**Implementation sequence:**
1. Refactor `load_dimensions`/`load_variables` to take a group pointer and operate on `grp->format_grp_info->hdf_group`.
2. Update `load_one_var` to accept the target group and its HDF5 group, and to read `_Netcdf4Coordinates` as the primary dimid source.
3. Add a `DIMENSION_LIST` / `REFERENCE_LIST` reader and a dimension-scale reference resolver.
4. Add a scale-name fallback for files that only expose `CLASS`/`NAME` dimension-scale attributes.
5. Update `load_one_dim` to handle unlimited dimension discovery from existing scale datasets and to validate `_Netcdf4Dimid`.
6. Make `NEXTCDF4_load_metadata` recurse into child groups in the correct dependency order (dims, vars, atts, children).
7. Add compatibility checks for `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` open paths.
8. Add `test/tst_nextcdf4_open.c` with cases for: (a) opening a NEXTCDF-4-written file without `NC_NEXTCDF4` (autoload), (b) opening an upstream `NC_NETCDF4` file with `NC_NEXTCDF4`, and (c) a file without `_Netcdf4Coordinates` that requires dimension-scale fallback.
9. Update `docs/roadmap.md` and `docs/plan/NEXTCDF4_plan.md` to reflect the new discovery capabilities.
10. Audit and update Doxygen comments in the modified `src/nextcdf4/` files.

**Verification and acceptance criteria:**
- A NEXTCDF-4-created file can be closed and reopened with `nc_open(..., NC_NEXTCDF4)` and all groups, dimensions, variables, attributes, and storage properties are recovered.
- The same file can be opened by autoload (no explicit format flag) when `.ncrc` / `NCRCENV_RC` points to the build `.ncrc`.
- An upstream `NC_NETCDF4` file with `_Netcdf4Coordinates` and `_Netcdf4Dimid` attributes opens and matches the original `nc_inq` results.
- A file with only `DIMENSION_LIST` / `REFERENCE_LIST` attributes opens and produces the same variable-to-dimension mapping.
- A file with only `CLASS`/`NAME` dimension scales opens and falls back to name-based matching.
- Data read back from an opened file (`nc_get_var_*`) matches the stored values for atomic types, chunked or contiguous variables, and unlimited variables.
- `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` compatibility checks reject unsupported files and allow supported ones.
- A clean NEXTCDF-4 build passes the full C test suite including the new open tests.

**Sprint 7 status:**
|- Implementation is complete in `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4file.c`, `src/nextcdf4/nxt4open.c`, `src/nextcdf4/nxt4internal.h`, `test/tst_nextcdf4_open.c`, and `test/CMakeLists.txt`.
|- `load_dimensions`, `load_variables`, `load_group_attributes`, and `load_group_metadata` now operate per-group and recurse through the group tree.
|- `load_one_var` resolves dimensions from `_Netcdf4Coordinates`, attached HDF5 dimension scales (`H5DS`), or `_Netcdf4Dimid`/`NAME` fallbacks.
|- `find_var_cb` and `find_dim_cb` distinguish NEXTCDF-4-created files, upstream `NC_NETCDF4` files, and arbitrary HDF5 datasets.
|- `NEXTCDF4_read_markers` treats a missing backend marker as an upstream NetCDF-4/HDF5 file while still rejecting arbitrary unmarked HDF5 files.
|- `test/tst_nextcdf4_open.c` covers reopening a NEXTCDF-4 file and opening an upstream `NC_NETCDF4` file via `NC_NEXTCDF4`.
|- All NEXTCDF-4 C tests pass; the unrelated `run_fortran_tests` segfault remains unchanged.

**Out of scope for Sprint 7:**
- Writing, re-chunking, or redefining files not originally created by NEXTCDF-4.
- Arbitrary non-NetCDF-4 HDF5 files that do not follow the dimension-scale convention.
- Dimension and variable renaming (Sprint 8).
- Float16, complex, bitfield, and reference types (Sprints 9 and 10).

#### Sprint 8: Correct Dimension and Variable Renaming
**Objective:** Implement atomic, flush-per-operation dimension and variable renaming. Keep in-memory metadata, HDF5 links, dimension-scale relationships, and hidden dimension-mapping attributes consistent across coordinate-variable and shared-dimension edge cases.

**Scope decisions:**
- `nc_rename_dim` and `nc_rename_var` with `NC_NEXTCDF4` must update the HDF5 link, the `NC_DIM_INFO_T`/`NC_VAR_INFO_T` names, and all indexes used for name-based lookup.
- A coordinate variable (a 1D variable whose name equals its dimension's name) shares the same HDF5 dataset with the dimension scale. Renaming one must rename the other and keep the `NAME` dimension-scale attribute in sync.
- A shared dimension used by many variables must only be renamed once; `DIMENSION_LIST`, `REFERENCE_LIST`, and `_Netcdf4Coordinates` references continue to work because they use object references or integer dimids.
- Renaming must work in define mode on a writable file. Read-only files and files in data mode must return `NC_EINVAL`.
- `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` restrictions are enforced by the existing checks; renaming is allowed as long as the resulting names are valid for the active model.

**Prerequisites and constraints:**
- Build on Sprint 6 DCPL and Sprint 7 metadata loading.
- Use public HDF5 APIs: `H5Lmove` for link renaming, `H5DSset_scale` for updating the scale `NAME` attribute, and `H5Aopen`/`H5Awrite` for hidden attributes if `H5DSset_scale` is insufficient.
- Only one HDF5 link move is required for a coordinate-variable/dimension pair.
- Do not rewrite dataset data or dimension-scale attachments; only the link name and the in-memory metadata change.
- Keep `NC_NEXTCDF4` opt-in and do not affect the upstream netcdf-c backend.

**Metadata and dispatch architecture:**
- Implement `NEXTCDF4_rename_dim` in `src/nextcdf4/nxt4dim.c` (or `nxt4meta.c` if a dim file does not yet exist). It finds the `NC_DIM_INFO_T`, validates the new name, moves the dimension-scale HDF5 link with `H5Lmove`, updates the `NAME` attribute with `H5DSset_scale`, updates the `NCindex` entry for dimensions, and updates the associated coordinate variable's `NC_VAR_INFO_T` name if present.
- Implement `NEXTCDF4_rename_var` in `src/nextcdf4/nxt4var.c` (or `nxt4meta.c`). It finds the `NC_VAR_INFO_T`, validates the new name, moves the variable HDF5 link with `H5Lmove`, updates the `NCindex` entry for variables, and updates the associated dimension and scale `NAME` attribute if the variable is a 1D coordinate variable.
- Add `NEXTCDF4_rename_dim` and `NEXTCDF4_rename_var` to the dispatch table in `nxt4dispatch.c` in place of the `NC_RO_*` fallbacks.
- Use `NC_DIM_INFO_T` to detect the coordinate-variable relationship: a 1D variable with one dimension whose name equals the variable name.
- Ensure `nc4_find_dim`/`nc4_find_var` still work after rename by re-inserting the object into the group dimension/variable index with the new name.

**Implementation sequence:**
1. Implement `NEXTCDF4_rename_dim`, including the `H5Lmove`, `H5DSset_scale`, in-memory name update, and coordinate-variable name sync.
2. Implement `NEXTCDF4_rename_var`, including the `H5Lmove`, in-memory name update, and coordinate-variable dimension rename.
3. Wire `NEXTCDF4_rename_dim` and `NEXTCDF4_rename_var` into the `NC_DISPATCH` table in `nxt4dispatch.c`.
4. Add `test/tst_nextcdf4_rename.c` covering: simple dimension rename, coordinate-variable dimension rename (one link move, both names update), variable rename that is also a coordinate variable (dimension also renames), rename of a shared dimension used by two variables, and reopen with verification.
5. Update `docs/roadmap.md` and `docs/plan/NEXTCDF4_plan.md` with the Sprint 8 details.
6. Run all `tst_nextcdf4_*` tests and the full C test suite.

**Verification and acceptance criteria:**
- A NEXTCDF-4 file can have a dimension renamed and the new name is returned by `nc_inq_dim` after `nc_enddef`.
- Renaming a coordinate variable (1D var with same name as its dim) also renames the underlying dimension.
- Renaming a dimension that has a coordinate variable also renames the variable.
- A shared dimension can be renamed without breaking `nc_inq_var` or `nc_get_var` for variables that use it.
- Reopening the file shows the new dimension and variable names.
- All NEXTCDF-4 C tests, including `tst_nextcdf4_rename`, pass.

**Sprint 8 status:**
- Implementation is complete in `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4internal.h`, `src/nextcdf4/nxt4dispatch.c`, `test/tst_nextcdf4_rename.c`, and `test/CMakeLists.txt`.
- `NEXTCDF4_rename_dim` and `NEXTCDF4_rename_var` are wired into the dispatch table and use `H5Lmove` and `H5DSset_scale` to rename HDF5 links and scale names.
- Both functions update in-memory `hdr.name` and rebuild the dimension/variable `NCindex` with `ncindexrebuild`.
- Coordinate variables (1D variables whose name matches their dimension) stay synchronized when either the dimension or the variable is renamed.
- `test/tst_nextcdf4_rename.c` covers simple dim rename, coordinate-variable rename in both directions, shared-dimension rename, and simple variable rename.
- All 9 `tst_nextcdf4_*` C tests pass; the pre-existing `run_fortran_tests` segfault is unchanged.

**Out of scope for Sprint 8:**
- Renaming user-defined types, groups, or attributes (attribute rename is already implemented).
- Renaming in files not created by NEXTCDF-4 or in `NC_NETCDF4_MODEL` files where it would violate the classic model.
- Re-chunking, redefining, or data movement as part of a rename.

#### Sprint 9: Add Small Floating-Point Types
**Objective:** Add `NC_FLOAT16` and, when supported by HDF5, bfloat16 and FP8/FP6/FP4 types. Define their public APIs, HDF5 mappings, conversion rules, fill-value behavior, compatibility restrictions, and round-trip coverage.

**Scope decisions:**
- Primary deliverable is `NC_FLOAT16` (IEEE 754 binary16). It maps to `H5T_IEEE_F16LE/BE` and is stored as 2-byte `uint16_t` in memory.
- If the build links HDF5 2.1.1 or later, also add `NC_BFLOAT16`, `NC_FLOAT8_E4M3`, `NC_FLOAT8_E5M2`, `NC_FLOAT6_E2M3`, `NC_FLOAT6_E3M2`, and `NC_FLOAT4_E2M1` with the mappings and memory types from the HDF5 2.x small-floating-point datatypes.
- Small floating-point types are atomic NetCDF types, not user-defined types. They use the same `nc_def_var`/`nc_put_vara`/`nc_get_vara` dispatch path as other atomic types.
- `NC_FLOAT16` and the other small floats are only allowed in native NEXTCDF-4 mode; they are rejected in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- They cannot be used as coordinate variables because they do not support a total ordering suitable for dimension coordinates.
- The canonical in-memory representation is the raw bit pattern (`uint16_t` for 16-bit, `uint8_t` for 8/6/4-bit types). Users are responsible for producing valid encodings; no automatic conversion to/from `float`/`double` is guaranteed for the non-IEEE formats.

**Prerequisites and constraints:**
- Build on `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, and `NEXTCDF4_check_atomic_type`.
- `NC_FLOAT16` requires HDF5 1.14.0 or later; the other small floats require HDF5 2.1.1 or later.
- The CMake/autotools checks must detect HDF5 version and the presence of the small-float predefined datatypes (`H5T_IEEE_F16LE`, `H5T_FLOAT_BFLOAT16LE`, etc.) before enabling them.
- `ncindex` and `NC_TYPE_INFO_T` do not need to change; the new types are atomic.

**API and type-model architecture:**
- Define `NC_FLOAT16` (and the optional small-float constants) in `include/nep.h` with values that do not collide with existing `nc_type` constants.
- Add `nc_put_vara_float16`/`nc_get_vara_float16` or document that callers use `nc_put_vara`/`nc_get_vara` with `memtype = NC_FLOAT16` and a `uint16_t` buffer.
- Extend `NEXTCDF4_map_hdf_type` to create `H5T_IEEE_F16LE/BE` (and the HDF5 2.x small-float types) for the new `nc_type` values.
- Extend `map_nc_type` in `nxt4meta.c` to recognize `H5T_FLOAT` with size 2 as `NC_FLOAT16`, and the HDF5 2.x small-float class/type pairs as the other new types.
- Extend `NEXTCDF4_type_size` and `NEXTCDF4_type_name` for each new type.
- Update `NEXTCDF4_check_atomic_type` to accept the new types only when `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL` are not set.
- Ensure `set_var_type` in `nxt4meta.c` accepts the new `nc_type` values and sets `var->nc_typeid` correctly.
- `var_io` in `nxt4io.c` must use the correct HDF5 memory and file types when the variable or `memtype` is one of the new small floats. For now, require `memtype` equal to the file type to avoid conversion edge cases.

**Implementation sequence:**
1. Add `NC_FLOAT16` constant to `include/nep.h` and guard the optional small-float constants with `HAVE_HDF5_2_1_1` or similar.
2. Update `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, and `NEXTCDF4_type_name` for `NC_FLOAT16`.
3. Update `NEXTCDF4_check_atomic_type` and `set_var_type` to permit `NC_FLOAT16`.
4. Add `test/tst_nextcdf4_float16.c` that creates a `NC_FLOAT16` variable, writes/reads `uint16_t` values with `nc_put_vara`/`nc_get_vara`, and verifies round-trip.
5. If HDF5 2.1.1 is available, repeat step 2-4 for bfloat16/FP8/FP6/FP4.
6. Add documentation notes in `docs/roadmap.md` and `docs/plan/NEXTCDF4_plan.md`.
7. Run the full `tst_nextcdf4_*` test suite and the full C test suite.

**Verification and acceptance criteria:**
- `nc_def_var` with `NC_FLOAT16` succeeds in native NEXTCDF-4 mode.
- `nc_put_vara` and `nc_get_vara` round-trip `uint16_t` values without corruption when `memtype == NC_FLOAT16 == var->nc_typeid`.
- `nc_inq_var` reports `xtype == NC_FLOAT16` for a float16 variable.
- `nc_def_var` with `NC_FLOAT16` fails with `NC_EBADTYPE` in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- Reopening a file with a float16 variable recovers the type correctly.
- (Optional, if HDF5 >= 2.1.1) bfloat16 and FP8/FP6/FP4 pass the same round-trip tests.
- All existing NEXTCDF-4 C tests continue to pass.

**Sprint 9 status:**
- Implementation is complete in `include/nep.h`, `src/nextcdf4/nxt4internal.h`, `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4io.c`, `test/tst_nextcdf4_float16.c`, and `test/CMakeLists.txt`.
- `NC_FLOAT16` and all HDF5 2.1.1 small floats (`NC_BFLOAT16`, `NC_FLOAT8_E4M3`, `NC_FLOAT8_E5M2`, `NC_FLOAT6_E2M3`, `NC_FLOAT6_E3M2`, `NC_FLOAT4_E2M1`) are defined and round-trip through `nc_def_var`, `nc_put_vara`, and `nc_get_vara`.
- `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, `NEXTCDF4_type_name`, `NEXTCDF4_check_atomic_type`, `set_var_type`, and `nxt4io.c` `memory_type` all handle the new types.
- `test/tst_nextcdf4_float16.c` covers round-trip for all seven types and rejection in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- All 10 `tst_nextcdf4_*` C tests pass; the pre-existing `run_fortran_tests` segfault is unchanged.

**Out of scope for Sprint 9:**
- Automatic conversion between `NC_FLOAT16`/small floats and `NC_FLOAT`/`NC_DOUBLE`; users must supply the correct in-memory type.
- Complex numbers, bitfields, and reference types (Sprint 10).
- Coordinate variables and unlimited dimensions using small floats.
- Support for HDF5 versions older than 1.14.0.

#### Sprint 10: Add Complex, Bitfield, and Reference Types
**Objective:** Add complex-number compounds, fixed-width HDF5 bitfields, and object and region references. Include structural type detection, reference creation and dereference APIs, validity checks, storage restrictions, and interoperability with existing HDF5 files.

**Scope decisions:**
- Add `NC_COMPLEX` (single-precision complex) and `NC_DOUBLECOMPLEX` (double-precision complex) as built-in compound-like atomic types. They map to HDF5 compound types `{ float r; float i; }` and `{ double r; double i; }`. In-memory layout is a `float[2]` or `double[2]` where the first element is the real part and the second is the imaginary part.
- Add `NC_BITFIELD8`, `NC_BITFIELD16`, `NC_BITFIELD32`, and `NC_BITFIELD64` as built-in atomic types. They map to `H5T_STD_B*LE/BE` bitfield datatypes and are held in memory as `uint8_t`, `uint16_t`, `uint32_t`, and `uint64_t`. The NetCDF API treats them as unsigned integers; bit decoding is the caller's responsibility.
- Add `NC_REF_OBJECT` and `NC_REF_REGION` as opaque reference types. They map to `H5T_STD_REF_OBJ` and `H5T_STD_REF_DSETREG`. Variables of these types store HDF5 reference tokens as opaque byte arrays. No dereferencing API is exposed in this sprint; callers read and write the opaque tokens.
- All new types are rejected in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL` because they are not part of the classic NetCDF data model.
- Complex and bitfield variables may be used as regular variables. Reference-typed variables cannot be coordinate variables, cannot appear inside compounds or attributes, and cannot be copied meaningfully between files.

**Prerequisites and constraints:**
- Build on `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, `NEXTCDF4_type_name`, `NEXTCDF4_check_atomic_type`, `set_var_type`, and `nxt4io.c` `memory_type` from Sprints 8 and 9.
- `NC_TYPE_INFO_T` for the new atomic types must carry the right `nc_type_class` (`NC_COMPOUND` for complex, `NC_INT` for bitfields, `NC_OPAQUE` for references) without being confused with user-defined types.
- Complex detection on read relies on `H5T_COMPOUND` with exactly two members named `r` and `i` whose base types are `H5T_FLOAT`/size 4 or `H5T_FLOAT`/size 8. Bitfield detection uses `H5Tget_class == H5T_BITFIELD` and size 1/2/4/8. Reference detection uses `H5Tget_class == H5T_REFERENCE` and `H5Tget_ref_type`.
- Reference variables must be read and written as whole arrays to avoid invalid token states; per-element hyperslab writes are allowed only within the same file.

**API and type-model architecture:**
- Define `NC_COMPLEX`, `NC_DOUBLECOMPLEX`, `NC_BITFIELD8`, `NC_BITFIELD16`, `NC_BITFIELD32`, `NC_BITFIELD64`, `NC_REF_OBJECT`, and `NC_REF_REGION` in `include/nep.h`.
- Extend `NEXTCDF4_map_hdf_type` to create:
  - `NC_COMPLEX`/`NC_DOUBLECOMPLEX` as `H5T_COMPOUND` with two `r`/`i` members.
  - `NC_BITFIELD*` as `H5T_STD_B*LE`.
  - `NC_REF_OBJECT` as `H5T_STD_REF_OBJ` and `NC_REF_REGION` as `H5T_STD_REF_DSETREG`.
- Extend `map_nc_type` to detect and map the corresponding HDF5 datatypes back to the new `nc_type` values.
- Extend `NEXTCDF4_type_size` and `NEXTCDF4_type_name` for each new type.
- Update `NEXTCDF4_check_atomic_type` to allow the new types only in native NEXTCDF-4 mode.
- Update `set_var_type` so `NC_COMPLEX`/`NC_DOUBLECOMPLEX` are recorded as compound-class types, `NC_BITFIELD*` as integer-class types, and `NC_REF_*` as opaque-class types.
- Extend `nxt4io.c` `memory_type` to return the matching in-memory HDF5 datatype (`H5T_NATIVE_FLOAT_COMPLEX`/`H5T_NATIVE_DOUBLE_COMPLEX` for complex, `H5T_NATIVE_B*` for bitfields, and a copy of `H5T_STD_REF_OBJ`/`H5T_STD_REF_DSETREG` for references).
- Ensure `var_io` performs whole-slab I/O for references and requires `memtype` to match the file type for the other new types to avoid unsupported HDF5 conversions.

**Implementation sequence:**
1. Add the new `nc_type` constants to `include/nep.h`.
2. Update `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, `NEXTCDF4_type_name`, and `NEXTCDF4_check_atomic_type` for complex, bitfield, and reference types.
3. Update `set_var_type` to set the correct `nc_type_class` and minimal `NC_TYPE_INFO_T` state for the new types.
4. Update `nxt4io.c` `memory_type`.
5. Add `test/tst_nextcdf4_complex.c` for complex round-trip and `nc_inq_var` type recovery.
6. Add `test/tst_nextcdf4_bitfield.c` for bitfield round-trip.
7. Add `test/tst_nextcdf4_ref.c` for reference-type write/read as opaque arrays.
8. Update `test/CMakeLists.txt` to build and register the three new tests.
9. Update `docs/roadmap.md` and `docs/plan/NEXTCDF4_plan.md`.
10. Run the `tst_nextcdf4_*` test suite.

**Verification and acceptance criteria:**
- `nc_def_var` with `NC_COMPLEX`/`NC_DOUBLECOMPLEX` succeeds and round-trips `float[2]`/`double[2]` data using `nc_put_vara` and `nc_get_vara`.
- `nc_inq_var` reports the correct `nc_type` for a variable of each new type.
- Bitfield variables round-trip raw unsigned integer patterns.
- Reference-typed variables can be written and read back as opaque byte arrays in the same file.
- `nc_def_var` with each new type fails with `NC_EBADTYPE` or `NC_ENOTNC4` in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- Reopening a file with the new types recovers the type information correctly.
- All existing `tst_nextcdf4_*` C tests continue to pass.

**Sprint 10 status:**
- Implementation is complete in `include/nep.h`, `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4io.c`, `test/tst_nextcdf4_complex.c`, `test/tst_nextcdf4_bitfield.c`, `test/tst_nextcdf4_ref.c`, and `test/CMakeLists.txt`.
- `NC_COMPLEX` and `NC_DOUBLECOMPLEX` round-trip using the native HDF5 2.1.1 complex datatypes (`H5T_COMPLEX_IEEE_F32LE` and `H5T_COMPLEX_IEEE_F64LE`).
- `NC_BITFIELD8/16/32/64` round-trip as raw unsigned integer bit patterns using `H5T_STD_B*LE` and `H5T_NATIVE_B*`.
- `NC_REF_OBJECT` and `NC_REF_REGION` round-trip as opaque `hobj_ref_t`/`hdset_reg_ref_t` tokens using `H5T_STD_REF_OBJ`/`H5T_STD_REF_DSETREG`.
- All new types are rejected in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- All 13 `tst_nextcdf4_*` C tests pass; the pre-existing `run_fortran_tests` segfault is unchanged.

**Out of scope for Sprint 10:**
- Dereferencing object or region references through the NetCDF API (e.g., `H5Rcreate`, `H5Rdereference`, `H5Rget_obj_type3`).
- Reference types inside user-defined compounds, enums, vlen, or attributes.
- Typed convenience functions such as `nc_put_var_complex` or `nc_put_var_bitfield`.
- Conversion between complex numbers and real/imaginary arrays.
- Support for opening arbitrary HDF5 files whose compound types happen to be named `r`/`i` but are not intended as complex numbers (detection is purely structural in this sprint).

#### Sprint 11: Create Tools and Language Bindings
**Objective:** Create `nextcopy` and `nextdump` with support for all NEXTCDF-4 types and compatibility modes. Expose the new C APIs and datatypes through Fortran and other maintained language bindings.

#### Sprint 12: Validate Compatibility and Prepare the Release
**Objective:** Run broad NetCDF-C compatibility, interoperability, regression, and representative-file testing across supported HDF5 versions. Complete performance and resource-leak checks, user documentation, examples, release notes, and v4.0.0 release readiness work.

### V3.5.0 - Add nep_meta.h with Build Info, and NISAR/SWOT/ABI Examples

#### Sprint 1: Create nep_meta.h
**Detailed Plan**: See `docs/plan/v3.5.0-sprint1-nep_meta.md`

- Create `include/nep_meta.h` from a committed `include/nep_meta.h.in` template using CMake `configure_file()` at build time; the generated `nep_meta.h` is added to `.gitignore` and never committed.
- Include version (major/minor/patch), build date, compiler name, and feature flags for enabled readers and compression filters.
- Add CMake logic to populate `nep_meta.h.in` placeholders using `version.txt` and the existing `HAVE_*` / `NEP_ENABLE_*` options.
- Update documentation to reference the new meta header and add an install rule for `nep_meta.h`.

**Clarified decisions:**
- `include/nep_meta.h.in` is the committed source of truth; `include/nep_meta.h` is generated into the build tree and added to `.gitignore`, matching the netcdf-c `netcdf_meta.h` convention.
- The build-info header is generated at configure time; build date is a configure-time timestamp, not a per-build dynamic value, to avoid unnecessary rebuilds.
- Feature flags reflect the state of `NEP_ENABLE_*` CMake options (PDB, MMCIF, CDF, GeoTIFF, GRIB2, FITS, PDS4, DICOM, LZ4, BZIP2, Fortran, etc.) as `0`/`1` macros.
- The generated header is installed alongside `nep.h` so downstream projects can inspect build capabilities.
- Acceptance is a compile-only C test `test/tst_meta.c` that includes `nep_meta.h` and validates the version macros.
- No runtime API for querying build info is added in this sprint.

**GitHub Issue:** [#353](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/353)

#### Sprint 2: NISAR Example
**Detailed Plan**: See `docs/plan/v3.5.0-sprint2-nisar-example.md`

- NISAR is a super-cool satellite!
- Let's add some example code which opens NISAR file and plots it in python.
- We want to take the example from /home/ed/nisar_play
- NISAR file is too big for repo, so user must provide path to it on the command line. There is one here: /home/ed/Downloads/NISAR_L3_PR_SME2_028_005_A_020_4005_DHDH_A_20260813T125218_20260813T125253_P05023_N_F_J_001_QA_STATS.h5
- CI does not really need to test this, we just need a working example from the command line.
- README.md needs to clearly explain how to run NISAR example, and show the output graph for fun.
- We are interested in NISAR soil moisture for this example.

**Clarified decisions:**
- The example lives in a new top-level `examples/nisar/` directory (not `examples/viz/NISAR/`), since NISAR SME2 files are plain CF/netCDF-compliant HDF5 read directly with `xarray`/`netCDF4` and involve no NEP UDF dispatch code, unlike the per-format `examples/viz/<FORMAT>` scripts.
- The example is Python-only and standalone: not registered in CMake/CTest and not run in CI, matching the roadmap's "CI does not really need to test this" note.
- The CLI mirrors `nisar_play`'s `plot-sme2` command: a required positional NISAR SME2 `.h5` path, plus `--output-dir` and `--show` flags, porting `sme2.py`/`plots.py` logic with minimal changes.
- Dependencies match `nisar_play` exactly: `xarray`, `netCDF4`, `matplotlib`, `cartopy`, plus `earthaccess` for the bbox fetch path, declared in a new `examples/nisar/requirements.txt`.
- The Earthdata bbox-search/download capability (`fetch.py`, `--bbox` flag) is ported over in full, including `~/.netrc` / `EARTHDATA_USERNAME`/`EARTHDATA_PASSWORD` credential handling, matching `nisar_play` feature-for-feature.
- A pre-generated soil-moisture PNG (produced from the sample SME2 file) is committed under `examples/nisar/` and embedded in `examples/nisar/README.md` and referenced from the top-level `README.md`.
- No changes are made to NEP's C library, `include/nep.h`, or the UDF dispatch framework this sprint; this is a pure Python usage example.

**GitHub Issue:** [#355](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/355)

#### Sprint 3: SWOT Example
**Detailed Plan**: See `docs/plan/v3.5.0-sprint3-swot-example.md`

- Add a standalone Python example under `examples/swot/` that opens a SWOT gridded sea-surface-height product (NASA PO.DAAC `SWOT_L2_LR_SSH_*` or AVISO+ `SWOT_L3_LR_SSH` NetCDF) and plots sea surface height anomaly (`ssha`) with `xarray`/`netCDF4`, `matplotlib`, and `cartopy`.
- Mirror the `examples/nisar/` structure: a `swot_example/` package with reader, plots, CLI, and an optional Earthdata/AVISO+ fetch module; `requirements.txt`; `README.md`; and committed example output PNG(s).
- The CLI takes a local `.nc` file path or a `--bbox W S E N` fetch path; the fetch path uses `earthaccess` against the chosen collection, with credentials from `~/.netrc` or environment variables.
- Write `docs/netCDF_with_SWOT.md` as a companion paper mirroring `docs/netCDF_with_NISAR.md`: explain how SWOT gridded SSH products map to netCDF-4, show `ncdump`/`h5dump` excerpts, and reference the example code.
- Update top-level `README.md` and `examples/README.md` to list the new SWOT example.

**Clarified decisions:**
- Target product is a SWOT gridded sea-surface-height NetCDF (e.g., PO.DAAC `SWOT_L2_LR_SSH_D` Basic/Expert or AVISO+ `SWOT_L3_LR_SSH` Basic). The primary plotted variable is `ssha` (sea surface height anomaly), masked with the product `quality_flag` when present; coordinates come from 2D `latitude`/`longitude` variables.
- The example is a standalone Python package (not a NEP UDF reader and not added to CMake/CTest/CI), following the NISAR Sprint 2 precedent.
- The fetch path uses `earthaccess` and NASA Earthdata/PO.DAAC credentials (`~/.netrc` or `EARTHDATA_USERNAME`/`EARTHDATA_PASSWORD`); if AVISO+ `L3_LR_SSH` is chosen, the fetch implementation switches to AVISO+ FTP/THREDDS access and documents AVISO+ credential setup.
- The reader is organized as `examples/swot/swot_example/l3_ssh.py`, `plots.py`, `fetch.py`, and `cli.py`, invoked via `python -m swot_example FILE` or `python -m swot_example --bbox W S E N`.
- Output PNGs (e.g., `ssha_map.png` and a `swath_footprint.png` overview) are committed under `examples/swot/figures/` and embedded in `examples/swot/README.md` and referenced from `docs/netCDF_with_SWOT.md`.
- No changes are made to NEP's C library, `include/nep.h`, the build system, or CI this sprint; this is documentation- and example-only.

**GitHub Issue:** [#357](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/357)

### V3.4.0 - More Proteins with mmCIF Format

#### Sprint 1: Setup
**Detailed Plan**: See `docs/plan/v3.4.0-sprint1-mmcif-setup.md`

- Refer to the `mmcif` skill file (`/.devin/skills/mmcif/SKILL.md`) and the `pdb-legacy` skill for the relationship between legacy PDB and mmCIF.
- Add the PDBx/mmCIF UDF8 slot definitions to `include/nep.h` (`NEP_UDF_MMCIF`, `NEP_MAGIC_MMCIF`, `NEP_FORMAT_NAME_MMCIF`).
- Add the `NEP_ENABLE_MMCIF` CMake option (default OFF) and the `HAVE_MMCIF`/`#cmakedefine HAVE_MMCIF` plumbing.
- Create a no-op dispatch skeleton (`include/mmcifdispatch.h`, `src/mmcifdispatch.c`, `src/mmciffile.c`) that builds `libncmmcif.so` and registers `"data_"` magic on UDF8.
- Ensure the existing `test/data/mmCIF/*.cif` files (from https://www.rcsb.org) are copied into the build tree.
- Add a C smoke test `test/tst_mmcif_udf.c` that opens and closes a real `.cif` file through `nc_open()`.
- Add a Fortran smoke test `ftest/ftst_mmcif_udf.F90` that opens and closes a real `.cif` file through `nf90_open()`.
- Add a dedicated `.github/workflows/ci-mmcif.yml` workflow that builds and tests with `NEP_ENABLE_MMCIF=ON`.

**Clarified decisions:**
- mmCIF is assigned **UDF8**; legacy PDB remains UDF7. The `mmcif` and `pdb-legacy` skill files already reflect this allocation.
- `NEP_ENABLE_MMCIF` defaults to **OFF** in this sprint, following the FITS/PDB precedent; it will flip to ON once the read-only dispatch layer is proven in Sprint 2.
- The new workflow is CMake-only because Autotools was removed in v3.1.0.
- No external parsing library is required; a custom STAR/CIF tokenizer will be added in Sprint 2.
- The smoke tests verify only `nc_open()`/`nc_close()` round-trips; no record parsing or PDBx category detection happens until Sprint 2.
- `NC_MMCIF_initialize()` registers the handler with the literal magic string `"data_"`. Avoiding false positives against generic (small-molecule) CIF files via a PDBx category check is deferred to Sprint 2.

**GitHub Issue:** TBD

#### Sprint 2: Dispatch Layer for mmCIF
**Detailed Plan**: See `docs/plan/v3.4.0-sprint2-mmcif-dispatch-layer.md`

- Fill in read-only dispatch layer.
- Build tests around files in /home/ed/NEP/test/data/mmCIF

**Clarified decisions:**
- Schema scope: parse `_atom_site` (coordinates and per-atom identity fields), `_cell`/`_symmetry` (unit cell), and single-row `_entry`/`_struct`/`_entity`/`_pdbx_database_status` categories as global attributes, matching the PDB Sprint 2 scope (CRYST1 + HEADER/TITLE/COMPND/SOURCE analogs). `_entity_poly_seq`-derived sequence data is deferred to a later sprint, mirroring PDB deferring `SEQRES` to Sprint 3.
- Coordinates are exposed as three separate `atom_site_Cartn_x/y/z` variables (`NC_DOUBLE`, shape `[model][atom]`), directly mirroring the PDB Sprint 2 `atom_site_Cartn_x/y/z` pattern (double instead of PDB's float, since mmCIF values carry more decimal precision).
- The `model` dimension is sized from distinct `_atom_site.pdbx_PDB_model_num` values (1 for all three current test files: `1J7W.cif`, `2W6V.cif`, `4HHB.cif`, all single-model X-ray structures); the `[model][atom]` code path exists but multi-model mmCIF is untested this sprint, a documented limitation until an NMR mmCIF file is acquired.
- The PDBx-vs-generic-CIF category check deferred from Sprint 1 is added now: after the `"data_"` magic match, `NC_MMCIF_open()` rejects files with no `_atom_site` category.
- `?` (unknown) and `.` (not-applicable) mmCIF placeholder values both map to the variable's type-appropriate NetCDF fill value; no separate mask/flag variable is added this sprint.
- `NEP_ENABLE_MMCIF` stays default **OFF** in `CMakeLists.txt`; only `ci-mmcif.yml` builds with it ON. Following the PDB precedent, the flip to default ON is reserved for a later "more testing" sprint.

**GitHub Issue:** [#350](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/350)

#### Sprint 3: Example Visualization
**Detailed Plan**: See `docs/plan/v3.4.0-sprint3-mmcif-visualizations.md`

- Add `examples/viz/mmCIF/` Python visualization scripts that open each mmCIF test file through the NetCDF UDF interface and produce publication-ready PNG artifacts, following the `examples/viz/PDB/` pattern.
- Wire the new scripts into `examples/viz/CMakeLists.txt` (`HAVE_MMCIF` guard, `_viz_artifacts` registration) and `examples/viz/README.md`.
- Update `docs/mmcif.md` (new file, if it does not already exist) with a Visualization section.
- Update `.github/workflows/ci-mmcif.yml` to build with `NEP_ENABLE_VIZ_EXAMPLES=ON` using the project virtual environment, so the new plots run in CI.

**Clarified decisions:**
- All three existing mmCIF test files (`1J7W.cif`, `2W6V.cif`, `4HHB.cif`) get their own visualization script/plot, maximizing parser regression coverage, matching the v3.1.0 Sprint 3 "visualize all test files" precedent rather than picking just one or two files.
- Plot content reuses the PDB Sprint viz pattern exactly: a 3D scatter of `atom_site_Cartn_x/y/z`, colored/marked by `atom_site_group_PDB` (`ATOM` vs `HETATM`), for consistency with `examples/viz/PDB/plot_pdb_xray_structure.py` and minimal new code, since the mmCIF and PDB schemas expose the same fields.
- `NEP_ENABLE_MMCIF` stays default **OFF** in `CMakeLists.txt`; this sprint is scoped to visualization only (unlike PDB Sprint 3, which combined "more testing" with the default flip). Flipping the default is deferred to a future "more testing" sprint.
- `ci-mmcif.yml` is extended to set up the project Python virtual environment and build with `-DNEP_ENABLE_VIZ_EXAMPLES=ON`, matching the DICOM Sprint 1 and PDB visualization precedent, so the new plots are exercised on every PR.

**GitHub Issue:** [#351](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/351)

### V3.3.0 - Proteins with PDB Format

#### Sprint 1: Setup
**Detailed Plan**: See `docs/plan/v3.3.0-sprint1-pdb-infrastructure.md`

- Look at PDB skill file.
- There are new test files in /test/data/PDB. They will be used in tests. Make sure they are available in build directory. (They are from https://www.rcsb.org)
- Create a no-op dispatch layer for PDB.
- Write a test for it.
- Update the CI so the new test is run.

**Clarified decisions:**
- Legacy PDB is assigned **UDF7** (next free slot; it ships before mmCIF, which now takes UDF8). The `pdb-legacy` and `mmcif` skill files have been updated to match.
- `NEP_ENABLE_PDB` / `--enable-pdb` defaults to **OFF** in this sprint, following the exact FITS Sprint 1/2 precedent (flip to ON once the dispatch layer is proven, in Sprint 2).
- A new dedicated `ci-pdb.yml` workflow is added (consistent with `ci-fits.yml`/`ci-dicom.yml`), even though PDB requires no external parsing library — no extra system packages beyond the standard HDF5/NetCDF-C/NetCDF-Fortran stack are needed.
- `test/tst_pdb_udf.c` only verifies an `nc_open()`/`nc_close()` round-trip on the real `.pdb` test files; no PDB record parsing or magic/format-detection testing happens until Sprint 2.
- `NC_PDB_initialize()` registers the format with `nc_def_user_format()` using the literal magic string `"HEADER"` (`NEP_MAGIC_PDB`), per the `pdb-legacy` skill. Files that do not start with a `HEADER` record are a documented limitation, not solved in Sprint 1.

**GitHub Issue:** [#342](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/342)

#### Sprint 2: Dispatch Layer for PDB
**Detailed Plan**: See `docs/plan/v3.3.0-sprint2-pdb-dispatch-layer.md`

- Look at PDB skill file.
- Write read-only PDB dispatch code.
- Write tests involving PDB files in test/data/PDB. Check metadata and some data.

**Clarified decisions:**
- Schema scope: parse `ATOM`/`HETATM` coordinates and per-atom identity fields, `CRYST1` unit-cell global attributes, and `HEADER`/`TITLE`/`COMPND`/`SOURCE` global attributes. `SEQRES`-derived sequence data is deferred to Sprint 3.
- `ATOM` and `HETATM` records share a single `atom` dimension in file order, distinguished by a per-atom `atom_site_group_PDB` variable (`"ATOM"`/`"HETATM"`), matching the `mmcif` skill's `_atom_site` mapping.
- `MODEL`/`ENDMDL` blocks are parsed now: the `model` dimension is sized from the number of `MODEL` blocks (1 if none are present, as in both current test files), and the coordinate variable is shaped `[model][atom]`. Untested against real multi-model data this sprint since neither `1J7W.pdb` nor `4HHB.pdb` contains `MODEL` records.
- Files with no `ATOM`/`HETATM` records at all are rejected with `NC_EINVAL`. Hybrid-36 encoded serial/residue numbers are a documented known limitation, not handled this sprint.

**GitHub Issue:** [#343](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/343)

#### Sprint 3: More Testing of PDB Reading
**Detailed Plan**: See `docs/plan/v3.3.0-sprint3-pdb-more-testing.md`

Acquire additional real-world legacy PDB test files and extend `test/tst_pdb_udf.c` to exercise features not covered by the existing `1J7W.pdb`/`4HHB.pdb` X-ray files. Flip `NEP_ENABLE_PDB` to default ON and add a Fortran smoke check once the reader is covered by the expanded C test suite.

**Clarified decisions:**
- Acquire one multi-model NMR ensemble from RCSB (to exercise the `MODEL`/`ENDMDL` path and the `[model][atom]` coordinate variables) and one single-model synthetic PDB file without `CRYST1` (AlphaFold DB direct download returned 404 for multiple UniProt IDs during acquisition).
- All new sample files are placed in `test/data/PDB/`, copied to the build tree by `test/CMakeLists.txt`, and documented with provenance and license in the sprint plan.
- `NEP_ENABLE_PDB` flips from OFF to default ON in `CMakeLists.txt` now that the dispatch layer parses real files end-to-end.
- `ftest/ftst_pdb_udf.F90` is added as an open/close smoke check on one real PDB file, matching FITS/DICOM Fortran coverage.
- New test assertions include: `model` dimension > 1, coordinates differ across models, and graceful handling of files with no `CRYST1` record (optional attributes omitted). `SEQRES`, hybrid-36 encoding, and multi-character chain IDs remain documented known limitations.
- Build system remains CMake-only (Autotools removed in v3.1.0); `ci-pdb.yml` is updated to exercise the default-ON reader.

**Acceptance Criteria:**
- At least two new PDB test files are present in `test/data/PDB/` with documented provenance.
- `model` dimension length is verified as > 1 for the NMR file, and `atom_site_Cartn_x/y/z` values for the same atom index differ across models.
- The synthetic `no_cryst1.pdb` file opens successfully; since it lacks `CRYST1`, no `cell_*` or `space_group_name_H-M` global attributes are required.
- `ftest/ftst_pdb_udf.F90` compiles and passes when `NEP_ENABLE_FORTRAN=ON`.
- `NEP_ENABLE_PDB` defaults to ON in `CMakeLists.txt`.
- All PDB tests pass with PDB enabled; the full suite remains green with PDB explicitly disabled.

**Testing:** Configure with `-DNEP_ENABLE_PDB=ON` (now default) and run `ctest --test-dir build -R pdb --output-on-failure`. Verify `tst_pdb_udf` and `ftst_pdb_udf` pass, then run the full CTest suite to confirm no regressions. Validate the updated `ci-pdb.yml` job is green.

**Build System Integration:** `CMakeLists.txt` (default option flip), `test/CMakeLists.txt` (copy new `test/data/PDB/` files and add them to `tst_pdb_udf`), `ftest/CMakeLists.txt` (add `ftst_pdb_udf`), and `.github/workflows/ci-pdb.yml` (remove explicit `-DNEP_ENABLE_PDB=ON` to test the default).

**Definition of Done:** PDB reader is enabled by default, the expanded C test suite covers multi-model NMR and non-RCSB single-model files, a Fortran smoke test is in place, CI exercises the default-ON configuration, and all tests pass with no regressions.

**GitHub Issue:** [#345](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/345)

#### Sprint 4: Documentation
**Detailed Plan**: See `docs/plan/v3.3.0-sprint4-pdb-documentation.md`

- Update all the docs.
- Add a section on PDB to doxygen docs.
- Write some examples/viz visulaizations.

**Clarified decisions:**
- Two visualization scripts are added under `examples/viz/PDB/`: a static 3D scatter plot of `ATOM`/`HETATM` Cartesian coordinates for the X-ray structure `4HHB.pdb`, and a multi-model overlay plot from the NMR ensemble `1GAB.pdb` showing coordinate variation across models. Both follow the existing per-format viz pattern (`plot_common.py`, `verify_viz_artifacts.py`, metadata sidecar files).
- A standalone C example `examples/pdb/pdb_read.c` is added, matching the style of `examples/dicom/dicom_read.c` (calls `NC_PDB_initialize()`, opens a PDB file, prints dims/vars and a slice of atom coordinates), and is registered in `examples/CMakeLists.txt` and `examples/README.md`.
- Full documentation parity with FITS/DICOM: a new `docs/pdb.md` format reference page (mapping, enabling, dependencies, resources, example, matching `docs/fits.md` structure), a new PDB row/section in `docs/formats.md`'s UDF slot table and format reference table, PDB entries in the top-level `README.md` (format table, CMake option table, Spack variant table), `docs/pdb.md` added to `docs/Doxyfile.in`'s `INPUT` list, and PDB mentions added to `examples/README.md` and `examples/viz/README.md`.
- No reader behavior changes in this sprint; it is documentation- and example-only.

**GitHub Issue:** [#347](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/347)

### V3.2.0 - More DICOM

Organize the visualization examples and add more DICOM functionality.

#### Sprint 1: Organize examples/viz
**Detailed Plan**: See `docs/plan/v3.2.0-sprint1-organize-viz.md`

Reorganize format-specific visualization scripts into source directories for CDF, DICOM, FITS, GeoTIFF, GRIB2, and PDS4. CMake must mirror the same directory structure in the build tree while preserving all existing visualization test names, runtime environment, UDF guards, copied test data, artifact basenames, flat artifact output location, and verifier interface.

**Clarified decisions:**
- The source and CMake build trees mirror the six per-format directories.
- The `examples/viz/` root retains shared infrastructure only: `plot_common.py`, helper tests, the verifier, root CMake configuration, and the root README.
- DICOM's `_dicom_udf.py` is colocated with its DICOM plot scripts; format scripts must reliably import root shared utilities from their mirrored build-tree locations.
- Generated PNG and metadata artifacts remain in the existing flat visualization build directory. No artifact basenames, CTest names, fixture behavior, or verifier command-line interface changes.
- Update the root visualization README with the new layout and manual paths. Do not add per-format READMEs.

**Acceptance Criteria:**
- All format-specific visualization scripts are under their matching source and mirrored build-tree format directories.
- Every enabled visualization test executes its relocated script with unchanged runtime behavior and expected artifacts.
- The flat artifact output directory continues to pass `verify_viz_artifacts.py` using the existing basename list.
- The root README accurately documents the grouped layout and manual execution paths.
- No reader behavior, public CMake option, artifact content, artifact basename, or test data changes solely because of this reorganization.

**Testing:** Configure with visualization examples and applicable readers enabled, run `ctest --test-dir build -R viz --output-on-failure`, execute enabled format tests individually, validate all artifacts through the existing verifier interface, inspect the mirrored build-tree layout, and run a non-visualization configuration for regression coverage.

**Build System Integration:** `examples/viz/CMakeLists.txt`, relocated Python imports/path handling, and `examples/viz/README.md`. CMake remains the sole supported build system; no dependency, option, UDF-registration, or install-rule change is required.

**Definition of Done:** Format-specific visualization scripts are organized in matching source and build-tree directories, shared utilities remain at the visualization root, existing CTest and artifact behavior is preserved, the README is current, and visualization tests pass for every enabled reader.

**GitHub Issue:** #334

#### Sprint 2: More DICOM Functionality
**Detailed Plan**: See `docs/plan/v3.2.0-sprint2-dicom-sample-coverage.md`

Verify and document that the NEP DICOM UDF reader correctly reads all sample files already present in `test/data/DICOM`. Baseline investigation found the sprint's original test-only premise was wrong: two of the five OME samples (`CT-MONO2-16-chest.dcm`, `MR-MONO2-12-shoulder.dcm`) use JPEG Lossless transfer syntaxes (`1.2.840.10008.1.2.4.70` and `.4.57`), not "Explicit VR Little Endian" as originally documented, and are currently rejected. A third (`CR-MONO1-10-chest.dcm`) has no DICOM preamble/File Meta Information at all and is unrelated to JPEG. This sprint adds JPEG Lossless decode support and descopes the no-preamble file.

**Implementation scope:**
- Add a memory-based `jpeg_source_mgr` and per-precision (8/12/16-bit) lossless JPEG decode wrappers using GDCM's `gdcmjpeg8`/`gdcmjpeg12`/`gdcmjpeg16` libraries (IJG libjpeg 6b + the classic lossless/Process 14 patch, mangled symbol namespace; new optional build dependency `libgdcm-dev`).
- Detect the JPEG frame's data precision (SOF3 marker) at decode time to select the matching 8/12/16-bit codec.
- Recognize `1.2.840.10008.1.2.4.57` and `1.2.840.10008.1.2.4.70` as supported encapsulated transfer syntaxes in `src/dicomfile.c`, alongside existing JPEG Baseline support.
- Add CMake detection for `gdcmjpeg8`/`gdcmjpeg12`/`gdcmjpeg16` headers/libraries under `NEP_ENABLE_DICOM`.
- Add test blocks to `test/tst_dicom_udf.c` for `CT-MONO2-16-brain.dcm` and `MR-MONO2-16-head.dcm` (native, already working) and for `CT-MONO2-16-chest.dcm` and `MR-MONO2-12-shoulder.dcm` (new lossless decode path), including pixel reads.
- Add a test asserting `CR-MONO1-10-chest.dcm` is cleanly rejected (`NC_EINVAL`) and is out of scope for this sprint.
- Add a lightweight open/close Fortran smoke check to `ftest/ftst_dicom_udf.F90` for one of the confirmed-working native files.
- Correct `test/data/DICOM/README.md`'s transfer-syntax labels for all five OME samples to match their actual embedded UIDs, and update `docs/dicom.md`.
- Update `.github/workflows/ci-dicom.yml` to install `libgdcm-dev`, build NetCDF-Fortran, and set `-DNEP_ENABLE_FORTRAN=ON` so `ftst_dicom_udf` runs in the focused DICOM workflow.

**Clarified decisions:**
- JPEG Lossless (`.4.57`, `.4.70`) decode is implemented via GDCM's bundled IJG lossless-JPEG codec (`gdcmjpeg8/12/16`), a new optional dependency, rather than vendoring the codec source or implementing a decoder from scratch.
- `CR-MONO1-10-chest.dcm` (no preamble/File Meta Information) is explicitly descoped; it remains a clean-rejection case. No-preamble DICOM support is deferred to a future sprint if ever needed.
- MONOCHROME1 pixel data (if encountered) is exposed as-is; the `PhotometricInterpretation` attribute remains the contract consumers use to invert for display.
- Each sample gets its own explicit test block in `test/tst_dicom_udf.c`, matching the current per-file style.
- Fortran coverage adds one smoke check (open/close only) rather than full parity with the C test.

**Acceptance Criteria:**
- `test/tst_dicom_udf.c` opens, inspects, and reads pixel data from `CT-MONO2-16-brain.dcm`, `CT-MONO2-16-chest.dcm`, and `MR-MONO2-12-shoulder.dcm`; opens and inspects metadata for `MR-MONO2-16-head.dcm` (pixel-data read is skipped due to an independent libdicom limitation with this file's missing `NumberOfFrames` tag, documented in the sprint plan); and asserts a clean `NC_EINVAL` rejection for `CR-MONO1-10-chest.dcm`.
- `ftest/ftst_dicom_udf.F90` opens and closes at least one confirmed-working new sample without error.
- `docs/dicom.md` and `test/data/DICOM/README.md` accurately describe JPEG Lossless support, correct transfer-syntax labels, and the `CR-MONO1-10-chest.dcm` rejection.
- `.github/workflows/ci-dicom.yml` installs `libgdcm-dev`, builds NetCDF-Fortran, enables `NEP_ENABLE_FORTRAN`, and runs `ftst_dicom_udf` alongside `tst_dicom_udf`.
- All DICOM tests pass with DICOM enabled; the full suite remains regression-free with DICOM disabled.

**Testing:** Run `ctest --test-dir build -R dicom --output-on-failure` with `-DNEP_ENABLE_DICOM=ON -DNEP_ENABLE_FORTRAN=ON`; confirm `tst_dicom_udf` and `ftst_dicom_udf` both pass; confirm the updated `ci-dicom.yml` job is green.

**Build System Integration:** `src/CMakeLists.txt` (gdcmjpeg8/12/16 detection and linking), `test/CMakeLists.txt`, `ftest/CMakeLists.txt`, `.github/workflows/ci-dicom.yml` (libgdcm-dev install, NetCDF-Fortran build step, `NEP_ENABLE_FORTRAN=ON`).

**Definition of Done:** JPEG Lossless (`.4.57`/`.4.70`) DICOM files decode correctly through the DICOM UDF reader, all four newly-supported OME samples are validated by the C and Fortran test suites, `CR-MONO1-10-chest.dcm` is documented as a clean, expected rejection, documentation accurately reflects transfer-syntax support, `ci-dicom.yml` runs the expanded Fortran coverage with the new dependency, and the full test suite remains green with DICOM enabled and disabled.

**GitHub Issue:** #338

#### Sprint 3: Visualize All DICOM Files in test/data/DICOM
- We need to create new examples/viz/DICOM examples for the new files we can now read.

### V3.1.0 - DICOM Visualizations, CMake Only Builds

#### Sprint 1: Add Visualization of DICOM Test File
**Detailed Plan**: See `docs/plan/v3.1.0-sprint1-dicom-visualizations.md`

Add the first DICOM visualization examples to `examples/viz/`, generate two publication-ready grayscale PNG artifacts from the existing `MRBRAIN.DCM` and `0003.DCM` samples, and acquire 3–5 additional public-domain DICOM samples to broaden visual content and regression coverage.

**Implementation scope:**
- Add `examples/viz/plot_dicom_mrbrain.py` to open `test/data/DICOM/MRBRAIN.DCM` through the NetCDF UDF interface, read `pixel_data[0, :, :]`, normalize the 16-bit signed/unsigned samples to grayscale, and write `dicom_mrbrain_image.png` + `dicom_mrbrain_image_metadata.txt`.
- Add `examples/viz/plot_dicom_xa_montage.py` to open `test/data/DICOM/0003.DCM`, read all 17 encapsulated JPEG Baseline frames, build a compact grayscale montage, and write `dicom_xa_frame_montage.png` + `dicom_xa_frame_montage_metadata.txt`.
- Update `examples/viz/CMakeLists.txt` and `examples/viz/Makefile.am` to copy the new scripts, conditionally run them under `HAVE_DICOM`, and append the two artifact basenames to the visualization artifact list.
- Update `examples/viz/README.md` with DICOM visualization instructions and script descriptions.
- Add `HAVE_DICOM` to the `NEP_ENABLE_VIZ_EXAMPLES` UDF-reader guard in top-level `CMakeLists.txt` and `configure.ac`.
- Add a Visualization section to `docs/dicom.md` describing the new plots and required environment variables.
- Acquire 3–5 additional public-domain DICOM samples (e.g., CT, CR, or US images) for `test/data/DICOM/`, documenting their provenance, license, and any clinically sensitive content considerations.
- Update `.github/workflows/ci-dicom.yml` to enable visualization examples (`NEP_ENABLE_VIZ_EXAMPLES=ON` / `--enable-viz-examples`), install Python dependencies in the project virtual environment, and run the visualization tests.

**Clarified decisions:**
- Two visualization artifacts are produced in Sprint 1: `dicom_mrbrain_image` (single-frame 16-bit MR) and `dicom_xa_frame_montage` (17-frame XA montage).
- DICOM visualization output uses an external `_metadata.txt` with exactly `title`, `caption`, and `alt_text`, a caption of at most 75 words, and a figure size at most 8.0 by 6.1 inches.
- `MRBRAIN.DCM` is plotted by normalizing the 16-bit `pixel_data` range to 8-bit grayscale; signed `NC_SHORT` values are offset before scaling.
- `0003.DCM` frames are arranged in a rectangular montage; if any frame has color photometric interpretation, it is converted to luminance before tiling.
- Additional test files are read-only data; they do not change existing test logic unless they exercise a new supported Transfer Syntax, in which case the change is limited to a new regression case.
- New samples must be public-domain or CC-licensed and must not contain real patient identifiers.

**Acceptance Criteria:**
- `plot_dicom_mrbrain.py` and `plot_dicom_xa_montage.py` are present in `examples/viz/` and copied to the build tree by both build systems.
- CMake and Autotools configure successfully with `-DNEP_ENABLE_DICOM=ON -DNEP_ENABLE_VIZ_EXAMPLES=ON` and `--enable-dicom --enable-viz-examples`, respectively.
- `ctest -R viz --output-on-failure` and `make check` generate both PNG/metadata pairs and `verify_viz_artifacts.py` passes.
- Both PNGs are within the 8.0 by 6.1 inch / 150 DPI limits and have valid `_metadata.txt` files.
- `docs/dicom.md` and `examples/viz/README.md` describe the new visualization scripts.
- At least 3 new public-domain DICOM samples are added under `test/data/DICOM/` with provenance documented in the sprint plan.
- `.github/workflows/ci-dicom.yml` passes with visualization enabled.
- Existing DICOM tests (`tst_dicom_udf`) continue to pass with DICOM enabled; the full suite remains regression-free with DICOM disabled.

**Testing:** Run `ctest -R viz --output-on-failure` and `make check` in a DICOM-enabled build, inspect the generated `dicom_mrbrain_image.png` and `dicom_xa_frame_montage.png` artifacts, run `tst_dicom_udf` for regression coverage, and verify the DICOM-enabled `ci-dicom.yml` job is green.

**Build System Integration:** `examples/viz/CMakeLists.txt`, `examples/viz/Makefile.am`, top-level `CMakeLists.txt`, `configure.ac`, `examples/viz/README.md`, `docs/dicom.md`, `.github/workflows/ci-dicom.yml`.

**Definition of Done:** Two DICOM visualization scripts generate validated, publication-ready artifacts under both CMake and Autotools; the visualization guard conditions recognize DICOM; user-facing documentation describes the plots; 3–5 new public-domain DICOM samples are added with documented provenance; `ci-dicom.yml` passes with visualization enabled; existing tests remain green.

**GitHub Issue:** #330

#### Sprint 2: CMake-Only Build Migration
**Detailed Plan**: See `docs/plan/v3.1.0-sprint2-cmake-only-build.md`

Make CMake the sole supported NEP build system beginning with v3.1.0. Remove the complete Autotools implementation and generated artifacts, convert all CI workflows to retain equivalent CMake coverage, and remove current user-facing Autotools guidance.

**Implementation scope:**
- Delete root and nested Autotools inputs, generated artifacts, helper scripts, macros, and Autotools-only test runners, including `configure.ac`, `configure`, `Makefile.am`, `Makefile.in`, `aclocal.m4`, `config.h.in`, `autogen.sh`, `m4/`, and Autotools build material under `hdf5_plugins/`.
- Preserve shared C/C++/Fortran sources, test data, CMake files, CMake-generated configuration, and CMake test execution.
- Remove Autotools matrix entries, bootstrap/configure/build/test steps, variables, and wording from all GitHub Actions workflows; retain or add equivalent CMake coverage for every active configuration.
- Update `README.md`, active product/design documentation, Doxygen configuration and generated-documentation guidance, packaging guidance, and the v3.1.0 release notes to identify CMake as the only supported build path.
- Remove references to `./configure`, `autoreconf`, `make check`, and Autotools-only options from active user-facing documentation; do not rewrite frozen historical roadmap, plan, issue, or release documents.
- Retain the current CMake minimum version unless the implementation audit identifies an existing incompatibility.

**Clarified decisions:**
- CMake is the sole supported build system from v3.1.0 onward; no `./configure` compatibility wrapper or legacy directory will be retained.
- The removal includes generated Autotools files and Autotools-specific test runners, not only their authored inputs.
- All repository CI is CMake-only. Existing configuration breadth remains: default/minimal/compression, Fortran on/off, enabled format readers, DICOM, parallel I/O, visualization, documentation, Conda, and Spack coverage where applicable.
- CMake 3.9 remains the declared minimum version for this sprint.
- Active documentation and packaging guidance must remove Autotools instructions; historical records remain unchanged.

**Acceptance Criteria:**
- No Autotools inputs, generated files, macros, helper scripts, `Makefile.am`, `Makefile.in`, or Autotools-only test runners remain in the repository.
- A repository search finds no active CI or user-facing references to `./configure`, `autoreconf`, `autogen.sh`, `make check`, or Autotools.
- Every GitHub Actions workflow uses CMake only, with no Autotools job, matrix value, bootstrap, configure, build, or test step.
- The CMake default, minimal, DICOM-enabled, format-enabled, Fortran on/off, documentation, examples, visualization, and parallel configurations continue to configure, build, and test successfully as applicable.
- `README.md`, active Doxygen documentation, design/requirements/FAQ documentation, and packaging guidance describe CMake-only installation and testing accurately.
- Existing public CMake options, install behavior, UDF reader behavior, compression filters, and source compatibility remain unchanged.

**Testing:** Run clean CMake configure/build/CTest validation for the retained CI configurations, build Doxygen documentation without warnings, and search the tracked repository for removed Autotools files and active references. Confirm CMake installation and package-consumer checks where covered by CI.

**Build System Integration:** Root and nested CMake files, CMake test registration, `.github/workflows/`, `README.md`, `docs/Doxyfile.in`, active design/requirements/FAQ documentation, release notes, Conda recipe/build script, and Spack package guidance.

**Definition of Done:** The repository has one supported build path (CMake), all active CI coverage is CMake-based, user and package documentation provides only CMake instructions, all retained configurations pass their CMake validation, and no unsupported Autotools artifacts or active references remain.

**GitHub Issue:** #332

#### Sprint 3: More DICOM Visualizations
- There are additional DICOM files in test/data/DICOM.
- Let's get some visualizations!

### V3.0.0 - DICOM Reader

Add the ability to read files in DICOM format through a new NetCDF UDF handler that uses the `libdicom` C library. The reader exposes DICOM image pixel data and key metadata as NetCDF variables and attributes, supports native uncompressed single-frame images first, then encapsulated JPEG compressed and multi-frame images, and finally functional-group metadata, Fortran bindings, documentation, and CI.

#### Sprint 1: DICOM Reader Infrastructure
**Detailed Plan**: See `docs/plan/v3.0.0-sprint1-dicom-infrastructure.md`

Integrate `libdicom` into both build systems, assign DICOM to UDF slot 6, implement the dispatch skeleton, and read native uncompressed single-frame DICOM files through the NetCDF API.

**Implementation scope:**
- Add `NEP_UDF_DICOM`, `NEP_MAGIC_DICOM`, and `NEP_FORMAT_NAME_DICOM` to `include/nep.h`.
- Add `-DNEP_ENABLE_DICOM` / `--enable-dicom` build options (default OFF) and detect `libdicom`.
- Create `src/dicomdispatch.c`, `src/dicomfile.c`, and `include/dicomdispatch.h` following the FITS handler pattern.
- Implement `NC_DICOM_initialize()`, `NC_DICOM_open()`, `NC_DICOM_close()`, `NC_DICOM_abort()`, and format inquiry functions.
- Map DICOM Patient/Study/Series/Image Pixel module tags to NetCDF dimensions, variables, and attributes.
- Implement `NC_DICOM_get_vara()` for native (uncompressed) pixel data.
- Add `test/tst_dicom_udf.c` and a small uncompressed DICOM sample.
- Create `ci-dicom.yml` to validate the build and the new test.

**Clarified decisions:**
- DICOM occupies UDF slot 6 (`NC_UDF6`); UDF0-UDF5 are already assigned.
- The DICOM magic string is `DICM` at byte offset 128; NetCDF-C magic detection may need an offset-aware check or the file can be opened explicitly with the UDF mode flag.
- Compressed Transfer Syntaxes (including the existing `test/data/DICOM/0003.DCM`, which is JPEG Baseline) are explicitly rejected in Sprint 1.
- Pixel data type is derived from `BitsAllocated` and `PixelRepresentation`.
- `PlanarConfiguration` and `SamplesPerPixel` determine whether a `sample` dimension is created.

**Acceptance Criteria:**
- `include/nep.h` defines the new UDF slot, magic, and format-name macros.
- CMake and Autotools detect `libdicom`, define `HAVE_DICOM`, and link `-ldicom`.
- `NC_DICOM_initialize()` registers the UDF6 dispatch table.
- `nc_open()` succeeds on an uncompressed single-frame DICOM file after the handler is registered.
- Dimensions, variable type, and global attributes match the DICOM metadata.
- `nc_get_vara()` reads a subset of `pixel_data` correctly.
- `test/tst_dicom_udf.c` passes under CMake and Autotools.
- `ci-dicom.yml` passes for both build systems.
- Compressed DICOM files fail cleanly with a clear error.

**Testing:** Run `tst_dicom_udf` directly and via `ctest -R dicom` / `make check`; verify the uncompressed sample; verify that `test/data/DICOM/0003.DCM` is rejected rather than crashing.

**Build System Integration:** Top-level `CMakeLists.txt` and `configure.ac`; `src/CMakeLists.txt` and `src/Makefile.am`; `test/CMakeLists.txt` and `test/Makefile.am`; new `.github/workflows/ci-dicom.yml`.

**Definition of Done:** `libdicom` is integrated, the UDF6 dispatch skeleton works, uncompressed single-frame DICOM files open and expose metadata, pixel data is readable, the C smoke test passes, CI is green, and the sprint documentation is complete.

**GitHub Issue:** TBD

#### Sprint 2: Compressed Pixel Data and Multi-Frame Support
**Detailed Plan**: See `docs/plan/v3.0.0-sprint2-dicom-compressed-multiframe.md`

Extend the reader to decompress encapsulated JPEG pixel data and support multi-frame images. Use the existing `test/data/DICOM/0003.DCM` JPEG Baseline sample as the primary compressed test case.

**Implementation scope:**
- Detect encapsulated Transfer Syntaxes from `(0002,0010)`.
- Add `libjpeg`/`libjpeg-turbo` detection and decompression for JPEG Baseline frames.
- Use `libdicom` frame-level read functions (`dcm_filehandle_read_frame`) to obtain raw encapsulated fragments.
- Decompress each requested frame on demand inside `NC_DICOM_get_vara()`.
- Add a `frame` dimension for `NumberOfFrames > 1`.
- Support grayscale and RGB 8-bit decompressed output.
- Update `test/tst_dicom_udf.c` to verify the compressed sample.

**Clarified decisions:**
- Only JPEG Baseline (`1.2.840.10008.1.2.4.50`) is required for this sprint; other Transfer Syntaxes may be recognized and rejected or supported if straightforward.
- Decompressed frames are cached for the duration of a single `nc_get_vara()` call.
- JPEG decompression output is exposed as `NC_UBYTE`; 12-bit JPEG values are stored in 16-bit unsigned samples.
- `PlanarConfiguration` is normalized to interleaved RGB in the NetCDF view for JPEG color images.

**Acceptance Criteria:**
- `test/data/DICOM/0003.DCM` opens and exposes correct Rows, Columns, SamplesPerPixel, and PhotometricInterpretation.
- `nc_get_vara()` reads decompressed pixel values from the JPEG sample.
- Single-frame and multi-frame uncompressed images continue to work (Sprint 1 regression coverage).
- CMake and Autotools both link `libjpeg` when DICOM is enabled.
- `ci-dicom.yml` passes with DICOM + JPEG support.
- Unsupported Transfer Syntaxes produce a clear error.

**Testing:** Run `tst_dicom_udf` against the compressed sample; run the Sprint 1 uncompressed sample for regression; run `ctest -R dicom --output-on-failure` and `make check`.

**Build System Integration:** Add `find_package(JPEG)` (CMake) and `AC_CHECK_LIB([jpeg], ...)` (Autotools); link `JPEG_LIBRARIES` into the DICOM reader objects.

**Definition of Done:** JPEG Baseline encapsulated DICOM files open, decompress, and expose readable pixel data; multi-frame layout works; the compressed sample test passes; CI is green; and the sprint documentation is complete.

**GitHub Issue:** TBD

#### Sprint 3: Test Expansion, C/Fortran Examples, Documentation, Packaging, and CI
**Detailed Plan**: See `docs/plan/v3.0.0-sprint3-dicom-tests-examples-ci.md`

Use the new DICOM test files (`MRBRAIN.DCM`, `0003.DCM`, and the existing uncompressed sample) to expand regression coverage, add a Fortran smoke test, create a C example program, update all user-facing documentation, and wire DICOM support into the existing CI matrix and packaging recipes.

**Implementation scope:**
- Expand `test/tst_dicom_udf.c` to exercise all three sample files, including the 16-bit uncompressed MR image and the 17-frame JPEG Baseline XA image.
- Add `ftest/ftst_dicom_udf.F90` and update Fortran build rules.
- Add `examples/dicom/dicom_read.c` with compact output and the NetCDF Developer's Handbook Second Edition reference.
- Create `docs/dicom.md` and update `docs/formats.md`, `docs/design.md`, `docs/prd.md`, and `README.md`.
- Update Spack and Conda recipes with optional DICOM support.
- Integrate DICOM into `.github/workflows/ci.yml` in addition to the focused `ci-dicom.yml`.

**Clarified decisions:**
- The Fortran test calls `NC_DICOM_initialize()` and opens the uncompressed sample, matching the FITS Fortran test pattern.
- DICOM remains disabled by default in CMake, Autotools, Spack, and Conda.
- Example output follows the NEP compact style (`ERR(retval)`, `Done.`).
- The three current sample files do not contain Shared/Per-frame Functional Groups; functional-group metadata is deferred to Sprint 4.

**Acceptance Criteria:**
- `test/tst_dicom_udf.c` passes for all three sample files under both build systems.
- `ftest/ftst_dicom_udf.F90` compiles and passes under both build systems.
- `examples/dicom/dicom_read.c` builds, runs, and prints compact metadata ending with `Done.`.
- All user-facing documentation describes DICOM support accurately.
- Spack and Conda recipes include optional DICOM support.
- `ci.yml` builds and tests DICOM in at least one job.
- All DICOM tests pass with DICOM enabled; existing tests pass with DICOM disabled.

**Testing:** Run `ctest -R dicom --output-on-failure`, `make check`, `ftst_dicom_udf`, and `dicom_read`; verify the DICOM-enabled `ci.yml` job and `ci-dicom.yml` both pass.

**Build System Integration:** `examples/CMakeLists.txt` and `examples/Makefile.am`; `ftest/CMakeLists.txt` and `ftest/Makefile.am`; `spack/NEP/package.py`; `conda/meta.yaml` and `conda/build.sh`; `.github/workflows/ci.yml`.

**Definition of Done:** The DICOM reader has expanded regression coverage over all available sample files, a C example and Fortran smoke test, user-facing documentation, optional packaging support, and a CI matrix job, and the full test suite passes with DICOM enabled while remaining regression-free with DICOM disabled.

**GitHub Issue:** TBD

#### Sprint 4: Functional Groups and Enhanced Multi-frame Metadata
**Detailed Plan**: See `docs/plan/v3.0.0-sprint4-dicom-functional-groups.md`

Expose Shared and Per-frame Functional Group metadata from enhanced multi-frame DICOM objects as NetCDF variables or global attributes once a suitable enhanced multi-frame sample is available.

**Implementation scope:**
- Parse Shared Functional Groups Sequence `(5200,9229)` and Per-frame Functional Groups Sequence `(5200,9230)`.
- Expose `image_position_patient`, `image_orientation_patient`, `pixel_spacing`, and `slice_thickness` as NetCDF variables over the `frame` dimension or as global attributes when shared.
- Acquire a multi-frame enhanced DICOM sample containing functional groups for testing.

**Clarified decisions:**
- Functional-group variables are created only when the corresponding sequences are present.
- Shared values become scalar global attributes; per-frame values become `[frame]` variables.

**Acceptance Criteria:**
- Multi-frame enhanced DICOM files expose functional-group metadata variables when present.
- All DICOM tests pass with DICOM enabled; existing tests continue to pass with DICOM disabled.
- Unsupported Transfer Syntaxes and malformed files fail cleanly.

**Testing:** Run `ctest -R dicom --output-on-failure` and `make check` against a multi-frame enhanced sample containing functional groups; run the full suite with DICOM disabled to confirm no regressions.

**Build System Integration:** None beyond existing `HAVE_DICOM` conditionals.

**Definition of Done:** The DICOM reader exposes shared and per-frame functional-group metadata for enhanced multi-frame objects, and the full test suite passes with DICOM enabled while remaining regression-free with DICOM disabled.

**GitHub Issue:** TBD

### V3.0.1 Spack and Conda Catchup

#### Sprint 1: Spack Catchup
**Detailed Plan**: See `docs/plan/v3.0.1-sprint1-spack-catchup.md`

Bring the in-repository NEP Spack recipe, Spack CI, user documentation, and the existing upstream Spack pull request up to date through NEP v3.0.0.

**Implementation scope:**
- Add released versions 2.6.0, 2.6.1, 2.7.0, 2.7.1, 2.8.0, and 3.0.0 with verified tag-tarball checksums, in descending version order.
- Resolve the open review feedback in [spack/spack-packages#5557](https://github.com/spack/spack-packages/pull/5557): version ordering, BZIP2 plugin verification, and removal of the unsatisfiable CDF variant.
- Omit `+cdf` until NASA CDF has an upstream Spack recipe, so the in-repository package file remains directly submit-ready.
- Update the upstream pull request in place, rather than opening a replacement.
- Update Spack CI to use v3.0.0 as the stable release, cover DICOM and all supported reader combinations, and remove masked install failures.
- Update the README and release-status documentation for the current stable Spack package.

**Clarified decisions:**
- All published releases after 2.5.0 through 3.0.0 are added.
- Spack CI validates default, minimal, DICOM-enabled, and all-supported-reader concretizations, plus minimal and all-supported-reader installs.
- CDF Spack support is deferred until its dependency is accepted upstream.

**Acceptance Criteria:**
- The local Spack recipe contains the six new versions with verified checksums in newest-to-oldest order.
- The in-repository recipe omits `+cdf`, matching the submit-ready upstream-compatible variant set.
- `check_install()` verifies both LZ4 and BZIP2 plugins when enabled.
- The existing upstream PR resolves every outstanding review thread without exposing an unsatisfiable CDF variant.
- CI concretizes v3.0.0 default, minimal, DICOM, and all-upstream-supported-reader configurations, and installs minimal and all-upstream-supported-reader configurations without `|| true`.
- README Spack guidance identifies v3.0.0 as the latest stable release and omits unavailable CDF variant guidance.

**Testing:** Run `spack style`, `spack audit`, default/minimal/DICOM `spack spec` checks, and an all-upstream-supported-reader spec. Run minimal and all-upstream-supported-reader `spack install -v --fail-fast` commands and confirm the Spack CI jobs pass.

**Build System Integration:** No CMake or Autotools source changes. The package retains the existing mappings from Spack variants to `NEP_*` CMake options.

**Definition of Done:** The in-repository recipe supports every published NEP release through v3.0.0 and is directly submit-ready for spack-packages, upstream review feedback is resolved according to Spack policy, CI validates v3.0.0 without ignored install failures, documentation is current, and the sprint issue is linked below.

**GitHub Issue:** #325

#### Sprint 2: Conda
**Detailed Plan**: See `docs/plan/v3.0.1-sprint2-conda.md`

Update the NEP Conda recipe for the existing v3.0.0 release, establish a reliable minimal LZ4 compression package with Fortran wrappers, and submit it to conda-forge through staged-recipes.

**Implementation scope:**
- Update `conda/meta.yaml` from v2.7.1 to the v3.0.0 tag tarball with its verified checksum.
- Build LZ4 compression and Fortran wrappers only; disable BZIP2 because NEP does not provide a BZIP2 HDF5 plugin without changing local build behavior.
- Disable every UDF reader, including DICOM, and remove all reader-only dependencies from the recipe.
- Simplify `conda/build.sh` to pass existing compression-enable and reader-disable options for the Conda recipe only; do not modify CMake, Autotools, or their defaults.
- Validate recipe rendering/linting, build and installed artifacts, then install the built package into a clean Conda environment for a plugin smoke test.
- Submit the initial `nep` recipe to `conda-forge/staged-recipes` and address review through feedstock creation.
- Update installation and release-status documentation to describe the intentionally minimal feature set.

**Clarified decisions:**
- The first conda-forge release packages the existing v3.0.0 source release; no new NEP release is created.
- The initial upstream submission uses `conda-forge/staged-recipes`.
- Only LZ4 and Fortran compression wrappers are enabled. BZIP2 and all format readers, including DICOM, remain disabled.
- Core HDF5, NetCDF-C, NetCDF-Fortran, and LZ4 dependencies remain; BZIP2 and reader-specific dependencies are excluded. This selection applies only to the Conda recipe.
- CI requires recipe render/lint, an unmasked package build, installed-artifact checks, and a clean-environment install smoke test.
- Future Conda sprints may add reader support after the minimal package is established.

**Acceptance Criteria:**
- `conda/meta.yaml` packages v3.0.0 using the verified immutable source tag tarball.
- The recipe and build script enable LZ4 and Fortran only, with BZIP2 and all UDF readers disabled.
- The recipe contains no BZIP2 or reader-only dependency, including `libdicom` and `libjpeg-turbo`.
- The built package contains and tests the LZ4 HDF5 plugin and the Fortran module.
- Conda CI renders/lints, builds, tests, and clean-environment-installs the package without ignored failures.
- An initial conda-forge staged-recipes PR for `nep` is opened and passes its required checks.
- Documentation accurately states the minimal Conda feature set and future reader-expansion path.
- No NEP CMake or Autotools source/default file changes are made, and local source builds remain unchanged.

**Testing:** Run recipe render/lint validation, `conda build conda/ --no-anaconda-upload`, plugin and Fortran-module artifact checks, and a clean-environment install/smoke test. Confirm the Conda CI workflow, staged-recipes validation, and the generated feedstock build pass.

**Build System Integration:** No NEP CMake or Autotools source/default changes. `conda/build.sh` alone invokes CMake with existing `NEP_*` options for the Conda package; local source builds remain unchanged.

**Definition of Done:** A minimal v3.0.0 NEP Conda package with LZ4 and Fortran support is validated end to end and submitted through conda-forge's staged-recipes process. Documentation states that BZIP2 and format readers are deferred until they can be added without changing local builds. No NEP CMake, Autotools, runtime, or local-build behavior changes are made.

**GitHub Issue:** #327
