!> @file f_user_types.f90
!! @brief Demonstrates NetCDF-4 user-defined types (Fortran)
!!
!! Fortran equivalent of user_types.c, demonstrating NetCDF-4 user-defined
!! types using the Fortran 90 NetCDF API. This example focuses on enum and
!! opaque types, which are the user-defined types that work well from Fortran.
!!
!! Compound types and variable-length types are not demonstrated here because
!! the Fortran language does not provide portable control over derived type
!! memory layout (needed for compounds) or a native equivalent of the C
!! nc_vlen_t struct (needed for vlens). See user_types.c for those types.
!!
!! Enum types work naturally in Fortran because the underlying data is just
!! integers. The enum type definition and member names are metadata; the
!! actual data written and read is an integer array using nf_put_var_int and
!! nf_get_var_int.
!!
!! Opaque types store fixed-size binary blobs. In Fortran, a character buffer
!! of the appropriate length serves as the data container.
!!
!! This example uses the Fortran 77 API (nf_* functions) because the Fortran
!! 90 nf90_def_var wrapper validates the xtype argument against built-in types
!! and rejects user-defined type IDs. The nf_def_var function passes the type
!! ID directly to the C library, which handles user-defined types correctly.
!!
!! **Learning Objectives:**
!! - Define enum types with nf90_def_enum and nf90_insert_enum
!! - Define opaque types with nf90_def_opaque
!! - Write and read enum data as integer arrays
!! - Write and read opaque data as character buffers
!! - Query user-defined type metadata after reopening a file
!!
!! **Prerequisites:**
!! - f_simple_nc4.f90 - NetCDF-4 format basics
!! - user_types.c - C equivalent (includes compound and vlen types)
!!
!! **Related Examples:**
!! - user_types.c - C equivalent with all four user-defined types
!!
!! **Compilation:**
!! @code
!! gfortran -o f_user_types f_user_types.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026

program f_user_types
   use netcdf
   implicit none

   character(len=*), parameter :: FILE_NAME = "f_user_types.nc"
   integer, parameter :: NOBS = 5
   integer, parameter :: CALIB_SIZE = 16

   ! Enum member values
   integer, parameter :: CLEAR = 0
   integer, parameter :: PARTLY_CLOUDY = 1
   integer, parameter :: CLOUDY = 2
   integer, parameter :: OVERCAST = 3

   integer :: ncid, retval
   integer :: enum_typeid, opaque_typeid
   integer :: obs_dimid, dimids(1)
   integer :: enum_varid, opaque_varid

   ! Data arrays
   integer :: cloud_out(NOBS), cloud_in(NOBS)
   character(len=CALIB_SIZE) :: calib_out, calib_in

   ! Variables for type inquiry on reopen
   integer :: ntypes, num_members, base_type_in, base_size_in
   integer :: type_size_in, nfields_in, class_in
   integer :: member_value
   character(len=NF90_MAX_NAME) :: type_name_in, member_name_in
   integer :: typeids(2)

   ! F77 API functions needed for user-defined type variables.
   ! Declared external to bypass any module wrappers that validate xtype.
   ! nf_put_var/nf_get_var are generic untyped I/O that pass raw bytes.
   integer, external :: nf_def_var
   integer, external :: nf_put_var, nf_get_var
   integer, external :: nf_put_var1, nf_get_var1

   integer :: i, errors, index(1)

   ! ========== INITIALIZE DATA ==========
   cloud_out = (/ CLEAR, PARTLY_CLOUDY, CLOUDY, PARTLY_CLOUDY, OVERCAST /)

   do i = 1, CALIB_SIZE
      calib_out(i:i) = char((i - 1) * 17)
   end do

   print *, "NetCDF-4 User-Defined Types (Fortran)"
   print *, "======================================"

   ! ========== WRITE PHASE ==========
   print *, ""
   print *, "=== Phase 1: Create file and define types ==="

   retval = nf90_create(FILE_NAME, NF90_CLOBBER + NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Define enum type: cloud_cover_t based on NF90_INT
   retval = nf90_def_enum(ncid, NF90_INT, "cloud_cover_t", enum_typeid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_insert_enum(ncid, enum_typeid, "CLEAR", CLEAR)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_insert_enum(ncid, enum_typeid, "PARTLY_CLOUDY", PARTLY_CLOUDY)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_insert_enum(ncid, enum_typeid, "CLOUDY", CLOUDY)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_insert_enum(ncid, enum_typeid, "OVERCAST", OVERCAST)
   if (retval /= nf90_noerr) call handle_err(retval)
   print *, "Defined enum type cloud_cover_t with 4 members"

   ! Define opaque type: calibration_t with CALIB_SIZE bytes
   retval = nf90_def_opaque(ncid, CALIB_SIZE, "calibration_t", opaque_typeid)
   if (retval /= nf90_noerr) call handle_err(retval)
   print *, "Defined opaque type calibration_t (", CALIB_SIZE, " bytes)"

   ! Define dimension
   retval = nf90_def_dim(ncid, "obs", NOBS, obs_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Define variables using nf_def_var (F77 API) because nf90_def_var
   ! rejects user-defined type IDs.
   dimids(1) = obs_dimid
   retval = nf_def_var(ncid, "cloud_cover", enum_typeid, 1, dimids, enum_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf_def_var(ncid, "calibration", opaque_typeid, 0, dimids, opaque_varid)
   if (retval /= nf90_noerr) call handle_err(retval)

   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Write enum data using nf_put_var (generic untyped I/O)
   retval = nf_put_var(ncid, enum_varid, cloud_out)
   if (retval /= nf90_noerr) call handle_err(retval)
   print *, "Wrote ", NOBS, " cloud cover values"

   ! Write opaque data using nf_put_var1 (generic untyped I/O)
   index(1) = 1
   retval = nf_put_var1(ncid, opaque_varid, index, calib_out)
   if (retval /= nf90_noerr) call handle_err(retval)
   print *, "Wrote ", CALIB_SIZE, " bytes of calibration data"

   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   print *, "*** SUCCESS writing file!"

   ! ========== READ AND VALIDATE PHASE ==========
   print *, ""
   print *, "=== Phase 2: Reopen and validate ==="

   retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   ! Check that we have two user-defined types
   retval = nf90_inq_typeids(ncid, ntypes, typeids)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (ntypes /= 2) then
      print *, "Error: expected 2 user types, found ", ntypes
      stop 2
   end if
   print *, "Verified: ", ntypes, " user-defined types"

   ! Validate enum type via nf90_inq_user_type
   retval = nf90_inq_user_type(ncid, typeids(1), type_name_in, type_size_in, &
        base_type_in, nfields_in, class_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(type_name_in) /= "cloud_cover_t") then
      print *, "Error: expected type name cloud_cover_t, found ", trim(type_name_in)
      stop 2
   end if
   if (class_in /= NF90_ENUM) then
      print *, "Error: expected NF90_ENUM class"
      stop 2
   end if
   print *, "Verified: cloud_cover_t is enum type"

   ! Validate enum details via nf90_inq_enum
   retval = nf90_inq_enum(ncid, typeids(1), type_name_in, base_type_in, &
        base_size_in, num_members)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (base_type_in /= NF90_INT .or. num_members /= 4) then
      print *, "Error: enum base type or member count wrong"
      stop 2
   end if
   print *, "Verified: base type NF90_INT, ", num_members, " members"

   ! Check each enum member
   retval = nf90_inq_enum_member(ncid, typeids(1), 1, member_name_in, member_value)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(member_name_in) /= "CLEAR" .or. member_value /= 0) stop 2

   retval = nf90_inq_enum_member(ncid, typeids(1), 2, member_name_in, member_value)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(member_name_in) /= "PARTLY_CLOUDY" .or. member_value /= 1) stop 2

   retval = nf90_inq_enum_member(ncid, typeids(1), 3, member_name_in, member_value)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(member_name_in) /= "CLOUDY" .or. member_value /= 2) stop 2

   retval = nf90_inq_enum_member(ncid, typeids(1), 4, member_name_in, member_value)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(member_name_in) /= "OVERCAST" .or. member_value /= 3) stop 2
   print *, "Verified: all 4 enum members correct"

   ! Validate opaque type
   retval = nf90_inq_user_type(ncid, typeids(2), type_name_in, type_size_in, &
        base_type_in, nfields_in, class_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   if (trim(type_name_in) /= "calibration_t") then
      print *, "Error: expected type name calibration_t, found ", trim(type_name_in)
      stop 2
   end if
   if (class_in /= NF90_OPAQUE) then
      print *, "Error: expected NF90_OPAQUE class"
      stop 2
   end if
   if (type_size_in /= CALIB_SIZE) then
      print *, "Error: expected opaque size ", CALIB_SIZE, ", found ", type_size_in
      stop 2
   end if
   print *, "Verified: calibration_t is opaque type (", type_size_in, " bytes)"

   ! Read and validate enum data
   retval = nf90_inq_varid(ncid, "cloud_cover", enum_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf_get_var(ncid, enum_varid, cloud_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   errors = 0
   do i = 1, NOBS
      if (cloud_in(i) /= cloud_out(i)) then
         print *, "Error: cloud_cover(", i, ") = ", cloud_in(i), ", expected ", cloud_out(i)
         errors = errors + 1
      end if
   end do
   if (errors > 0) then
      print *, "*** FAILED: ", errors, " enum data errors"
      stop 2
   end if
   print *, "Verified: all ", NOBS, " cloud cover values correct"

   ! Read and validate opaque data
   retval = nf90_inq_varid(ncid, "calibration", opaque_varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   index(1) = 1
   retval = nf_get_var1(ncid, opaque_varid, index, calib_in)
   if (retval /= nf90_noerr) call handle_err(retval)

   errors = 0
   do i = 1, CALIB_SIZE
      if (calib_in(i:i) /= calib_out(i:i)) then
         print *, "Error: calibration byte ", i, " mismatch"
         errors = errors + 1
      end if
   end do
   if (errors > 0) then
      print *, "*** FAILED: ", errors, " opaque data errors"
      stop 2
   end if
   print *, "Verified: all ", CALIB_SIZE, " calibration bytes correct"

   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)

   print *, ""
   print *, "*** SUCCESS: Enum and opaque types demonstrated!"

contains

   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err

end program f_user_types
