# Issue #111 Refinement Summary

## Executive Summary

Create a comprehensive NetCDF-4 groups example (`groups.c`) demonstrating hierarchical group structures, nested groups, dimension visibility across group boundaries, and all five new NetCDF-4 integer types (NC_UBYTE, NC_USHORT, NC_UINT, NC_INT64, NC_UINT64). The example follows established patterns from `simple_nc4.c` and `multi_unlimited.c` with extensive documentation and validation.

## Clarified Requirements & Acceptance Criteria

### Group Hierarchy
- [ ] 3-level hierarchy: root → SubGroup1, root → SubGroup2 → NestedGroup
- [ ] Root group implicit (file ncid)
- [ ] Two direct children of root (SubGroup1, SubGroup2)
- [ ] One nested group under SubGroup2 (NestedGroup)

### Dimension Visibility
- [ ] Root dimensions (x=3, y=4) defined at root level
- [ ] Local dimension (z=2) defined in NestedGroup only
- [ ] **Explicit testing**: Verify root dimensions visible in all child groups using `nc_inq_dimid()`
- [ ] **Explicit testing**: Verify local dimension (z) accessible in NestedGroup
- [ ] Dimension lookup follows parent chain (NetCDF-4 architecture)

### Variable Definitions (All 5 New Types)
- [ ] Root group: NC_UBYTE variable `ubyte_var` (2D: x, y)
- [ ] SubGroup1: NC_USHORT variable `ushort_var` (2D: x, y)
- [ ] SubGroup2: NC_UINT variable `uint_var` (2D: x, y)
- [ ] NestedGroup: NC_INT64 variable `int64_var` (2D: x, y)
- [ ] NestedGroup: NC_UINT64 variable `uint64_var` (3D: x, y, z)

### Data Pattern
- [ ] Use sequential integers starting from 1 (1, 2, 3, ..., 12 for 3x4 arrays)
- [ ] For 3D variable (3x4x2): sequential values 1-24
- [ ] Consistent pattern across all variables for easy validation

### Documentation Style
- [ ] Match `simple_nc4.c` and `multi_unlimited.c` documentation depth
- [ ] Comprehensive Doxygen header (60+ lines)
- [ ] Learning Objectives section
- [ ] Key Concepts section (groups, dimension visibility, new types)
- [ ] Prerequisites section (reference simple_nc4.c)
- [ ] Related Examples section
- [ ] Expected Output section

### Error Handling
- [ ] Use ERR() macro pattern for concise error handling
- [ ] Check all NetCDF API return codes
- [ ] Exit with non-zero code on any error
- [ ] Print descriptive error messages using `nc_strerror()`

### Build System Integration
- [ ] CMake: Add `groups` to `NETCDF4_EXAMPLES` list in `examples/netcdf-4/CMakeLists.txt`
- [ ] CMake: Automatic test registration via `add_test()`
- [ ] Autotools: Add to `check_PROGRAMS` in `examples/netcdf-4/Makefile.am`
- [ ] Autotools: Add to `TESTS` variable
- [ ] No math library needed (no floating-point math functions)

## Implementation Approach

### Architecture Foundation
Based on NetCDF-C architecture skill:
- Groups use `NC_GRP_INFO_T` hierarchical structures (libhdf5/libsrc4)
- Dimensions visible in child groups via parent chain lookup
- Variables scoped to defining group only
- Requires NC_NETCDF4 flag (HDF5 backend)
- Not compatible with NC_CLASSIC_MODEL

### Code Structure
Follow established example pattern:
1. **Write Phase**: Create file, define groups, dimensions, variables, write data
2. **Read/Validate Phase**: Reopen file, verify structure, validate data

### Key NetCDF-4 APIs
- `nc_def_grp(parent_ncid, name, &grp_id)` - Create groups
- `nc_inq_grps(ncid, &ngrps, grpids)` - Query child groups
- `nc_inq_grpname(ncid, name, len)` - Get group name
- `nc_inq_grp_ncid(ncid, name, &grp_id)` - Navigate to group by name
- `nc_inq_dimid(ncid, name, &dimid)` - Test dimension visibility

## Implementation Steps

### Step 1: Create File Structure (2 hours)
- Create `examples/netcdf-4/groups.c`
- Add comprehensive Doxygen documentation header
- Include standard headers and define constants
- Define ERR() macro and error code

### Step 2: Implement Write Phase (3 hours)
- Create NetCDF-4 file with NC_NETCDF4 flag
- Define root dimensions (x=3, y=4)
- Create SubGroup1 using `nc_def_grp(ncid, "SubGroup1", &grp1_id)`
- Create SubGroup2 using `nc_def_grp(ncid, "SubGroup2", &grp2_id)`
- Create NestedGroup using `nc_def_grp(grp2_id, "NestedGroup", &nested_id)`
- Define local dimension z=2 in NestedGroup
- Define all 5 variables with new integer types
- Initialize data arrays with sequential values (1, 2, 3, ...)
- Write data to all variables
- Close file

### Step 3: Implement Read/Validate Phase (4 hours)
- Reopen file in NC_NOWRITE mode
- Query and validate number of groups using `nc_inq_grps()`
- Navigate to each group using `nc_inq_grp_ncid()`
- Validate group names using `nc_inq_grpname()`
- **Dimension visibility testing**:
  - Test `nc_inq_dimid(grp1_id, "x", &dimid)` succeeds (root dim visible in child)
  - Test `nc_inq_dimid(grp1_id, "y", &dimid)` succeeds
  - Test `nc_inq_dimid(nested_id, "x", &dimid)` succeeds (root dim visible in nested)
  - Test `nc_inq_dimid(nested_id, "y", &dimid)` succeeds
  - Test `nc_inq_dimid(nested_id, "z", &dimid)` succeeds (local dim)
- Query and validate all variable metadata (name, type, dimensions)
- Read all variable data
- Verify data values match written values (1, 2, 3, ...)
- Close file

### Step 4: CMake Build Integration (1 hour)
- Edit `examples/netcdf-4/CMakeLists.txt`
- Add `groups` to `NETCDF4_EXAMPLES` list
- Verify automatic executable creation and test registration
- Test build with `cmake -B build -DBUILD_EXAMPLES=ON && cmake --build build`

### Step 5: Autotools Build Integration (1 hour)
- Edit `examples/netcdf-4/Makefile.am`
- Add `groups` to `check_PROGRAMS`
- Add `groups_SOURCES = groups.c`
- Add `groups` to `TESTS` variable
- Test build with `./configure --enable-examples && make && make check`

### Step 6: Testing & Validation (2 hours)
- Run example: `./groups`
- Verify exit code 0
- Verify NetCDF file created: `groups.nc`
- Manual inspection: `ncdump groups.nc`
- Verify group structure in output
- Verify all 5 variables present with correct types
- Verify dimension visibility
- Test in both build systems (CMake and Autotools)
- Test with BUILD_EXAMPLES=ON/OFF configurations

## Dependencies

**No blocking dependencies** - This is a standalone example using existing NetCDF-4 APIs.

**Related work**:
- Follows patterns from existing examples (simple_nc4.c, multi_unlimited.c)
- Uses NetCDF-4 group APIs documented in NetCDF-C library
- Integrates with existing build systems

## Testing Requirements

### Unit Testing
- [ ] Example runs without errors (exit code 0)
- [ ] NetCDF file created successfully
- [ ] All groups created and navigable
- [ ] All dimensions defined and visible per scope rules
- [ ] All 5 variables created with correct types
- [ ] Data written and read back correctly
- [ ] Dimension visibility explicitly tested

### Integration Testing
- [ ] CMake build with BUILD_EXAMPLES=ON
- [ ] CMake build with BUILD_EXAMPLES=OFF (example not built)
- [ ] Autotools build with --enable-examples
- [ ] Autotools build with --disable-examples (example not built)
- [ ] CTest execution: `ctest --test-dir build -R groups`
- [ ] Autotools execution: `make check` includes groups test

### Manual Validation
- [ ] `ncdump groups.nc` shows correct structure
- [ ] Group hierarchy visible: root, SubGroup1, SubGroup2, NestedGroup
- [ ] Dimensions: x=3, y=4 at root; z=2 in NestedGroup
- [ ] All 5 variables with correct types and data

## Risks & Mitigations

### Risk: NetCDF-4 API Availability
**Mitigation**: NetCDF-C v4.9+ required (already a project dependency). Group APIs are core NetCDF-4 features.

### Risk: Build System Complexity
**Mitigation**: Follow exact pattern from existing examples. Both CMakeLists.txt and Makefile.am have established patterns.

### Risk: Dimension Visibility Confusion
**Mitigation**: Add clear comments explaining NetCDF-4 dimension scoping rules. Reference NetCDF-C architecture documentation.

### Risk: New Integer Type Support
**Mitigation**: All 5 types (NC_UBYTE, NC_USHORT, NC_UINT, NC_INT64, NC_UINT64) are standard NetCDF-4 types available since NetCDF-4.0.

## Notes

### NetCDF-4 Group Architecture
From netcdf-architecture skill:
- Groups implemented in libsrc4/ and libhdf5/
- `NC_GRP_INFO_T` structure contains child groups, dimensions, variables
- Dimension lookup walks parent chain: child → parent → root
- Variables are NOT inherited (scoped to defining group)
- HDF5 backend required (NC_NETCDF4 flag)

### Example Code Patterns
Established in simple_nc4.c and multi_unlimited.c:
- Write phase followed by read/validate phase
- ERR() macro for error handling
- Detailed validation with specific error messages
- Educational comments explaining concepts
- Sequential data patterns for easy verification

### Build System Patterns
From examples/netcdf-4/:
- CMake: List in NETCDF4_EXAMPLES, automatic test registration
- Autotools: check_PROGRAMS + TESTS variables
- Math library only if using math.h functions
- CLEANFILES = *.nc for cleanup

## Definition of Done

- [ ] groups.c created with comprehensive documentation
- [ ] All 5 new integer types demonstrated
- [ ] 3-level group hierarchy implemented
- [ ] Dimension visibility explicitly tested
- [ ] Example compiles in CMake build
- [ ] Example compiles in Autotools build
- [ ] Example runs as test in both build systems
- [ ] Example passes (exit code 0)
- [ ] NetCDF file created with correct structure
- [ ] Manual ncdump verification shows expected structure
- [ ] CI passes with new example
- [ ] Code follows established example patterns
- [ ] Documentation matches existing example quality

## Estimated Effort

**Total: 13 hours** (approximately 2 working days)

- File structure & documentation: 2 hours
- Write phase implementation: 3 hours
- Read/validate phase implementation: 4 hours
- CMake integration: 1 hour
- Autotools integration: 1 hour
- Testing & validation: 2 hours

## References

- NetCDF-4 Groups API: https://docs.unidata.ucar.edu/netcdf-c/current/group__groups.html
- NetCDF-4 Types: https://docs.unidata.ucar.edu/netcdf-c/current/data_type.html
- Existing examples: `examples/netcdf-4/simple_nc4.c`, `examples/netcdf-4/multi_unlimited.c`
- NetCDF-C Architecture Skill: `.windsurf/skills/netcdf-architecture/`
- Build patterns: `examples/netcdf-4/CMakeLists.txt`, `examples/netcdf-4/Makefile.am`
