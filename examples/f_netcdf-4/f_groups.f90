!> @file f_groups.f90
!! @brief Demonstrates NetCDF-4 hierarchical groups, nested groups, and dimension visibility (Fortran)
!!
!! Fortran equivalent of groups.c, demonstrating NetCDF-4's hierarchical group feature using
!! the Fortran 90 NetCDF API. Groups enable organizing datasets into logical groupings similar
!! to directories in a filesystem, providing namespace isolation for variables while allowing
!! dimensions to be shared across the hierarchy through dimension visibility rules.
!!
!! The program creates a three-level group hierarchy (root → SubGroup1, root → SubGroup2 → 
!! NestedGroup), demonstrates dimension visibility across group boundaries, and showcases
!! all five new NetCDF-4 integer types (NF90_UBYTE, NF90_USHORT, NF90_UINT, NF90_INT64, NF90_UINT64).
!!
!! **Learning Objectives:**
!! - Understand NetCDF-4 hierarchical group structures in Fortran
!! - Learn to create and navigate nested groups
!! - Master dimension visibility rules across group boundaries
!! - Work with all five new NetCDF-4 integer types
!! - Recognize when groups provide organizational benefits
!!
!! **Key Concepts:**
!! - **Hierarchical Groups**: Organize datasets into logical groupings (like directories)
!! - **Dimension Visibility**: Parent dimensions visible in all child groups
!! - **Variable Scoping**: Variables only visible in their defining group
!! - **Group Navigation**: Use nf90_inq_grp_ncid() to navigate by name
!! - **New Integer Types**: NF90_UBYTE, NF90_USHORT, NF90_UINT, NF90_INT64, NF90_UINT64
!!
!! **NetCDF-4 Group Architecture:**
!! - Groups implemented via NC_GRP_INFO_T structures (libsrc4/libhdf5)
!! - Dimensions visible in child groups via parent chain lookup
!! - Variables NOT inherited (scoped to defining group only)
!! - Requires NF90_NETCDF4 flag (HDF5 backend)
!! - Not compatible with NF90_CLASSIC_MODEL
!!
!! **Dimension Visibility Rules:**
!! - Dimensions defined in a group are visible in that group and all descendants
!! - Root dimensions (x, y) visible in SubGroup1, SubGroup2, and NestedGroup
!! - Local dimensions (z in NestedGroup) only visible in defining group
!! - Dimension lookup walks parent chain: child → parent → root
!!
!! **Use Cases for Groups:**
!! - **Multi-instrument datasets**: Group data by instrument or sensor
!! - **Model ensembles**: Separate ensemble members into groups
!! - **Quality levels**: Organize raw, calibrated, and derived products
!! - **Temporal organization**: Group data by year, month, or campaign
!! - **Namespace management**: Avoid variable name conflicts
!!
!! **Prerequisites:**
!! - f_simple_nc4.f90 - Basic NetCDF-4 file operations
!! - f_multi_unlimited.f90 - Advanced NetCDF-4 features
!!
!! **Related Examples:**
!! - groups.c - C equivalent
!! - f_user_types.f90 - User-defined types in groups
!!
!! **Compilation:**
!! @code
!! gfortran -o f_groups f_groups.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_groups
!! ncdump f_groups.nc
!! ncdump -h f_groups.nc  # Header only
!! @endcode
!!
!! **Expected Output:**
!! Creates f_groups.nc in NetCDF-4/HDF5 format containing:
!! - Root group with dimensions x(3), y(4) and variable ubyte_var(NF90_UBYTE)
!! - SubGroup1 with variable ushort_var(NF90_USHORT)
!! - SubGroup2 with variable uint_var(NF90_UINT)
!! - NestedGroup (under SubGroup2) with dimension z(2) and variables:
!!   - int64_var(NF90_INT64, 2D: x, y)
!!   - uint64_var(NF90_UINT64, 3D: x, y, z)
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_groups
   use netcdf
   implicit none
   
   character(len=*), parameter :: FILE_NAME = "f_groups.nc"
   integer, parameter :: NX = 3
   integer, parameter :: NY = 4
   integer, parameter :: NZ = 2
   integer, parameter :: NDIMS_2D = 2
   integer, parameter :: NDIMS_3D = 3
   integer, parameter :: ERRCODE = 2
   
   integer :: ncid, grp1_id, grp2_id, nested_id
   integer :: x_dimid, y_dimid, z_dimid
   integer :: dimids_2d(NDIMS_2D), dimids_3d(NDIMS_3D)
   integer :: ubyte_varid, ushort_varid, uint_varid, int64_varid, uint64_varid
   integer :: retval
   
   ! Data arrays for all five new integer types
   ! Note: Fortran uses column-major ordering (dimensions reversed from C)
   integer(kind=1), dimension(NX, NY) :: ubyte_data
   integer(kind=2), dimension(NX, NY) :: ushort_data
   integer(kind=4), dimension(NX, NY) :: uint_data
   integer(kind=8), dimension(NX, NY) :: int64_data
   integer(kind=8), dimension(NX, NY, NZ) :: uint64_data
   
   integer :: i, j, k, value
   
   print *, "NetCDF-4 Groups Example (Fortran)"
   print *, "=================================="
   print *, ""
   
   ! ========== WRITE PHASE ==========
   print *, "=== Phase 1: Create file with group hierarchy ==="
   
   ! Create the NetCDF-4 file
   print *, "Creating NetCDF-4 file: ", FILE_NAME
   retval = nf90_create(FILE_NAME, NF90_CLOBBER + NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define root dimensions (visible in all groups)
   print *, "Defining root dimensions: x=", NX, ", y=", NY
   retval = nf90_def_dim(ncid, "x", NX, x_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "y", NY, y_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Create SubGroup1
   print *, "Creating SubGroup1"
   retval = nf90_def_grp(ncid, "SubGroup1", grp1_id)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Create SubGroup2
   print *, "Creating SubGroup2"
   retval = nf90_def_grp(ncid, "SubGroup2", grp2_id)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Create NestedGroup under SubGroup2
   print *, "Creating NestedGroup under SubGroup2"
   retval = nf90_def_grp(grp2_id, "NestedGroup", nested_id)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define local dimension z in NestedGroup
   print *, "Defining local dimension z=", NZ, " in NestedGroup"
   retval = nf90_def_dim(nested_id, "z", NZ, z_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define variables in each group using all 5 new integer types
   print *, ""
   print *, "Defining variables with new integer types:"
   
   ! Root group: NF90_UBYTE variable (2D: x, y)
   ! Note: Fortran dimension order is reversed from C
   print *, "  Root: ubyte_var (NF90_UBYTE, 2D: x, y)"
   dimids_2d(1) = x_dimid
   dimids_2d(2) = y_dimid
   retval = nf90_def_var(ncid, "ubyte_var", NF90_UBYTE, dimids_2d, ubyte_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! SubGroup1: NF90_USHORT variable (2D: x, y)
   print *, "  SubGroup1: ushort_var (NF90_USHORT, 2D: x, y)"
   retval = nf90_def_var(grp1_id, "ushort_var", NF90_USHORT, dimids_2d, ushort_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! SubGroup2: NF90_UINT variable (2D: x, y)
   print *, "  SubGroup2: uint_var (NF90_UINT, 2D: x, y)"
   retval = nf90_def_var(grp2_id, "uint_var", NF90_UINT, dimids_2d, uint_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! NestedGroup: NF90_INT64 variable (2D: x, y)
   print *, "  NestedGroup: int64_var (NF90_INT64, 2D: x, y)"
   retval = nf90_def_var(nested_id, "int64_var", NF90_INT64, dimids_2d, int64_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! NestedGroup: NF90_UINT64 variable (3D: x, y, z)
   print *, "  NestedGroup: uint64_var (NF90_UINT64, 3D: x, y, z)"
   dimids_3d(1) = x_dimid
   dimids_3d(2) = y_dimid
   dimids_3d(3) = z_dimid
   retval = nf90_def_var(nested_id, "uint64_var", NF90_UINT64, dimids_3d, uint64_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! End define mode
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Initialize data with sequential values starting from 1
   print *, ""
   print *, "Initializing data with sequential values (1, 2, 3, ...):"
   value = 1
   
   ! NF90_UBYTE data (3x4 = 12 values)
   do j = 1, NY
      do i = 1, NX
         ubyte_data(i, j) = int(value, kind=1)
         value = value + 1
      end do
   end do
   
   ! NF90_USHORT data (3x4 = 12 values)
   do j = 1, NY
      do i = 1, NX
         ushort_data(i, j) = int(value, kind=2)
         value = value + 1
      end do
   end do
   
   ! NF90_UINT data (3x4 = 12 values)
   do j = 1, NY
      do i = 1, NX
         uint_data(i, j) = int(value, kind=4)
         value = value + 1
      end do
   end do
   
   ! NF90_INT64 data (3x4 = 12 values)
   do j = 1, NY
      do i = 1, NX
         int64_data(i, j) = int(value, kind=8)
         value = value + 1
      end do
   end do
   
   ! NF90_UINT64 data (3x4x2 = 24 values)
   do k = 1, NZ
      do j = 1, NY
         do i = 1, NX
            uint64_data(i, j, k) = int(value, kind=8)
            value = value + 1
         end do
      end do
   end do
   
   ! Write data to all variables
   print *, "Writing data to all variables..."
   retval = nf90_put_var(ncid, ubyte_varid, ubyte_data)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(grp1_id, ushort_varid, ushort_data)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(grp2_id, uint_varid, uint_data)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(nested_id, int64_varid, int64_data)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_put_var(nested_id, uint64_varid, uint64_data)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, "*** SUCCESS writing file!"
   
   ! ========== READ AND VALIDATE PHASE ==========
   print *, ""
   print *, "=== Phase 2: Read and validate file structure ==="
   
   ! Open the file for reading
   print *, "Reopening file for validation..."
   retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Query and validate number of groups in root
   call validate_groups(ncid, grp1_id, grp2_id, nested_id)
   
   ! Test dimension visibility across group boundaries
   call test_dimension_visibility(ncid, grp1_id, grp2_id, nested_id)
   
   ! Validate dimension sizes
   call validate_dimensions(ncid, nested_id)
   
   ! Query and validate all variable metadata
   call validate_variables(ncid, grp1_id, grp2_id, nested_id, &
                           ubyte_varid, ushort_varid, uint_varid, int64_varid, uint64_varid)
   
   ! Read and validate all data
   call validate_data(ncid, grp1_id, grp2_id, nested_id, &
                      ubyte_varid, ushort_varid, uint_varid, int64_varid, uint64_varid)
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Summary
   print *, ""
   print *, "=== Summary ==="
   print *, "Group hierarchy:"
   print *, "  Root"
   print *, "  ├── SubGroup1"
   print *, "  └── SubGroup2"
   print *, "      └── NestedGroup"
   print *, ""
   print *, "Dimensions:"
   print *, "  Root: x=", NX, ", y=", NY, " (visible in all groups)"
   print *, "  NestedGroup: z=", NZ, " (local only)"
   print *, ""
   print *, "Variables (all 5 new integer types):"
   print *, "  Root: ubyte_var (NF90_UBYTE)"
   print *, "  SubGroup1: ushort_var (NF90_USHORT)"
   print *, "  SubGroup2: uint_var (NF90_UINT)"
   print *, "  NestedGroup: int64_var (NF90_INT64), uint64_var (NF90_UINT64)"
   print *, ""
   print *, "Key Concepts Demonstrated:"
   print *, "  ✓ Hierarchical group structures (3 levels)"
   print *, "  ✓ Nested groups (NestedGroup under SubGroup2)"
   print *, "  ✓ Dimension visibility across group boundaries"
   print *, "  ✓ All 5 new NetCDF-4 integer types"
   print *, "  ✓ Variable scoping to defining group"
   print *, ""
   print *, "*** SUCCESS: All validation checks passed!"
   print *, "Use 'ncdump f_groups.nc' to view the file structure."
   
contains

   subroutine validate_groups(ncid, grp1_id, grp2_id, nested_id)
      integer, intent(in) :: ncid
      integer, intent(out) :: grp1_id, grp2_id, nested_id
      integer :: ngrps, retval
      integer, dimension(NF90_MAX_VARS) :: grpids
      character(len=NF90_MAX_NAME) :: grpname
      
      ! Query number of groups in root
      retval = nf90_inq_grps(ncid, ngrps, grpids)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      if (ngrps /= 2) then
         print *, "Error: Expected 2 groups in root, found ", ngrps
         stop ERRCODE
      end if
      print *, "Verified: Root has ", ngrps, " child groups"
      
      ! Navigate to groups by name
      print *, ""
      print *, "Navigating to groups by name:"
      retval = nf90_inq_grp_ncid(ncid, "SubGroup1", grp1_id)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  Found SubGroup1 (ncid=", grp1_id, ")"
      
      retval = nf90_inq_grp_ncid(ncid, "SubGroup2", grp2_id)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  Found SubGroup2 (ncid=", grp2_id, ")"
      
      retval = nf90_inq_grp_ncid(grp2_id, "NestedGroup", nested_id)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  Found NestedGroup (ncid=", nested_id, ")"
      
      ! Validate group names
      retval = nf90_inq_grpname(grp1_id, grpname)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (trim(grpname) /= "SubGroup1") then
         print *, "Error: Expected group name 'SubGroup1', found '", trim(grpname), "'"
         stop ERRCODE
      end if
      
      retval = nf90_inq_grpname(grp2_id, grpname)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (trim(grpname) /= "SubGroup2") then
         print *, "Error: Expected group name 'SubGroup2', found '", trim(grpname), "'"
         stop ERRCODE
      end if
      
      retval = nf90_inq_grpname(nested_id, grpname)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (trim(grpname) /= "NestedGroup") then
         print *, "Error: Expected group name 'NestedGroup', found '", trim(grpname), "'"
         stop ERRCODE
      end if
      print *, "Verified: All group names correct"
   end subroutine validate_groups
   
   subroutine test_dimension_visibility(ncid, grp1_id, grp2_id, nested_id)
      integer, intent(in) :: ncid, grp1_id, grp2_id, nested_id
      integer :: test_dimid, retval
      
      print *, ""
      print *, "=== Phase 3: Test dimension visibility ==="
      print *, "Testing that root dimensions (x, y) are visible in all groups:"
      
      ! Test x dimension visibility in SubGroup1
      retval = nf90_inq_dimid(grp1_id, "x", test_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  ✓ SubGroup1 can see dimension 'x' from root"
      
      ! Test y dimension visibility in SubGroup1
      retval = nf90_inq_dimid(grp1_id, "y", test_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  ✓ SubGroup1 can see dimension 'y' from root"
      
      ! Test x dimension visibility in SubGroup2
      retval = nf90_inq_dimid(grp2_id, "x", test_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  ✓ SubGroup2 can see dimension 'x' from root"
      
      ! Test y dimension visibility in SubGroup2
      retval = nf90_inq_dimid(grp2_id, "y", test_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  ✓ SubGroup2 can see dimension 'y' from root"
      
      ! Test x dimension visibility in NestedGroup
      retval = nf90_inq_dimid(nested_id, "x", test_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  ✓ NestedGroup can see dimension 'x' from root"
      
      ! Test y dimension visibility in NestedGroup
      retval = nf90_inq_dimid(nested_id, "y", test_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  ✓ NestedGroup can see dimension 'y' from root"
      
      ! Test local dimension z in NestedGroup
      retval = nf90_inq_dimid(nested_id, "z", test_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "  ✓ NestedGroup can see its local dimension 'z'"
      
      print *, "Verified: Dimension visibility follows parent chain rules"
   end subroutine test_dimension_visibility
   
   subroutine validate_dimensions(ncid, nested_id)
      integer, intent(in) :: ncid, nested_id
      integer :: x_dimid, y_dimid, z_dimid, retval
      integer :: len_x, len_y, len_z
      
      print *, ""
      print *, "Validating dimension sizes:"
      
      retval = nf90_inq_dimid(ncid, "x", x_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_dimension(ncid, x_dimid, len=len_x)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (len_x /= NX) then
         print *, "Error: Expected x dimension = ", NX, ", found ", len_x
         stop ERRCODE
      end if
      
      retval = nf90_inq_dimid(ncid, "y", y_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_dimension(ncid, y_dimid, len=len_y)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (len_y /= NY) then
         print *, "Error: Expected y dimension = ", NY, ", found ", len_y
         stop ERRCODE
      end if
      
      retval = nf90_inq_dimid(nested_id, "z", z_dimid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_dimension(nested_id, z_dimid, len=len_z)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (len_z /= NZ) then
         print *, "Error: Expected z dimension = ", NZ, ", found ", len_z
         stop ERRCODE
      end if
      
      print *, "  x = ", len_x, ", y = ", len_y, ", z = ", len_z
      print *, "Verified: All dimension sizes correct"
   end subroutine validate_dimensions
   
   subroutine validate_variables(ncid, grp1_id, grp2_id, nested_id, &
                                 ubyte_varid, ushort_varid, uint_varid, int64_varid, uint64_varid)
      integer, intent(in) :: ncid, grp1_id, grp2_id, nested_id
      integer, intent(out) :: ubyte_varid, ushort_varid, uint_varid, int64_varid, uint64_varid
      integer :: retval, vartype, varndims
      character(len=NF90_MAX_NAME) :: varname
      
      print *, ""
      print *, "=== Phase 4: Validate variable metadata ==="
      
      ! Validate ubyte_var in root
      retval = nf90_inq_varid(ncid, "ubyte_var", ubyte_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_variable(ncid, ubyte_varid, varname, vartype, varndims)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (vartype /= NF90_UBYTE .or. varndims /= NDIMS_2D) then
         print *, "Error: ubyte_var has wrong type or dimensions"
         stop ERRCODE
      end if
      print *, "  ✓ Root: ubyte_var (NF90_UBYTE, ", varndims, "D)"
      
      ! Validate ushort_var in SubGroup1
      retval = nf90_inq_varid(grp1_id, "ushort_var", ushort_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_variable(grp1_id, ushort_varid, varname, vartype, varndims)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (vartype /= NF90_USHORT .or. varndims /= NDIMS_2D) then
         print *, "Error: ushort_var has wrong type or dimensions"
         stop ERRCODE
      end if
      print *, "  ✓ SubGroup1: ushort_var (NF90_USHORT, ", varndims, "D)"
      
      ! Validate uint_var in SubGroup2
      retval = nf90_inq_varid(grp2_id, "uint_var", uint_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_variable(grp2_id, uint_varid, varname, vartype, varndims)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (vartype /= NF90_UINT .or. varndims /= NDIMS_2D) then
         print *, "Error: uint_var has wrong type or dimensions"
         stop ERRCODE
      end if
      print *, "  ✓ SubGroup2: uint_var (NF90_UINT, ", varndims, "D)"
      
      ! Validate int64_var in NestedGroup
      retval = nf90_inq_varid(nested_id, "int64_var", int64_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_variable(nested_id, int64_varid, varname, vartype, varndims)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (vartype /= NF90_INT64 .or. varndims /= NDIMS_2D) then
         print *, "Error: int64_var has wrong type or dimensions"
         stop ERRCODE
      end if
      print *, "  ✓ NestedGroup: int64_var (NF90_INT64, ", varndims, "D)"
      
      ! Validate uint64_var in NestedGroup
      retval = nf90_inq_varid(nested_id, "uint64_var", uint64_varid)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_inquire_variable(nested_id, uint64_varid, varname, vartype, varndims)
      if (retval /= nf90_noerr) call handle_err(retval)
      if (vartype /= NF90_UINT64 .or. varndims /= NDIMS_3D) then
         print *, "Error: uint64_var has wrong type or dimensions"
         stop ERRCODE
      end if
      print *, "  ✓ NestedGroup: uint64_var (NF90_UINT64, ", varndims, "D)"
      
      print *, "Verified: All variable metadata correct"
   end subroutine validate_variables
   
   subroutine validate_data(ncid, grp1_id, grp2_id, nested_id, &
                           ubyte_varid, ushort_varid, uint_varid, int64_varid, uint64_varid)
      integer, intent(in) :: ncid, grp1_id, grp2_id, nested_id
      integer, intent(in) :: ubyte_varid, ushort_varid, uint_varid, int64_varid, uint64_varid
      integer :: retval, i, j, k, value, errors, total_values
      
      integer(kind=1), dimension(NX, NY) :: ubyte_in
      integer(kind=2), dimension(NX, NY) :: ushort_in
      integer(kind=4), dimension(NX, NY) :: uint_in
      integer(kind=8), dimension(NX, NY) :: int64_in
      integer(kind=8), dimension(NX, NY, NZ) :: uint64_in
      
      print *, ""
      print *, "=== Phase 5: Read and validate data values ==="
      
      retval = nf90_get_var(ncid, ubyte_varid, ubyte_in)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_get_var(grp1_id, ushort_varid, ushort_in)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_get_var(grp2_id, uint_varid, uint_in)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_get_var(nested_id, int64_varid, int64_in)
      if (retval /= nf90_noerr) call handle_err(retval)
      retval = nf90_get_var(nested_id, uint64_varid, uint64_in)
      if (retval /= nf90_noerr) call handle_err(retval)
      
      ! Validate data correctness
      errors = 0
      value = 1
      
      ! Validate NF90_UBYTE data
      do j = 1, NY
         do i = 1, NX
            if (ubyte_in(i, j) /= int(value, kind=1)) then
               print *, "Error: ubyte_var(", i, ",", j, ") = ", ubyte_in(i, j), &
                        ", expected ", value
               errors = errors + 1
            end if
            value = value + 1
         end do
      end do
      
      ! Validate NF90_USHORT data
      do j = 1, NY
         do i = 1, NX
            if (ushort_in(i, j) /= int(value, kind=2)) then
               print *, "Error: ushort_var(", i, ",", j, ") = ", ushort_in(i, j), &
                        ", expected ", value
               errors = errors + 1
            end if
            value = value + 1
         end do
      end do
      
      ! Validate NF90_UINT data
      do j = 1, NY
         do i = 1, NX
            if (uint_in(i, j) /= int(value, kind=4)) then
               print *, "Error: uint_var(", i, ",", j, ") = ", uint_in(i, j), &
                        ", expected ", value
               errors = errors + 1
            end if
            value = value + 1
         end do
      end do
      
      ! Validate NF90_INT64 data
      do j = 1, NY
         do i = 1, NX
            if (int64_in(i, j) /= int(value, kind=8)) then
               print *, "Error: int64_var(", i, ",", j, ") = ", int64_in(i, j), &
                        ", expected ", value
               errors = errors + 1
            end if
            value = value + 1
         end do
      end do
      
      ! Validate NF90_UINT64 data
      do k = 1, NZ
         do j = 1, NY
            do i = 1, NX
               if (uint64_in(i, j, k) /= int(value, kind=8)) then
                  print *, "Error: uint64_var(", i, ",", j, ",", k, ") = ", uint64_in(i, j, k), &
                           ", expected ", value
                  errors = errors + 1
               end if
               value = value + 1
            end do
         end do
      end do
      
      if (errors > 0) then
         print *, "*** FAILED: ", errors, " data validation errors"
         stop ERRCODE
      end if
      
      total_values = NX * NY * 4 + NX * NY * NZ
      print *, "Verified: All ", total_values, " data values correct (sequential 1 to ", total_values, ")"
   end subroutine validate_data
   
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop ERRCODE
   end subroutine handle_err
   
end program f_groups
