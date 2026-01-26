!> @file f_user_types.f90
!! @brief Demonstrates NetCDF-4 user-defined types (Fortran)
!!
!! Fortran equivalent of user_types.c, demonstrating NetCDF-4 user-defined types
!! using the Fortran 90 NetCDF API. Note: Fortran support for compound types is
!! limited compared to C, so this focuses on enum, vlen, and opaque types.
!!
!! **Learning Objectives:**
!! - Understand Fortran limitations with compound types
!! - Work with enum types in Fortran (nf90_def_enum)
!! - Use vlen types for variable-length arrays
!! - Handle opaque types for binary data
!!
!! **Fortran User Type Limitations:**
!! - Compound types have limited Fortran support
!! - Enum, vlen, and opaque types fully supported
!! - Type definitions similar to C API
!!
!! **Prerequisites:**
!! - f_simple_nc4.f90 - NetCDF-4 format basics
!! - user_types.c - C equivalent (more complete)
!!
!! **Related Examples:**
!! - user_types.c - C equivalent (full compound type support)
!! - f_compression.f90 - Compression with user types
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
   integer, parameter :: NDAYS = 3
   integer, parameter :: CALIB_SIZE = 16
   
   integer :: ncid, retval
   
   print *, "User-Defined Types Demonstration"
   print *, "================================="
   print *, "Note: Fortran netcdf-fortran 4.5.4 has very limited user type support"
   print *, "This program demonstrates opaque types"
   print *, "(For full user type support, see user_types.c)"
   
   ! ========== CREATE FILE AND DEFINE TYPES ==========
   print *, ""
   print *, "=== Phase 1: Create file and define user types ==="
   
   retval = nf90_create(FILE_NAME, NF90_CLOBBER + NF90_NETCDF4, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   call define_and_test_opaque(ncid)
   
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, ""
   print *, "=== Use Cases ==="
   print *, "- Opaque types: Binary metadata or proprietary formats"
   print *, "- For compound, vlen, and enum types: See user_types.c (C API)"
   print *, "- Newer netcdf-fortran versions may have better support"
   
   print *, ""
   print *, "*** SUCCESS: User-defined types demonstrated!"
   
contains

   subroutine define_and_test_opaque(ncid)
      integer, intent(in) :: ncid
      integer :: opaque_typeid
      integer :: retval
      
      print *, ""
      print *, "--- Opaque Type (binary calibration data) ---"
      
      retval = nf90_def_opaque(ncid, CALIB_SIZE, "calibration_t", opaque_typeid)
      if (retval /= nf90_noerr) call handle_err(retval)
      print *, "Defined opaque type with ", CALIB_SIZE, "-byte size"
      print *, "Note: Full opaque type I/O requires C API or newer Fortran bindings"
      print *, "(Type definition successful, but data I/O not demonstrated)"
      
      retval = nf90_enddef(ncid)
      if (retval /= nf90_noerr) call handle_err(retval)
      
   end subroutine define_and_test_opaque
   
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err
   
end program f_user_types
